// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2026 Serina Sakurai
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
//
// -----------------------------------------------------------------------

#include "core/avdec/video_encoder.hpp"

#include "core/avdec/omv.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/dict.h>
#include <libavutil/error.h>
#include <libavutil/mem.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}

#include <algorithm>
#include <climits>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr int kAvioBufferSize = 32 * 1024;

std::string AvError(int error) {
  char buffer[AV_ERROR_MAX_STRING_SIZE] = {};
  av_strerror(error, buffer, sizeof(buffer));
  return buffer;
}

void CheckAv(int result, const std::string& context) {
  if (result < 0)
    throw std::runtime_error(context + ": " + AvError(result));
}

struct OStreamWriter {
  std::ostream* out = nullptr;
};

int WritePacket(void* opaque, const uint8_t* buffer, int buffer_size) {
  auto* writer = static_cast<OStreamWriter*>(opaque);
  if (!writer || !writer->out)
    return AVERROR(EIO);

  writer->out->write(reinterpret_cast<const char*>(buffer), buffer_size);
  if (!*writer->out)
    return AVERROR(EIO);
  return buffer_size;
}

struct AvFormatContextDeleter {
  void operator()(AVFormatContext* context) const {
    if (context)
      avformat_free_context(context);
  }
};

struct AvCodecContextDeleter {
  void operator()(AVCodecContext* context) const {
    if (context)
      avcodec_free_context(&context);
  }
};

struct AvioContextDeleter {
  void operator()(AVIOContext* context) const {
    if (context)
      avio_context_free(&context);
  }
};

struct AvFrameDeleter {
  void operator()(AVFrame* frame) const {
    if (frame)
      av_frame_free(&frame);
  }
};

struct AvPacketDeleter {
  void operator()(AVPacket* packet) const {
    if (packet)
      av_packet_free(&packet);
  }
};

struct SwsContextDeleter {
  void operator()(SwsContext* context) const {
    if (context)
      sws_freeContext(context);
  }
};

using AvFormatContextPtr =
    std::unique_ptr<AVFormatContext, AvFormatContextDeleter>;
using AvCodecContextPtr =
    std::unique_ptr<AVCodecContext, AvCodecContextDeleter>;
using AvioContextPtr = std::unique_ptr<AVIOContext, AvioContextDeleter>;
using AvFramePtr = std::unique_ptr<AVFrame, AvFrameDeleter>;
using AvPacketPtr = std::unique_ptr<AVPacket, AvPacketDeleter>;
using SwsContextPtr = std::unique_ptr<SwsContext, SwsContextDeleter>;

bool SupportsPixelFormat(const AVCodec* codec, AVPixelFormat format) {
  const void* configs = nullptr;
  int config_count = 0;
  const int result = avcodec_get_supported_config(
      nullptr, codec, AV_CODEC_CONFIG_PIX_FORMAT, 0, &configs, &config_count);
  if (result < 0 || !configs)
    return true;

  const auto* formats = static_cast<const AVPixelFormat*>(configs);
  for (int i = 0; i < config_count; ++i) {
    if (formats[i] == format)
      return true;
  }
  return false;
}

AVPixelFormat SelectPixelFormat(const AVCodec* codec, bool has_alpha) {
  const AVPixelFormat desired =
      has_alpha ? AV_PIX_FMT_YUVA420P : AV_PIX_FMT_YUV420P;
  if (SupportsPixelFormat(codec, desired))
    return desired;
  if (SupportsPixelFormat(codec, AV_PIX_FMT_YUV420P))
    return AV_PIX_FMT_YUV420P;

  throw std::runtime_error("WebM VP9 encoder does not support yuv420p");
}

std::int64_t FrameDurationUsec(const OmvDecoder& decoder) {
  if (decoder.info().usec_per_frame > 0)
    return decoder.info().usec_per_frame;

  if (decoder.total_time() > 0 && decoder.frame_count() > 0) {
    const std::int64_t total_usec =
        static_cast<std::int64_t>(decoder.total_time()) * 1000;
    return std::max<std::int64_t>(1, total_usec / decoder.frame_count());
  }

  return 33333;
}

AVRational FrameRateFromDuration(std::int64_t frame_duration_usec) {
  AVRational frame_rate = {30, 1};
  av_reduce(&frame_rate.num, &frame_rate.den, 1000000,
            frame_duration_usec, INT_MAX);
  return frame_rate;
}

AvioContextPtr CreateAvioContext(OStreamWriter& writer) {
  auto* buffer = static_cast<unsigned char*>(av_malloc(kAvioBufferSize));
  if (!buffer)
    throw std::runtime_error("Could not allocate WebM output buffer");

  AVIOContext* raw_context =
      avio_alloc_context(buffer, kAvioBufferSize, 1, &writer, nullptr,
                         WritePacket, nullptr);
  if (!raw_context) {
    av_free(buffer);
    throw std::runtime_error("Could not create WebM output context");
  }

  return AvioContextPtr(raw_context);
}

void DrainEncoder(AVCodecContext* codec_context,
                  AVFormatContext* format_context,
                  AVStream* stream,
                  AVPacket* packet) {
  while (true) {
    const int receive_result = avcodec_receive_packet(codec_context, packet);
    if (receive_result == AVERROR(EAGAIN) || receive_result == AVERROR_EOF)
      return;
    CheckAv(receive_result, "Could not receive encoded WebM packet");

    av_packet_rescale_ts(packet, codec_context->time_base, stream->time_base);
    packet->stream_index = stream->index;
    CheckAv(av_interleaved_write_frame(format_context, packet),
            "Could not write WebM packet");
    av_packet_unref(packet);
  }
}

void SetEncoderOptions(AVDictionary** options) {
  av_dict_set(options, "deadline", "realtime", 0);
  av_dict_set(options, "cpu-used", "8", 0);
  av_dict_set(options, "row-mt", "1", 0);
  av_dict_set(options, "crf", "32", 0);
  av_dict_set(options, "auto-alt-ref", "0", 0);
}

}  // namespace

void EncodeOmvAsWebm(const std::filesystem::path& path, std::ostream& out) {
  OmvDecoder decoder(path, false);
  const int frame_count = decoder.frame_count();
  if (frame_count <= 0)
    throw std::runtime_error("OMV contains no video frames: " + path.string());

  const AVCodec* codec = avcodec_find_encoder_by_name("libvpx-vp9");
  if (!codec)
    codec = avcodec_find_encoder(AV_CODEC_ID_VP9);
  if (!codec)
    throw std::runtime_error("No VP9 encoder is available for WebM output");

  const bool has_alpha = decoder.info().type == OmvDecoder::kTypeRgba;
  const AVPixelFormat output_format = SelectPixelFormat(codec, has_alpha);
  const std::int64_t frame_duration_usec = FrameDurationUsec(decoder);

  OStreamWriter writer{&out};
  AvioContextPtr avio_context = CreateAvioContext(writer);

  AVFormatContext* raw_format_context = nullptr;
  CheckAv(avformat_alloc_output_context2(&raw_format_context, nullptr, "webm",
                                         nullptr),
          "Could not create WebM muxer");
  AvFormatContextPtr format_context(raw_format_context);
  format_context->pb = avio_context.get();
  format_context->flags |= AVFMT_FLAG_CUSTOM_IO;

  AVStream* stream = avformat_new_stream(format_context.get(), nullptr);
  if (!stream)
    throw std::runtime_error("Could not create WebM video stream");

  AvCodecContextPtr codec_context(avcodec_alloc_context3(codec));
  if (!codec_context)
    throw std::runtime_error("Could not allocate VP9 encoder context");

  codec_context->codec_id = AV_CODEC_ID_VP9;
  codec_context->codec_type = AVMEDIA_TYPE_VIDEO;
  codec_context->width = decoder.width();
  codec_context->height = decoder.height();
  codec_context->pix_fmt = output_format;
  codec_context->time_base = AVRational{1, 1000000};
  codec_context->framerate = FrameRateFromDuration(frame_duration_usec);
  codec_context->gop_size = std::min(frame_count, 120);
  codec_context->max_b_frames = 0;
  codec_context->thread_count =
      static_cast<int>(
          std::min(8u, std::max(1u, std::thread::hardware_concurrency())));

  if (format_context->oformat->flags & AVFMT_GLOBALHEADER)
    codec_context->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

  AVDictionary* raw_options = nullptr;
  SetEncoderOptions(&raw_options);
  const int open_result =
      avcodec_open2(codec_context.get(), codec, &raw_options);
  av_dict_free(&raw_options);
  CheckAv(open_result, "Could not open VP9 encoder");

  CheckAv(avcodec_parameters_from_context(stream->codecpar,
                                          codec_context.get()),
          "Could not copy WebM stream parameters");
  stream->time_base = codec_context->time_base;

  CheckAv(avformat_write_header(format_context.get(), nullptr),
          "Could not write WebM header");

  AvFramePtr frame(av_frame_alloc());
  if (!frame)
    throw std::runtime_error("Could not allocate WebM frame");
  frame->format = codec_context->pix_fmt;
  frame->width = codec_context->width;
  frame->height = codec_context->height;
  CheckAv(av_frame_get_buffer(frame.get(), 32),
          "Could not allocate WebM frame buffer");

  AvPacketPtr packet(av_packet_alloc());
  if (!packet)
    throw std::runtime_error("Could not allocate WebM packet");

  SwsContextPtr sws_context(sws_getContext(
      decoder.width(), decoder.height(), AV_PIX_FMT_BGRA,
      codec_context->width, codec_context->height, codec_context->pix_fmt,
      SWS_BILINEAR, nullptr, nullptr, nullptr));
  if (!sws_context)
    throw std::runtime_error("Could not create WebM pixel converter");

  std::vector<char> bgra(static_cast<std::size_t>(decoder.width()) *
                         decoder.height() * 4);

  for (int i = 0; i < frame_count; ++i) {
    decoder.DecodeFrame(i, bgra, i == 0);
    CheckAv(av_frame_make_writable(frame.get()),
            "Could not make WebM frame writable");

    const uint8_t* source_data[] = {
        reinterpret_cast<const uint8_t*>(bgra.data()), nullptr, nullptr,
        nullptr};
    const int source_linesize[] = {decoder.width() * 4, 0, 0, 0};
    sws_scale(sws_context.get(), source_data, source_linesize, 0,
              decoder.height(), frame->data, frame->linesize);

    frame->pts = static_cast<std::int64_t>(i) * frame_duration_usec;
    frame->duration = frame_duration_usec;
    CheckAv(avcodec_send_frame(codec_context.get(), frame.get()),
            "Could not send OMV frame to VP9 encoder");
    DrainEncoder(codec_context.get(), format_context.get(), stream,
                 packet.get());
  }

  CheckAv(avcodec_send_frame(codec_context.get(), nullptr),
          "Could not flush VP9 encoder");
  DrainEncoder(codec_context.get(), format_context.get(), stream, packet.get());

  CheckAv(av_write_trailer(format_context.get()),
          "Could not write WebM trailer");
  out.flush();
}
