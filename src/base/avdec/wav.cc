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

#include "base/avdec/wav.h"

#include "utilities/byte_reader.h"
#include "utilities/bytestream.h"

#include <sstream>
#include <stdexcept>

std::vector<uint8_t> MakeRiffHeader(AVSpec spec, size_t data_cksize) {
  const int sample_width = Bytecount(spec.sample_format);

  oBytestream hdr;
  hdr << "RIFF" << static_cast<int32_t>(4 + 24 + 8 + data_cksize) << "WAVE"
      << "fmt ";
  hdr << (int32_t)16;  // fmt chunk size
  hdr << (int16_t)1;
  hdr << (int16_t)spec.channel_count;
  hdr << (int32_t)spec.sample_rate;
  hdr << (int32_t)(spec.sample_rate * spec.channel_count * sample_width);
  hdr << (int16_t)(spec.channel_count * sample_width);
  hdr << (int16_t)(8 * sample_width);
  hdr << "data" << static_cast<int32_t>(data_cksize);
  return hdr.GetCopy();
}

std::vector<uint8_t> EncodeWav(AudioData audio) {
  auto [begin, size] = std::visit(
      [](const auto& it) {
        auto begin = reinterpret_cast<const char*>(it.data()),
             end = reinterpret_cast<const char*>(it.data() + it.size());
        return std::make_pair(begin, std::distance(begin, end));
      },
      audio.data);

  auto result = MakeRiffHeader(audio.spec, size);
  result.resize(result.size() + size);
  std::copy_n(begin, size, result.begin() + 44);
  return result;
}

// -----------------------------------------------------------------------
// template class WavDataExtractor
// -----------------------------------------------------------------------

template <typename sample_t>
class WavDataExtractor {
 public:
  WavDataExtractor(std::string_view data) : data_(data) {}

  std::vector<sample_t> Extract() {
    std::vector<sample_t> result;
    static constexpr int sample_width = sizeof(sample_t);

    ByteReader reader(data_.data(), data_.length());
    while (reader.Position() < reader.Size()) {
      result.push_back(reader.PopAs<sample_t>(sample_width));
    }

    return result;
  }

 private:
  std::string_view data_;
};
template class WavDataExtractor<avsample_u8_t>;
template class WavDataExtractor<avsample_s16_t>;
template class WavDataExtractor<avsample_s32_t>;
template class WavDataExtractor<avsample_s64_t>;

// -----------------------------------------------------------------------
// class WavDecoder
// -----------------------------------------------------------------------

WavDecoder::WavDecoder(std::string_view sv) : wavdata_(sv), fmt_(nullptr) {
  ValidateWav();
  ParseChunks();
}

AudioData WavDecoder::DecodeAll() {
  AudioData result;
  result.spec = {.sample_rate = fmt_->nSamplesPerSec,
                 .sample_format = AV_SAMPLE_FMT::NONE,
                 .channel_count = fmt_->nChannels};
  switch (fmt_->wBitsPerSample) {
    case 8u:
      result.spec.sample_format = AV_SAMPLE_FMT::U8;
      result.data = WavDataExtractor<avsample_u8_t>(data_).Extract();
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
    default:
      throw std::logic_error("Unsupported sample format");
  }
  return result;
}

void WavDecoder::ValidateWav() {
  static constexpr size_t MIN_WAV_HEADER_SIZE = 44;
  if (wavdata_.size() < MIN_WAV_HEADER_SIZE) {
    throw std::logic_error("Invalid WAV data: too small");
  }

  ByteReader reader(wavdata_.data(), 44);
  std::ostringstream oss;
  auto riff_tag = reader.ReadAs<std::string_view>(4);
  reader.Seek(8);
  auto wave_tag = reader.ReadAs<std::string_view>(4);
  if (riff_tag != "RIFF" || wave_tag != "WAVE") {
    oss << "Invalid format in RIFF header\n";
  }

  reader.Seek(4);
  size_t wav_size = 8 + reader.ReadBytes(4);
  if (wav_size > wavdata_.size()) {
    oss << "File size mismatch\n";
  } else{
    wavdata_ = wavdata_.substr(0, wav_size);
  }

  std::string errors = oss.str();
  if (!errors.empty()) {
    throw std::logic_error(errors);
  }
}

void WavDecoder::ParseChunks() {
  ByteReader reader(wavdata_.data(), wavdata_.size());
  reader.Seek(12);
  while (reader.Position() < reader.Size()) {
    auto chunk_tag = reader.PopAs<std::string_view>(4);
    auto chunk_len = reader.PopAs<int>(4);

    if (chunk_tag == "fmt ") {
      if (fmt_ != nullptr)
        throw std::logic_error("Found more than one fmt chunk");

      fmt_ = reinterpret_cast<const fmtHeader*>(reader.Ptr());

      if (chunk_len != 16) {
        if (chunk_len != 18 || fmt_->cbSize != 0)
          throw std::logic_error("Invalid fmt header");
      }
    } else if (chunk_tag == "data") {
      if (fmt_ == nullptr) {
        throw std::logic_error("Found data chunk before fmt chunk");
      }
      data_ = reader.ReadAs<std::string_view>(chunk_len);
    } else {
      /*uninteresting chunk*/
    }

    reader.Proceed(chunk_len);
  }

  if (fmt_ == nullptr) {
    throw std::logic_error("No fmt chunk found");
  }
  if (data_.empty()) {
    throw std::logic_error("No data chunk found");
  }
}
