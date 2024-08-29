// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2024 Serina Sakurai
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

#ifndef SRC_BASE_AVDEC_WAV_H_
#define SRC_BASE_AVDEC_WAV_H_

#include "base/avspec.h"
#include "libreallive/alldefs.h"
#include "utilities/bitstream.h"

using libreallive::read_i16;
using libreallive::read_i32;

#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>

struct fmtHeader {
  uint16_t wFormatTag;
  int16_t nChannels;
  int32_t nSamplesPerSec;
  int32_t nAvgBytesPerSec;
  int16_t nBlockAlign;
  int16_t wBitsPerSample;
  uint16_t cbSize;
};

template <typename sample_t>
class WavDataExtractor {
 public:
  WavDataExtractor(std::string_view data) : data_(data) {}

  std::vector<sample_t> Extract() {
    std::vector<sample_t> result;
    static constexpr int sample_width = 8 * sizeof(sample_t);

    // TODO: switch to a faster byte reader
    BitStream reader(data_.data(), data_.length());
    while (reader.Position() < reader.Length())
      result.push_back(reader.PopAs<sample_t>(sample_width));

    return result;
  }

  std::string_view data_;
};

class WavDecoder {
 public:
  WavDecoder(std::string_view sv) : wavdata_(sv), fmt_(nullptr) {
    ValidateWav();
    ParseChunks();
  }
  ~WavDecoder() = default;

  AudioData DecodeAll() {
    AudioData result;
    result.spec = {.sample_rate = fmt_->nSamplesPerSec,
                   .sample_format = AV_SAMPLE_FMT::NONE,
                   .channel_count = fmt_->nChannels};
    switch (fmt_->wBitsPerSample) {
      case 8u:
        result.spec.sample_format = AV_SAMPLE_FMT::S8;
        result.data = WavDataExtractor<avsample_s8_t>(data_).Extract();
        break;
      case 16u:
        result.spec.sample_format = AV_SAMPLE_FMT::S16;
        result.data = WavDataExtractor<avsample_s16_t>(data_).Extract();
        break;
      case 32u:
        result.spec.sample_format = AV_SAMPLE_FMT::S32;
        result.data = WavDataExtractor<avsample_s32_t>(data_).Extract();
        break;
      case 64u:
        result.spec.sample_format = AV_SAMPLE_FMT::S64;
        result.data = WavDataExtractor<avsample_s64_t>(data_).Extract();
        break;
    }
    return result;
  }

 private:
  std::string_view wavdata_;
  fmtHeader* fmt_;
  std::string_view data_;

  void ValidateWav() {
    if (wavdata_.size() < 44)
      throw std::logic_error("invalid wav data");

    const char* stream = wavdata_.data();
    std::ostringstream oss;
    std::string_view riff_tag(stream, 4), wave_tag(stream + 8, 4);
    if (riff_tag != "RIFF" || wave_tag != "WAVE")
      oss << "invalid format in riff header\n";

    stream += 4;
    if (8 + read_i32(stream) != wavdata_.size())
      oss << "File size mismatch\n";

    std::string errors = oss.str();
    if (!errors.empty())
      throw std::logic_error(errors);
  }

  void ParseChunks() {
    const char* stream = wavdata_.data() + 12;
    while (stream < wavdata_.data() + wavdata_.size()) {
      std::string_view chunk_tag(stream, 4);
      stream += 4;
      int chunk_len = read_i32(stream);
      stream += 4;

      if (chunk_tag == "fmt ") {
        if (fmt_ != nullptr)
          throw std::logic_error("found more than one fmt chunk");

        fmt_ = (fmtHeader*)stream;
        // todo: validate fmt header

        if (chunk_len != 16)
          if (chunk_len != 18 || fmt_->cbSize != 0)
            throw std::logic_error("invalid fmt header");
      } else if (chunk_tag == "data") {
        if (fmt_ == nullptr)
          throw std::logic_error("found data chunk before fmt");
        data_ = std::string_view(stream, chunk_len);
      } else {
        /*uninteresting chunk*/
      }

      stream += chunk_len;
    }
    if (stream != wavdata_.data() + wavdata_.size())
      throw std::logic_error("invalid final chunk");

    if (fmt_ == nullptr)
      throw std::logic_error("no fmt chunk found");
    if (data_.empty())
      throw std::logic_error("no data chunk found");
  }
};

#endif
