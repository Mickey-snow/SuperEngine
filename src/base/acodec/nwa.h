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

#ifndef BASE_ACODEC_NWA_H_
#define BASE_ACODEC_NWA_H_

#include <memory>
#include <sstream>
#include <string_view>
#include <vector>

#include "utilities/bitstream.h"

struct NwaHeader {
  int channels : 16;
  int bitsPerSample : 16;
  int samplesPerSec : 32;
  int compressionMode : 32;
  int zeroMode : 32;
  int unitCount : 32;
  int origSize : 32;
  int packedSize : 32;
  int samplePerChannel : 32;
  int samplePerUnit : 32;
  int lastUnitSamples : 32;
  int lastUnitPackedSize : 32;
};

class NwaDecoderImpl {
 public:
  virtual ~NwaDecoderImpl() = default;

  using PcmStream_t = std::vector<float>;

  virtual bool HasNext() { return false; }  // unused
  virtual PcmStream_t DecodeNext() = 0;
  virtual PcmStream_t DecodeAll() = 0;
};

class NwaHQDecoder : public NwaDecoderImpl {
 public:
  NwaHQDecoder(std::string_view data);
  NwaHQDecoder(char* data, size_t length);
  ~NwaHQDecoder() = default;

  std::vector<float> DecodeAll() override;
  std::vector<float> DecodeNext() override { return DecodeAll(); }

 private:
  void ReadHeader();
  void CheckHeader();

  std::string_view sv_data_;
  NwaHeader* hdr_;
  int16_t* stream_;
};

class NwaCompDecoder : public NwaDecoderImpl {
 public:
  NwaCompDecoder(std::string_view data) : data_(data) {
    hdr_ = (NwaHeader*)data_.data();
    offset_count_ = hdr_->unitCount;
    offset_table_ = (int*)(data_.data() + sizeof(NwaHeader));
    current_unit_ = 0;
  }
  NwaCompDecoder(const char* data, size_t size)
      : NwaCompDecoder(std::string_view(data, size)) {}

  std::vector<float> DecodeAll() override {
    std::vector<float> ret;
    ret.reserve(hdr_->samplePerUnit * hdr_->unitCount);
    while (current_unit_ < offset_count_) {
      auto samples = DecodeNext();
      ret.insert(ret.end(), samples.begin(), samples.end());
    }
    return ret;
  }

  std::vector<float> DecodeNext() override {
    if (current_unit_ >= offset_count_)
      throw std::logic_error(
          "DecodeNext() called when no more data is available for decoding.");
    return DecodeUnit(current_unit_++);
  }
  std::vector<float> DecodeUnit(int id) {
    int begin_pos = offset_table_[id],
        unit_size = id == (offset_count_ - 1)
                        ? hdr_->lastUnitPackedSize
                        : (offset_table_[id + 1] - offset_table_[id]);
    std::string_view unit_data = data_.substr(begin_pos, unit_size);
    BitStream reader(unit_data.data(), unit_data.size());

    std::vector<float> ret;
    auto yieldSample = [&ret](int sample) {
      ret.push_back(1.0 * sample / 32767);
    };

    int sample[2] = {}, channel = 0;
    sample[0] = reader.PopAs<int16_t>(16);
    if (hdr_->channels == 2)
      sample[1] = reader.PopAs<int16_t>(16);
    const int comp = hdr_->compressionMode;

    auto ReadSM = [](int value, int bits) {
      bool sign = (value >> (bits - 1)) & 1;
      int magnitude = value & ((1 << ((bits - 1))) - 1);
      return (sign ? -1 : 1) * magnitude;
    };

    const int unit_sample_count =
        id == hdr_->unitCount - 1 ? hdr_->lastUnitSamples : hdr_->samplePerUnit;
    while (ret.size() < unit_sample_count) {
      if (reader.Position() >= reader.Size()) {
        std::ostringstream oss;
        oss << "Data section length mismatch. (section id " << id;
        oss << ", from=" << begin_pos;
        oss << " size=" << unit_size;
        throw std::runtime_error(oss.str());
      }

      unsigned int type = reader.Popbits(3);
      do {
        if (type == 0) {
          if (hdr_->zeroMode) {
            int zeros = reader.Popbits(1);
            if (zeros == 0b1)
              zeros = reader.Popbits(2);
            if (zeros == 0b11)
              zeros = reader.Popbits(8);

            while (zeros--)
              yieldSample(sample[channel]);
            break;
          }
        } else if (1 <= type && type <= 6) {
          int bits = comp >= 3 ? 3 + comp : 5 - comp;
          int shift = comp >= 3 ? 1 + type : 2 + type + comp;
          int base = ReadSM(reader.Popbits(bits), bits);
          sample[channel] += base << shift;
        } else if (type == 7) {
          bool flag = reader.Popbits(1);
          if (flag)
            sample[channel] = 0;
          else {
            int bits = comp >= 3 ? 8 : 8 - comp;
            int shift = comp >= 3 ? 9 : 9 + comp;
            int base = ReadSM(reader.Popbits(bits), bits);
            sample[channel] += base << shift;
          }
        }

        yieldSample(sample[channel]);
      } while (false);

      if (hdr_->channels == 2)
        channel ^= 1;
    }

    return ret;
  }

 private:
  std::string_view data_;
  NwaHeader* hdr_;
  int offset_count_;
  int* offset_table_;
  int current_unit_;
};

class NwaDecoder {
 public:
  NwaDecoder(std::string_view data)
      : data_(data), hdr_((NwaHeader*)data.data()) {
    if (hdr_->compressionMode == -1)
      impl_ = std::make_unique<NwaHQDecoder>(data_);
    else
      impl_ = std::make_unique<NwaCompDecoder>(data_);
  }

  NwaDecoder(char* data, size_t length)
      : NwaDecoder(std::string_view(data, length)) {}

  using PcmStream_t = std::vector<float>;

  PcmStream_t DecodeNext() { return impl_->DecodeNext(); }
  PcmStream_t DecodeAll() { return impl_->DecodeAll(); }
  bool HasNext() { return impl_->HasNext(); }

 private:
  std::string_view data_;
  const NwaHeader* hdr_;
  std::unique_ptr<NwaDecoderImpl> impl_;
};

#endif
