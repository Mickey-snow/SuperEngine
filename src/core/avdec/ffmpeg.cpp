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

#include "core/avdec/ffmpeg.hpp"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavutil/frame.h>
#include <libavutil/mem.h>
#include <libavutil/samplefmt.h>
#include <libswscale/swscale.h>
}

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <format>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace {

constexpr int kAvioBufferSize = 32 * 1024;
constexpr AVRational kMilliseconds = {1, 1000};

std::string AvError(int error) {
  char buffer[AV_ERROR_MAX_STRING_SIZE] = {};
  av_strerror(error, buffer, sizeof(buffer));
  return buffer;
}

void CheckAv(int result, const std::string& context) {
  if (result < 0)
    throw std::runtime_error(context + ": " + AvError(result));
}

struct AvFormatContextDeleter {
  void operator()(AVFormatContext* context) const {
    if (context)
      avformat_close_input(&context);
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

int ChannelCount(const AVCodecContext* codec_context) {
#if LIBAVUTIL_VERSION_MAJOR >= 57
  if (codec_context->ch_layout.nb_channels > 0)
    return codec_context->ch_layout.nb_channels;
#endif
#if FF_API_OLD_CHANNEL_LAYOUT
  if (codec_context->channels > 0)
    return codec_context->channels;
#endif
  return 2;
}

int ChannelCount(const AVFrame* frame, const AVCodecContext* codec_context) {
#if LIBAVUTIL_VERSION_MAJOR >= 57
  if (frame->ch_layout.nb_channels > 0)
    return frame->ch_layout.nb_channels;
#endif
#if FF_API_OLD_CHANNEL_LAYOUT
  if (frame->channels > 0)
    return frame->channels;
#endif
  return ChannelCount(codec_context);
}

AVSpec SpecFromCodec(const AVCodecContext* codec_context) {
  const AVSampleFormat packed =
      av_get_packed_sample_fmt(codec_context->sample_fmt);
  AV_SAMPLE_FMT sample_format = AV_SAMPLE_FMT::NONE;
  switch (packed) {
    case AV_SAMPLE_FMT_U8:
      sample_format = AV_SAMPLE_FMT::U8;
      break;
    case AV_SAMPLE_FMT_S16:
      sample_format = AV_SAMPLE_FMT::S16;
      break;
    case AV_SAMPLE_FMT_S32:
      sample_format = AV_SAMPLE_FMT::S32;
      break;
    case AV_SAMPLE_FMT_S64:
      sample_format = AV_SAMPLE_FMT::S64;
      break;
    case AV_SAMPLE_FMT_FLT:
      sample_format = AV_SAMPLE_FMT::FLT;
      break;
    case AV_SAMPLE_FMT_DBL:
      sample_format = AV_SAMPLE_FMT::DBL;
      break;
    default:
      throw std::runtime_error("Unsupported FFmpeg audio sample format: " +
                               std::string(av_get_sample_fmt_name(
                                   codec_context->sample_fmt)));
  }

  return {.sample_rate = codec_context->sample_rate,
          .sample_format = sample_format,
          .channel_count = ChannelCount(codec_context)};
}

template <typename T>
AudioData ExtractAudioFrameAs(const AVFrame* frame,
                              const AVCodecContext* codec_context,
                              AV_SAMPLE_FMT format) {
  const int channels = ChannelCount(frame, codec_context);
  const int samples = frame->nb_samples;
  const bool planar = av_sample_fmt_is_planar(
      static_cast<AVSampleFormat>(frame->format));

  std::vector<T> pcm;
  pcm.reserve(static_cast<std::size_t>(samples) * channels);
  if (planar) {
    for (int sample = 0; sample < samples; ++sample) {
      for (int channel = 0; channel < channels; ++channel) {
        const auto* plane =
            reinterpret_cast<const T*>(frame->extended_data[channel]);
        pcm.push_back(plane[sample]);
      }
    }
  } else {
    const auto* data = reinterpret_cast<const T*>(frame->extended_data[0]);
    pcm.assign(data, data + static_cast<std::size_t>(samples) * channels);
  }

  return {.spec = {.sample_rate = frame->sample_rate,
                   .sample_format = format,
                   .channel_count = channels},
          .data = std::move(pcm)};
}

AudioData ExtractAudioFrame(const AVFrame* frame,
                            const AVCodecContext* codec_context) {
  const AVSampleFormat packed =
      av_get_packed_sample_fmt(static_cast<AVSampleFormat>(frame->format));
  switch (packed) {
    case AV_SAMPLE_FMT_U8:
      return ExtractAudioFrameAs<avsample_u8_t>(frame, codec_context,
                                                AV_SAMPLE_FMT::U8);
    case AV_SAMPLE_FMT_S16:
      return ExtractAudioFrameAs<avsample_s16_t>(frame, codec_context,
                                                 AV_SAMPLE_FMT::S16);
    case AV_SAMPLE_FMT_S32:
      return ExtractAudioFrameAs<avsample_s32_t>(frame, codec_context,
                                                 AV_SAMPLE_FMT::S32);
    case AV_SAMPLE_FMT_S64:
      return ExtractAudioFrameAs<avsample_s64_t>(frame, codec_context,
                                                 AV_SAMPLE_FMT::S64);
    case AV_SAMPLE_FMT_FLT:
      return ExtractAudioFrameAs<avsample_flt_t>(frame, codec_context,
                                                 AV_SAMPLE_FMT::FLT);
    case AV_SAMPLE_FMT_DBL:
      return ExtractAudioFrameAs<avsample_dbl_t>(frame, codec_context,
                                                 AV_SAMPLE_FMT::DBL);
    default:
      throw std::runtime_error("Unsupported decoded FFmpeg audio sample format");
  }
}

struct BufferReader {
  std::string_view data;
  std::int64_t position = 0;
};

int ReadPacket(void* opaque, std::uint8_t* buffer, int buffer_size) {
  auto* reader = static_cast<BufferReader*>(opaque);
  if (!reader)
    return AVERROR(EIO);

  const std::int64_t remaining =
      static_cast<std::int64_t>(reader->data.size()) - reader->position;
  if (remaining <= 0)
    return AVERROR_EOF;

  const int to_read =
      static_cast<int>(std::min<std::int64_t>(buffer_size, remaining));
  std::memcpy(buffer, reader->data.data() + reader->position, to_read);
  reader->position += to_read;
  return to_read;
}

std::int64_t SeekPacket(void* opaque, std::int64_t offset, int whence) {
  auto* reader = static_cast<BufferReader*>(opaque);
  if (!reader)
    return AVERROR(EIO);

  if (whence == AVSEEK_SIZE)
    return static_cast<std::int64_t>(reader->data.size());

  std::int64_t next = 0;
  switch (whence) {
    case SEEK_SET:
      next = offset;
      break;
    case SEEK_CUR:
      next = reader->position + offset;
      break;
    case SEEK_END:
      next = static_cast<std::int64_t>(reader->data.size()) + offset;
      break;
    default:
      return AVERROR(EINVAL);
  }

  if (next < 0 || next > static_cast<std::int64_t>(reader->data.size()))
    return AVERROR(EINVAL);

  reader->position = next;
  return next;
}

AvioContextPtr CreateReadAvioContext(BufferReader& reader) {
  auto* buffer = static_cast<unsigned char*>(av_malloc(kAvioBufferSize));
  if (!buffer)
    throw std::runtime_error("Could not allocate FFmpeg read buffer");

  AVIOContext* raw_context =
      avio_alloc_context(buffer, kAvioBufferSize, 0, &reader, ReadPacket,
                         nullptr, SeekPacket);
  if (!raw_context) {
    av_free(buffer);
    throw std::runtime_error("Could not create FFmpeg read context");
  }

  return AvioContextPtr(raw_context);
}

int FrameDurationUsec(AVStream* stream) {
  if (stream->avg_frame_rate.num > 0 && stream->avg_frame_rate.den > 0) {
    return static_cast<int>(
        std::max(1.0, 1000000.0 / av_q2d(stream->avg_frame_rate)));
  }
  if (stream->r_frame_rate.num > 0 && stream->r_frame_rate.den > 0) {
    return static_cast<int>(
        std::max(1.0, 1000000.0 / av_q2d(stream->r_frame_rate)));
  }
  return 33333;
}

int StreamDurationMs(AVFormatContext* format_context, AVStream* stream) {
  if (stream->duration != AV_NOPTS_VALUE && stream->duration > 0) {
    return static_cast<int>(
        av_rescale_q(stream->duration, stream->time_base, kMilliseconds));
  }
  if (format_context->duration != AV_NOPTS_VALUE && format_context->duration > 0)
    return static_cast<int>(format_context->duration / 1000);
  return 0;
}

int FrameTimeMs(const AVFrame* frame, AVStream* stream, int fallback_ms) {
  std::int64_t timestamp = frame->best_effort_timestamp;
  if (timestamp == AV_NOPTS_VALUE)
    timestamp = frame->pts;
  if (timestamp == AV_NOPTS_VALUE)
    return fallback_ms;

  if (stream->start_time != AV_NOPTS_VALUE)
    timestamp -= stream->start_time;

  return std::max<int>(
      0, static_cast<int>(av_rescale_q(timestamp, stream->time_base,
                                       kMilliseconds)));
}

}  // namespace

struct FfmpegVideoDecoder::Impl {
  explicit Impl(std::filesystem::path in_path) : path(std::move(in_path)) {
    AVFormatContext* raw_format_context = nullptr;
    CheckAv(avformat_open_input(&raw_format_context, path.string().c_str(),
                                nullptr, nullptr),
            "Could not open movie");
    format_context.reset(raw_format_context);
    CheckAv(avformat_find_stream_info(format_context.get(), nullptr),
            "Could not read movie stream info");

    const int best_stream = av_find_best_stream(
        format_context.get(), AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    CheckAv(best_stream, "Could not find movie video stream");
    stream_index = best_stream;
    stream = format_context->streams[stream_index];

    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec)
      throw std::runtime_error("No FFmpeg decoder for movie video stream");

    codec_context.reset(avcodec_alloc_context3(codec));
    if (!codec_context)
      throw std::runtime_error("Could not allocate movie video codec context");
    CheckAv(avcodec_parameters_to_context(codec_context.get(),
                                          stream->codecpar),
            "Could not copy movie video parameters");
    CheckAv(avcodec_open2(codec_context.get(), codec, nullptr),
            "Could not open movie video decoder");

    info.size = Size(codec_context->width, codec_context->height);
    info.total_time = StreamDurationMs(format_context.get(), stream);
    info.usec_per_frame = FrameDurationUsec(stream);

    sws_context.reset(sws_getContext(
        codec_context->width, codec_context->height, codec_context->pix_fmt,
        codec_context->width, codec_context->height, AV_PIX_FMT_BGRA,
        SWS_BILINEAR, nullptr, nullptr, nullptr));
    if (!sws_context)
      throw std::runtime_error("Could not create movie video converter");

    frame.reset(av_frame_alloc());
    packet.reset(av_packet_alloc());
    if (!frame || !packet)
      throw std::runtime_error("Could not allocate movie video decode buffers");
  }

  bool DecodeNext(std::span<char> bgra, int& decoded_time_ms) {
    while (true) {
      const int receive_result =
          avcodec_receive_frame(codec_context.get(), frame.get());
      if (receive_result == 0) {
        const int next_time = FrameTimeMs(
            frame.get(), stream,
            current_frame_time_ms < 0
                ? 0
                : current_frame_time_ms + info.usec_per_frame / 1000);

        std::uint8_t* dst_data[4] = {
            reinterpret_cast<std::uint8_t*>(bgra.data()), nullptr, nullptr,
            nullptr};
        int dst_linesize[4] = {codec_context->width * 4, 0, 0, 0};
        sws_scale(sws_context.get(), frame->data, frame->linesize, 0,
                  codec_context->height, dst_data, dst_linesize);
        av_frame_unref(frame.get());

        decoded_time_ms = next_time;
        return true;
      }
      if (receive_result == AVERROR_EOF) {
        output_eof = true;
        return false;
      }
      if (receive_result != AVERROR(EAGAIN))
        CheckAv(receive_result, "Could not decode movie video frame");

      while (true) {
        const int read_result = av_read_frame(format_context.get(), packet.get());
        if (read_result == AVERROR_EOF) {
          CheckAv(avcodec_send_packet(codec_context.get(), nullptr),
                  "Could not drain movie video decoder");
          input_eof = true;
          break;
        }
        CheckAv(read_result, "Could not read movie video packet");

        if (packet->stream_index == stream_index) {
          const int send_result =
              avcodec_send_packet(codec_context.get(), packet.get());
          av_packet_unref(packet.get());
          if (send_result == AVERROR(EAGAIN))
            break;
          CheckAv(send_result, "Could not submit movie video packet");
          break;
        }

        av_packet_unref(packet.get());
      }
    }
  }

  void Seek(int time_ms) {
    const std::int64_t target =
        av_rescale_q(time_ms, kMilliseconds, stream->time_base) +
        (stream->start_time == AV_NOPTS_VALUE ? 0 : stream->start_time);
    if (av_seek_frame(format_context.get(), stream_index, target,
                      AVSEEK_FLAG_BACKWARD) >= 0) {
      avcodec_flush_buffers(codec_context.get());
      input_eof = false;
      output_eof = false;
      current_frame_time_ms = -1;
    }
  }

  std::filesystem::path path;
  AvFormatContextPtr format_context;
  AvCodecContextPtr codec_context;
  AvFramePtr frame;
  AvPacketPtr packet;
  SwsContextPtr sws_context;
  AVStream* stream = nullptr;
  int stream_index = -1;
  Info info;
  bool input_eof = false;
  bool output_eof = false;
  int current_frame_time_ms = -1;
};

FfmpegVideoDecoder::FfmpegVideoDecoder(std::filesystem::path path)
    : impl_(std::make_unique<Impl>(std::move(path))) {}

FfmpegVideoDecoder::~FfmpegVideoDecoder() = default;

const FfmpegVideoDecoder::Info& FfmpegVideoDecoder::info() const {
  return impl_->info;
}

int FfmpegVideoDecoder::width() const { return impl_->info.size.width(); }

int FfmpegVideoDecoder::height() const { return impl_->info.size.height(); }

int FfmpegVideoDecoder::total_time() const { return impl_->info.total_time; }

bool FfmpegVideoDecoder::DecodeTime(int time_ms,
                                    std::span<char> bgra,
                                    bool force) {
  if (static_cast<int>(bgra.size()) < width() * height() * 4)
    throw std::runtime_error("Movie video output buffer is too small");

  if (force || (impl_->current_frame_time_ms > time_ms && time_ms >= 0))
    impl_->Seek(std::max(time_ms, 0));

  bool updated = false;
  while (!impl_->output_eof &&
         (impl_->current_frame_time_ms < 0 ||
          impl_->current_frame_time_ms < time_ms)) {
    int decoded_time_ms = 0;
    if (!impl_->DecodeNext(bgra, decoded_time_ms))
      break;
    impl_->current_frame_time_ms = decoded_time_ms;
    updated = true;
    if (impl_->current_frame_time_ms >= time_ms)
      break;
  }

  return updated;
}

bool FfmpegVideoDecoder::IsPlaying(int time_ms) const {
  if (impl_->info.total_time > 0 && time_ms >= impl_->info.total_time)
    return false;
  return !impl_->output_eof;
}

struct FfmpegAudioDecoder::Impl {
  explicit Impl(std::string_view data)
      : reader({.data = data, .position = 0}),
        avio_context(CreateReadAvioContext(reader)) {
    AVFormatContext* raw_format_context = avformat_alloc_context();
    if (!raw_format_context)
      throw std::runtime_error("Could not allocate FFmpeg audio context");
    raw_format_context->pb = avio_context.get();
    raw_format_context->flags |= AVFMT_FLAG_CUSTOM_IO;

    AVFormatContext* opened_context = raw_format_context;
    CheckAv(avformat_open_input(&opened_context, nullptr, nullptr, nullptr),
            "Could not open FFmpeg audio input");
    format_context.reset(opened_context);
    CheckAv(avformat_find_stream_info(format_context.get(), nullptr),
            "Could not read FFmpeg audio stream info");

    const int best_stream = av_find_best_stream(
        format_context.get(), AVMEDIA_TYPE_AUDIO, -1, -1, nullptr, 0);
    CheckAv(best_stream, "Could not find FFmpeg audio stream");
    stream_index = best_stream;
    stream = format_context->streams[stream_index];

    const AVCodec* codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec)
      throw std::runtime_error("No FFmpeg decoder for audio stream");

    codec_context.reset(avcodec_alloc_context3(codec));
    if (!codec_context)
      throw std::runtime_error("Could not allocate FFmpeg audio codec context");
    CheckAv(avcodec_parameters_to_context(codec_context.get(),
                                          stream->codecpar),
            "Could not copy FFmpeg audio parameters");
    CheckAv(avcodec_open2(codec_context.get(), codec, nullptr),
            "Could not open FFmpeg audio decoder");

    spec = SpecFromCodec(codec_context.get());
    frame.reset(av_frame_alloc());
    packet.reset(av_packet_alloc());
    if (!frame || !packet)
      throw std::runtime_error("Could not allocate FFmpeg audio decode buffers");
  }

  void EnsureDecoded() {
    if (next || output_eof)
      return;
    next = DecodeOne();
    if (!next)
      output_eof = true;
  }

  std::optional<AudioData> DecodeOne() {
    while (true) {
      const int receive_result =
          avcodec_receive_frame(codec_context.get(), frame.get());
      if (receive_result == 0) {
        AudioData decoded = ExtractAudioFrame(frame.get(), codec_context.get());
        av_frame_unref(frame.get());
        return decoded;
      }
      if (receive_result == AVERROR_EOF)
        return std::nullopt;
      if (receive_result != AVERROR(EAGAIN))
        CheckAv(receive_result, "Could not decode FFmpeg audio frame");

      while (true) {
        const int read_result = av_read_frame(format_context.get(), packet.get());
        if (read_result == AVERROR_EOF) {
          CheckAv(avcodec_send_packet(codec_context.get(), nullptr),
                  "Could not drain FFmpeg audio decoder");
          input_eof = true;
          break;
        }
        CheckAv(read_result, "Could not read FFmpeg audio packet");

        if (packet->stream_index == stream_index) {
          const int send_result =
              avcodec_send_packet(codec_context.get(), packet.get());
          av_packet_unref(packet.get());
          if (send_result == AVERROR(EAGAIN))
            break;
          CheckAv(send_result, "Could not submit FFmpeg audio packet");
          break;
        }

        av_packet_unref(packet.get());
      }
    }
  }

  BufferReader reader;
  AvioContextPtr avio_context;
  AvFormatContextPtr format_context;
  AvCodecContextPtr codec_context;
  AvFramePtr frame;
  AvPacketPtr packet;
  AVStream* stream = nullptr;
  int stream_index = -1;
  AVSpec spec;
  std::optional<AudioData> next;
  bool input_eof = false;
  bool output_eof = false;
  pcm_count_t frame_position = 0;
};

FfmpegAudioDecoder::FfmpegAudioDecoder(std::string_view data)
    : impl_(std::make_unique<Impl>(data)) {}

FfmpegAudioDecoder::~FfmpegAudioDecoder() = default;

std::string FfmpegAudioDecoder::DecoderName() const {
  return "FfmpegAudioDecoder";
}

AVSpec FfmpegAudioDecoder::GetSpec() { return impl_->spec; }

AudioData FfmpegAudioDecoder::DecodeAll() {
  AudioData result;
  result.spec = GetSpec();
  result.PrepareDatabuf();
  while (HasNext())
    result.Append(DecodeNext());
  return result;
}

AudioData FfmpegAudioDecoder::DecodeNext() {
  if (!HasNext())
    throw std::logic_error("No more FFmpeg audio data to decode.");

  AudioData result = std::move(*impl_->next);
  impl_->next.reset();
  impl_->frame_position +=
      static_cast<pcm_count_t>(result.SampleCount() / result.spec.channel_count);
  return result;
}

bool FfmpegAudioDecoder::HasNext() {
  impl_->EnsureDecoded();
  return impl_->next.has_value();
}

SEEK_RESULT FfmpegAudioDecoder::Seek(pcm_count_t offset, SEEKDIR whence) {
  pcm_count_t target = offset;
  switch (whence) {
    case SEEKDIR::BEG:
      break;
    case SEEKDIR::CUR:
      target += impl_->frame_position;
      break;
    case SEEKDIR::END:
      if (impl_->stream->duration == AV_NOPTS_VALUE)
        return SEEK_RESULT::FAIL;
      target += av_rescale_q(impl_->stream->duration, impl_->stream->time_base,
                             AVRational{1, impl_->spec.sample_rate});
      break;
    default:
      return SEEK_RESULT::FAIL;
  }

  if (target < 0)
    return SEEK_RESULT::FAIL;

  const std::int64_t timestamp =
      av_rescale_q(target, AVRational{1, impl_->spec.sample_rate},
                   impl_->stream->time_base) +
      (impl_->stream->start_time == AV_NOPTS_VALUE ? 0
                                                   : impl_->stream->start_time);
  CheckAv(av_seek_frame(impl_->format_context.get(), impl_->stream_index,
                        timestamp, AVSEEK_FLAG_BACKWARD),
          "Could not seek FFmpeg audio stream");
  avcodec_flush_buffers(impl_->codec_context.get());
  impl_->next.reset();
  impl_->input_eof = false;
  impl_->output_eof = false;
  impl_->frame_position = target;
  return SEEK_RESULT::IMPRECISE_SEEK;
}

FfmpegAudioDecoder::pcm_count_t FfmpegAudioDecoder::Tell() {
  return impl_->frame_position;
}
