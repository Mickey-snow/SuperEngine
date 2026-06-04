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
// -----------------------------------------------------------------------

#include "core/avdec/omv.hpp"

#include <ogg/ogg.h>
#include <theora/theoradec.h>

#include <algorithm>
#include <cstring>
#include <format>
#include <fstream>
#include <memory>
#include <stdexcept>

namespace {

constexpr int kOmvMajorVersion = 1;
constexpr int kOmvMinorVersion = 1;
constexpr std::size_t kPageRecordSize = 28;
constexpr std::size_t kPacketRecordSize = 32;

int ReadI32(const std::vector<unsigned char>& data, std::size_t offset) {
  if (offset + 4 > data.size())
    throw std::runtime_error("OMV: unexpected end of file");

  return static_cast<int>(static_cast<unsigned int>(data[offset]) |
                          (static_cast<unsigned int>(data[offset + 1]) << 8) |
                          (static_cast<unsigned int>(data[offset + 2]) << 16) |
                          (static_cast<unsigned int>(data[offset + 3]) << 24));
}

unsigned char ClampByte(int value) {
  return static_cast<unsigned char>(std::clamp(value, 0, 255));
}

unsigned char PlaneAt(const th_img_plane& plane,
                      int x,
                      int y,
                      int output_width,
                      int output_height) {
  if (!plane.data || plane.width <= 0 || plane.height <= 0)
    return 0;

  const int px = std::clamp(x * plane.width / std::max(output_width, 1), 0,
                            plane.width - 1);
  const int py = std::clamp(y * plane.height / std::max(output_height, 1), 0,
                            plane.height - 1);
  return plane.data[py * plane.stride + px];
}

unsigned char PlaneAtExact(const th_img_plane& plane, int x, int y) {
  if (!plane.data || x < 0 || y < 0 || x >= plane.width || y >= plane.height)
    return 255;
  return plane.data[y * plane.stride + x];
}

struct OggSyncGuard {
  ogg_sync_state sync{};
  OggSyncGuard() { ogg_sync_init(&sync); }
  ~OggSyncGuard() { ogg_sync_clear(&sync); }
};

}  // namespace

struct OmvDecoder::Header {
  int header_size = 0;
  int major_version = 0;
  int minor_version = 0;
  int theora_type = 0;
  int width = 0;
  int height = 0;
  int center_x = 0;
  int center_y = 0;
  int usec_per_frame = 0;
  int theora_serial_no = 0;
  int header_page_no = 0;
  int subheader_page_no = 0;
  int page_count = 0;
  int packet_count = 0;
};

struct OmvDecoder::Page {
  int own_page_no = 0;
  bool is_eos = false;
  bool is_key_page = false;
  int page_size = 0;
  int seek_offset = 0;
  int seek_page_no = 0;
  int packet_count = 0;
  int top_packet_no = 0;
};

struct OmvDecoder::Packet {
  int own_packet_no = 0;
  int own_page_no = 0;
  int own_packet_no_in_page = 0;
  bool is_key_frame = false;
  int key_frame_packet_no = 0;
  int key_frame_page_no = 0;
  int frame_time_start = 0;
  int frame_time_end = 0;
};

struct OmvDecoder::Impl {
  ogg_stream_state stream{};
  bool stream_initialized = false;
  th_info info{};
  th_comment comment{};
  th_dec_ctx* decoder = nullptr;

  Impl() {
    th_info_init(&info);
    th_comment_init(&comment);
  }

  ~Impl() {
    if (decoder)
      th_decode_free(decoder);
    if (stream_initialized)
      ogg_stream_clear(&stream);
    th_comment_clear(&comment);
    th_info_clear(&info);
  }
};

OmvDecoder::OmvDecoder(std::filesystem::path path, bool loop)
    : path_(std::move(path)), loop_(loop) {
  LoadFile();
  ParseHeader();
  InitializeDecoder();
}

OmvDecoder::~OmvDecoder() = default;

void OmvDecoder::LoadFile() {
  std::ifstream file(path_, std::ios::binary);
  if (!file)
    throw std::runtime_error("OMV: failed to open " + path_.string());

  file.seekg(0, std::ios::end);
  const std::streamoff size = file.tellg();
  if (size <= 0)
    throw std::runtime_error("OMV: empty file " + path_.string());
  file.seekg(0, std::ios::beg);

  data_.resize(static_cast<std::size_t>(size));
  if (!file.read(reinterpret_cast<char*>(data_.data()), size))
    throw std::runtime_error("OMV: failed to read " + path_.string());
}

void OmvDecoder::ParseHeader() {
  if (data_.size() < 168)
    throw std::runtime_error("OMV: file too small");

  header_ = std::make_unique<Header>();
  header_->header_size = ReadI32(data_, 0x00);
  header_->major_version = data_.at(0x04);
  header_->minor_version = data_.at(0x05);
  header_->theora_type = ReadI32(data_, 0x28);
  header_->width = ReadI32(data_, 0x2c);
  header_->height = ReadI32(data_, 0x30);
  header_->center_x = ReadI32(data_, 0x34);
  header_->center_y = ReadI32(data_, 0x38);
  header_->usec_per_frame = ReadI32(data_, 0x3c);
  header_->theora_serial_no = ReadI32(data_, 0x40);
  header_->header_page_no = ReadI32(data_, 0x44);
  header_->subheader_page_no = ReadI32(data_, 0x48);
  header_->page_count = ReadI32(data_, 0x4c);
  header_->packet_count = ReadI32(data_, 0x50);

  if (header_->major_version != kOmvMajorVersion ||
      header_->minor_version != kOmvMinorVersion) {
    throw std::runtime_error(std::format("OMV: unsupported version {}.{}",
                                         header_->major_version,
                                         header_->minor_version));
  }
  if (header_->header_size < 168 || header_->width <= 0 ||
      header_->height <= 0 || header_->page_count <= 0 ||
      header_->packet_count <= 0) {
    throw std::runtime_error("OMV: invalid header");
  }

  const std::size_t page_table_offset =
      static_cast<std::size_t>(header_->header_size);
  const std::size_t packet_table_offset =
      page_table_offset +
      static_cast<std::size_t>(header_->page_count) * kPageRecordSize;
  data_start_ =
      packet_table_offset +
      static_cast<std::size_t>(header_->packet_count) * kPacketRecordSize;
  if (data_start_ > data_.size())
    throw std::runtime_error("OMV: invalid table sizes");

  pages_.reserve(header_->page_count);
  for (int i = 0; i < header_->page_count; ++i) {
    const std::size_t off = page_table_offset + i * kPageRecordSize;
    Page page;
    page.own_page_no = ReadI32(data_, off + 0);
    page.is_eos = data_.at(off + 4) != 0;
    page.is_key_page = data_.at(off + 5) != 0;
    page.page_size = ReadI32(data_, off + 8);
    page.seek_offset = ReadI32(data_, off + 12);
    page.seek_page_no = ReadI32(data_, off + 16);
    page.packet_count = ReadI32(data_, off + 20);
    page.top_packet_no = ReadI32(data_, off + 24);
    pages_.emplace_back(page);
  }

  packets_.reserve(header_->packet_count);
  for (int i = 0; i < header_->packet_count; ++i) {
    const std::size_t off = packet_table_offset + i * kPacketRecordSize;
    Packet packet;
    packet.own_packet_no = ReadI32(data_, off + 0);
    packet.own_page_no = ReadI32(data_, off + 4);
    packet.own_packet_no_in_page = ReadI32(data_, off + 8);
    packet.is_key_frame = data_.at(off + 12) != 0;
    packet.key_frame_packet_no = ReadI32(data_, off + 16);
    packet.key_frame_page_no = ReadI32(data_, off + 20);
    packet.frame_time_start = ReadI32(data_, off + 24);
    packet.frame_time_end = ReadI32(data_, off + 28);
    packets_.emplace_back(packet);
  }

  info_.type = header_->theora_type;
  info_.size = Size(header_->width, header_->height);
  info_.center = Point(header_->center_x, header_->center_y);
  info_.frame_count = header_->packet_count;
  info_.usec_per_frame = header_->usec_per_frame;
  info_.total_time = packets_.empty() ? 0 : packets_.back().frame_time_end;
}

void OmvDecoder::InitializeDecoder() {
  impl_ = std::make_unique<Impl>();
  if (ogg_stream_init(&impl_->stream, header_->theora_serial_no) != 0)
    throw std::runtime_error("OMV: failed to initialize Ogg stream");
  impl_->stream_initialized = true;

  th_setup_info* setup = nullptr;
  try {
    ogg_packet packet;
    if (ReadPage(header_->header_page_no) < 0)
      throw std::runtime_error("OMV: failed to read Theora header page");
    if (ogg_stream_packetout(&impl_->stream, &packet) <= 0)
      throw std::runtime_error("OMV: missing Theora header packet");
    if (th_decode_headerin(&impl_->info, &impl_->comment, &setup, &packet) < 0)
      throw std::runtime_error("OMV: invalid Theora header packet");
    EmptyStream();

    if (ReadPage(header_->subheader_page_no) < 0)
      throw std::runtime_error("OMV: failed to read Theora subheader page");
    for (int i = 0; i < 2; ++i) {
      if (ogg_stream_packetout(&impl_->stream, &packet) <= 0)
        throw std::runtime_error("OMV: missing Theora subheader packet");
      if (th_decode_headerin(&impl_->info, &impl_->comment, &setup, &packet) <
          0) {
        throw std::runtime_error("OMV: invalid Theora subheader packet");
      }
    }
    EmptyStream();

    impl_->decoder = th_decode_alloc(&impl_->info, setup);
    if (!impl_->decoder)
      throw std::runtime_error("OMV: failed to allocate Theora decoder");

    int pp_level = 0;
    th_decode_ctl(impl_->decoder, TH_DECCTL_SET_PPLEVEL, &pp_level,
                  sizeof(pp_level));
  } catch (...) {
    if (setup)
      th_setup_free(setup);
    throw;
  }

  if (setup)
    th_setup_free(setup);
}

int OmvDecoder::ReadPage(int page_no) {
  if (page_no < 0 || page_no >= static_cast<int>(pages_.size()))
    throw std::runtime_error("OMV: page index out of range");

  const Page& page = pages_[page_no];
  if (page.page_size <= 0)
    throw std::runtime_error("OMV: empty page");

  const std::size_t page_offset =
      data_start_ + static_cast<std::size_t>(page.seek_offset);
  const std::size_t page_size = static_cast<std::size_t>(page.page_size);
  if (page_offset + page_size > data_.size())
    throw std::runtime_error("OMV: page points past end of file");

  OggSyncGuard sync;
  char* syncbuf = ogg_sync_buffer(&sync.sync, page.page_size);
  if (!syncbuf)
    throw std::runtime_error("OMV: failed to allocate Ogg sync buffer");

  std::memcpy(syncbuf, data_.data() + page_offset, page_size);
  if (ogg_sync_wrote(&sync.sync, page.page_size) < 0)
    throw std::runtime_error("OMV: failed to commit Ogg sync buffer");

  ogg_page oggpage;
  if (ogg_sync_pageout(&sync.sync, &oggpage) != 1)
    throw std::runtime_error("OMV: failed to read Ogg page");

  if (ogg_stream_pagein(&impl_->stream, &oggpage) < 0)
    throw std::runtime_error("OMV: failed to submit Ogg page");

  return page.is_eos ? 1 : 0;
}

void OmvDecoder::EmptyStream() {
  ogg_packet packet;
  while (ogg_stream_packetout(&impl_->stream, &packet) > 0) {
  }
}

void OmvDecoder::CheckReady() const {
  if (!impl_ || !impl_->decoder || pages_.empty() || packets_.empty())
    throw std::runtime_error("OMV: decoder is not ready");
}

void OmvDecoder::UpdateTimeOnly(int now_time) {
  end_of_theora_ = now_time >= info_.total_time;
}

int OmvDecoder::TimeToFrame(int now_time, bool set_end) {
  int frame_no = -1;

  if (packets_.empty())
    return 0;

  if (now_time <= 0) {
    now_time = 0;
    frame_no = 0;
    end_of_theora_ = false;
  } else if (info_.total_time > 0 && now_time >= info_.total_time) {
    if (loop_) {
      now_time %= info_.total_time;
    } else {
      frame_no = static_cast<int>(packets_.size()) - 1;
      if (set_end)
        end_of_theora_ = true;
    }
  } else {
    end_of_theora_ = false;
  }

  if (frame_no != -1)
    return frame_no;

  if (now_frame_no_ >= 0 && now_frame_no_ < static_cast<int>(packets_.size())) {
    for (int i = now_frame_no_; i < static_cast<int>(packets_.size()); ++i) {
      const Packet& packet = packets_[i];
      if (now_time <= packet.frame_time_end) {
        if (now_time >= packet.frame_time_start)
          return i;
        break;
      }
    }
  }

  for (int i = 0; i < static_cast<int>(packets_.size()); ++i) {
    if (now_time <= packets_[i].frame_time_end)
      return i;
  }

  return static_cast<int>(packets_.size()) - 1;
}

bool OmvDecoder::CheckNeedUpdate(int now_time, int* frame_no, bool force) {
  CheckReady();
  const int next_frame = TimeToFrame(now_time, true);
  if (!force && next_frame == now_frame_no_)
    return false;
  if (frame_no)
    *frame_no = next_frame;
  return true;
}

bool OmvDecoder::DecodeTime(int now_time, std::span<char> bgra, bool force) {
  const int frame_no = TimeToFrame(now_time, true);
  return DecodeFrame(frame_no, bgra, force);
}

bool OmvDecoder::DecodeFrame(int frame_no, std::span<char> bgra, bool force) {
  CheckReady();
  const std::size_t expected_size =
      static_cast<std::size_t>(width()) * height() * 4;
  if (bgra.size() != expected_size)
    throw std::runtime_error("OMV: output buffer size mismatch");

  if (!force && frame_no == now_frame_no_)
    return false;

  DecodeFrameInternal(frame_no, force);
  WriteVideo(bgra);
  return true;
}

void OmvDecoder::DecodeFrameInternal(int frame_no, bool force) {
  frame_no = std::clamp(frame_no, 0, static_cast<int>(packets_.size()) - 1);
  if (frame_no == static_cast<int>(packets_.size()) - 1 && !loop_)
    end_of_theora_ = true;

  if (now_page_no_ == -1 || now_frame_no_ == -1)
    force = true;
  else if (frame_no < now_frame_no_)
    force = true;
  else {
    const Packet& current = packets_[now_frame_no_];
    const Packet& target = packets_[frame_no];
    if (current.key_frame_packet_no != target.key_frame_packet_no) {
      const Page& key_page = pages_.at(target.key_frame_page_no);
      if (current.own_page_no < key_page.seek_page_no)
        force = true;
    }
  }

  const Packet& target_packet = packets_[frame_no];
  const int key_frame_packet_no = target_packet.key_frame_packet_no;
  int skip_for_key_frame_no = -1;
  bool eos = false;
  int decode_packet_no = 0;
  int read_page_no = 0;
  int page_cursor = 0;

  if (force) {
    ogg_stream_reset(&impl_->stream);

    const Page& key_page = pages_.at(target_packet.key_frame_page_no);
    read_page_no = key_page.seek_page_no;
    while (true) {
      const int result = ReadPage(read_page_no);
      if (result < 0)
        throw std::runtime_error("OMV: failed to read keyframe page");
      if (read_page_no == key_page.own_page_no)
        break;
      EmptyStream();
      ++read_page_no;
    }

    skip_for_key_frame_no = key_frame_packet_no;
    eos = key_page.is_eos;
    decode_packet_no = key_page.top_packet_no;
    read_page_no = key_page.own_page_no + 1;
    page_cursor = read_page_no;
  } else {
    const Packet& current = packets_[now_frame_no_];
    const Page& current_page = pages_.at(current.own_page_no);
    if (current.key_frame_packet_no != key_frame_packet_no)
      skip_for_key_frame_no = key_frame_packet_no;
    eos = current_page.is_eos;
    decode_packet_no = now_frame_no_ + 1;
    read_page_no = current.own_page_no + 1;
    page_cursor = read_page_no;
  }

  ogg_packet packet;
  if (skip_for_key_frame_no != -1 &&
      decode_packet_no != skip_for_key_frame_no) {
    while (true) {
      while (decode_packet_no != skip_for_key_frame_no) {
        if (ogg_stream_packetout(&impl_->stream, &packet) <= 0)
          break;
        ++decode_packet_no;
      }
      if (decode_packet_no == skip_for_key_frame_no)
        break;

      const int result = ReadPage(read_page_no);
      if (result < 0)
        throw std::runtime_error("OMV: failed to skip to keyframe");
      if (pages_.at(page_cursor).is_eos)
        eos = true;
      ++read_page_no;
      ++page_cursor;
    }
  }

  bool done = false;
  ogg_int64_t granulepos = 0;
  while (!done) {
    while (true) {
      const int packet_result = ogg_stream_packetout(&impl_->stream, &packet);
      if (packet_result <= 0) {
        done = eos;
        break;
      }

      const int decode_result =
          th_decode_packetin(impl_->decoder, &packet, &granulepos);
      if (decode_result < 0 && decode_result != TH_DUPFRAME) {
        throw std::runtime_error("OMV: failed to decode Theora packet");
      }

      if (decode_packet_no == frame_no) {
        done = true;
        break;
      }

      ++decode_packet_no;
    }

    if (done)
      break;

    const int result = ReadPage(read_page_no);
    if (result < 0)
      throw std::runtime_error("OMV: failed to read decode page");
    if (pages_.at(page_cursor).is_eos)
      eos = true;
    ++read_page_no;
    ++page_cursor;
  }

  now_page_no_ = target_packet.own_page_no;
  now_frame_no_ = frame_no;
}

void OmvDecoder::WriteVideo(std::span<char> bgra) {
  th_ycbcr_buffer yuv;
  th_decode_ycbcr_out(impl_->decoder, yuv);

  const int pic_w = width();
  const int pic_h = height();
  auto* dst = reinterpret_cast<unsigned char*>(bgra.data());

  if (info_.type == kTypeRgb) {
    for (int y = 0; y < pic_h; ++y) {
      for (int x = 0; x < pic_w; ++x) {
        const std::size_t idx = (static_cast<std::size_t>(y) * pic_w + x) * 4;
        dst[idx + 0] = PlaneAt(yuv[0], x, y, pic_w, pic_h);
        dst[idx + 1] = PlaneAt(yuv[1], x, y, pic_w, pic_h);
        dst[idx + 2] = PlaneAt(yuv[2], x, y, pic_w, pic_h);
        dst[idx + 3] = 255;
      }
    }
    return;
  }

  if (info_.type == kTypeYuv) {
    for (int y = 0; y < pic_h; ++y) {
      for (int x = 0; x < pic_w; ++x) {
        const int cy = PlaneAt(yuv[0], x, y, pic_w, pic_h);
        const int cu = PlaneAt(yuv[1], x, y, pic_w, pic_h);
        const int cv = PlaneAt(yuv[2], x, y, pic_w, pic_h);
        const std::size_t idx = (static_cast<std::size_t>(y) * pic_w + x) * 4;
        dst[idx + 2] = ClampByte(cy + 1.40200 * (cv - 128));
        dst[idx + 1] =
            ClampByte(cy - 0.34414 * (cu - 128) - 0.71414 * (cv - 128));
        dst[idx + 0] = ClampByte(cy + 1.77200 * (cu - 128));
        dst[idx + 3] = 255;
      }
    }
    return;
  }

  if (info_.type == kTypeRgba) {
    const int alpha_h = (pic_h + 2) / 3;
    const int alpha_h_2 = alpha_h * 2;

    for (int y = 0; y < pic_h; ++y) {
      for (int x = 0; x < pic_w; ++x) {
        unsigned char alpha = 255;
        if (y < alpha_h)
          alpha = PlaneAtExact(yuv[0], x, pic_h + y);
        else if (y < alpha_h_2)
          alpha = PlaneAtExact(yuv[1], x, pic_h + y - alpha_h);
        else
          alpha = PlaneAtExact(yuv[2], x, pic_h + y - alpha_h_2);

        const std::size_t idx = (static_cast<std::size_t>(y) * pic_w + x) * 4;
        dst[idx + 0] = PlaneAt(yuv[0], x, y, pic_w, pic_h);
        dst[idx + 1] = PlaneAt(yuv[1], x, y, pic_w, pic_h);
        dst[idx + 2] = PlaneAt(yuv[2], x, y, pic_w, pic_h);
        dst[idx + 3] = alpha;
      }
    }
    return;
  }

  throw std::runtime_error("OMV: unsupported Theora pixel type");
}

void OmvDecoder::EndLoop() { loop_ = false; }

bool OmvDecoder::IsPlaying() const {
  return impl_ && impl_->decoder && !end_of_theora_;
}
