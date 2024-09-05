// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2009 Elliot Glaysher
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
//
// Parts of this file have been copied from koedec_ogg.cc in the xclannad
// distribution and they are:
//
// Copyright (c) 2004-2006  Kazunori "jagarl" Ueno
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products
//    derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// -----------------------------------------------------------------------

#include "base/avdec/ogg.h"

#include <vorbis/vorbisfile.h>

#include <algorithm>
#include <cstring>
#include <stdexcept>
#include <string_view>
#include <type_traits>

std::string oggErrorCodeToString(int code) {
  switch (code) {
    case OV_FALSE:
      return "Not true, or no data available";
    case OV_HOLE:
      return "Vorbisfile encountered missing or corrupt data in the bitstream. "
             "Recovery is normally automatic and this return code is for "
             "informational purposes only.";
    case OV_EREAD:
      return "Read error while fetching compressed data for decode";
    case OV_EFAULT:
      return "Internal inconsistency in decode state. Continuing is likely "
             "not possible.";
    case OV_EIMPL:
      return "Feature not implemented";
    case OV_EINVAL:
      return "Either an invalid argument, or incompletely initialized argument "
             "passed to libvorbisfile call";
    case OV_ENOTVORBIS:
      return "The given file/data was not recognized as Ogg Vorbis data.";
    case OV_EBADHEADER:
      return "The file/data is apparently an Ogg Vorbis stream, but contains a "
             "corrupted or undecipherable header.";
    case OV_EVERSION:
      return "The bitstream format revision of the given stream is not "
             "supported.";
    case OV_EBADLINK:
      return "The given link exists in the Vorbis data stream, but is not "
             "decipherable due to garbage or corruption.";
    case OV_ENOSEEK:
      return "The given stream is not seekable";
    default:
      return "Unknown error";
  }
}

// -----------------------------------------------------------------------
// class ov_adapter
// -----------------------------------------------------------------------

class ov_adapter {
 public:
  ov_adapter(std::string_view sv) : data_(sv), current_(0) {
    ov_callbacks callback;
    callback.read_func = ov_adapter::ov_readfunc;
    callback.seek_func = ov_adapter::ov_seekfunc;
    callback.close_func = nullptr;
    callback.tell_func = ov_adapter::ov_tellfunc;

    if (int r = ov_open_callbacks(this, &vf, nullptr, 0, callback)) {
      throw std::runtime_error("Ogg stream error: " + oggErrorCodeToString(r));
    }
  }

  ~ov_adapter() { ov_clear(&vf); }

  ov_adapter(const ov_adapter&) = delete;
  ov_adapter& operator=(const ov_adapter&) = delete;

  static size_t ov_readfunc(void* dst,
                            size_t size,
                            size_t nmemb,
                            void* datasource) {
    ov_adapter* me = static_cast<ov_adapter*>(datasource);
    long long next = me->current_ + size * nmemb;
    if (next > me->data_.size()) {
      nmemb = (me->data_.size() - me->current_) / size;
      next = me->current_ + size * nmemb;
    }
    std::memcpy(dst, me->data_.data() + me->current_, size * nmemb);
    me->current_ = next;
    return nmemb;
  }

  static int ov_seekfunc(void* datasource, ogg_int64_t offset, int whence) {
    ov_adapter* me = static_cast<ov_adapter*>(datasource);
    long long pt = 0;

    if (whence == SEEK_SET)
      pt = offset;
    else if (whence == SEEK_CUR)
      pt = me->current_ + offset;
    else if (whence == SEEK_END)
      pt = me->data_.size() + offset;

    if (pt > me->data_.size() || pt < 0)
      return -1;
    me->current_ = pt;
    return 0;
  }

  static long ov_tellfunc(void* datasource) {
    ov_adapter* me = static_cast<ov_adapter*>(datasource);
    return static_cast<long>(me->current_);
  }

  std::string_view data_;
  long long current_;
  OggVorbis_File vf;
};

// -----------------------------------------------------------------------
// class OggDecoder
// -----------------------------------------------------------------------

OggDecoder::OggDecoder(std::string_view sv)
    : impl_(std::make_unique<ov_adapter>(sv)) {}

OggDecoder::~OggDecoder() = default;

std::string OggDecoder::DecoderName() const { return "OggDecoder"; }

AVSpec OggDecoder::GetSpec() {
  vorbis_info* vinfo = ov_info(&impl_->vf, -1);
  return {.sample_rate = static_cast<int>(vinfo->rate),
          .sample_format = AV_SAMPLE_FMT::S16,
          .channel_count = vinfo->channels};
}

AudioData OggDecoder::DecodeNext() {
  AudioData result;
  result.spec = GetSpec();
  std::vector<avsample_s16_t> samples;

  static constexpr size_t batch_size = 8192;
  static constexpr size_t sample_size = sizeof(avsample_s16_t);
  samples.resize(batch_size * sample_size);
  int bytes = ov_read(&impl_->vf, reinterpret_cast<char*>(samples.data()),
                      sample_size * samples.size(), 0, 2, 1, nullptr);
  if (bytes < 0) {
    throw std::runtime_error("Error decoding Ogg stream.");
  }

  samples.resize(bytes / sample_size);
  result.data = std::move(samples);
  return result;
}

AudioData OggDecoder::DecodeAll() {
  AudioData result;
  result.spec = GetSpec();
  std::vector<avsample_s16_t> samples;

  bool has_next = true;
  while (has_next) {
    auto next_chunk = DecodeNext();
    has_next = std::visit(
        [&samples](auto& data) -> bool {
          if (data.empty())
            return false;
          samples.insert(samples.end(), data.begin(), data.end());
          return true;
        },
        next_chunk.data);
  }

  result.data = std::move(samples);
  return result;
}

bool OggDecoder::HasNext() { return Tell() < PcmTotal(); }

using pcm_count_t = typename OggDecoder::pcm_count_t;
pcm_count_t OggDecoder::PcmTotal() const {
  return ov_pcm_total(&impl_->vf, -1);
}

pcm_count_t OggDecoder::Tell() { return ov_pcm_tell(&impl_->vf); }

SEEK_RESULT OggDecoder::Seek(long long offset, SEEKDIR whence) {
  long long pos = 0;
  switch (whence) {
    case SEEKDIR::CUR:
      pos = offset + Tell();
      break;
    case SEEKDIR::BEG:
      pos = offset;
      break;
    case SEEKDIR::END:
      pos = offset + PcmTotal();
      break;
  }

  int result = ov_pcm_seek(&impl_->vf, pos);
  if (result != 0)
    throw std::runtime_error("Error seeking to sample position " +
                             std::to_string(pos));

  return SEEK_RESULT::PRECISE_SEEK;
}
