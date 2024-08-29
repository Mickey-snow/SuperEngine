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

#include "base/avdec/nwa.h"

#include <algorithm>
#include <sstream>

// -----------------------------------------------------------------------
// class NwaHQDecoder
// -----------------------------------------------------------------------
class NwaHQDecoder : public NwaDecoderImpl {
 public:
  NwaHQDecoder(std::string_view data) : data_(data) {
    header_ = reinterpret_cast<const NwaHeader*>(data_.data());
    sample_stream_ = reinterpret_cast<const avsample_s16_t*>(data_.data() +
                                                             sizeof(NwaHeader));
    ValidateData();
  }

  std::vector<avsample_s16_t> DecodeAll() override {
    const int sample_count = header_->total_sample_count;
    std::vector<avsample_s16_t> ret(sample_count);
    std::copy_n(sample_stream_, ret.size(), ret.begin());
    return ret;
  }

  std::vector<avsample_s16_t> DecodeNext() override { return DecodeAll(); }

 private:
  void ValidateData() {
    std::ostringstream os;

    if (sizeof(NwaHeader) + header_->original_size != data_.size()) {
      os << "File size mismatch: expected "
         << (sizeof(NwaHeader) + header_->original_size) << " bytes, but got "
         << data_.size() << " bytes.\n";
    }
    if (header_->unit_count != 0) {
      os << "Uncompressed NWA should have 0 units, but got "
         << header_->unit_count << ".\n";
    }
    if (header_->total_sample_count * header_->bits_per_sample !=
        header_->original_size * 8) {
      os << "Data stream length is insufficient to hold all samples: expected "
         << (header_->total_sample_count * header_->bits_per_sample) / 8
         << " bytes, but got " << header_->original_size << " bytes.\n";
    }

    std::string errorMsg = os.str();
    if (!errorMsg.empty())
      throw std::runtime_error(errorMsg);
  }

  std::string_view data_;
  const NwaHeader* header_;
  const avsample_s16_t* sample_stream_;
};

// -----------------------------------------------------------------------
// class NwaHQDecoder
// -----------------------------------------------------------------------
class NwaCompDecoder : public NwaDecoderImpl {
 public:
  NwaCompDecoder(std::string_view data) : data_(data) {
    header_ = reinterpret_cast<const NwaHeader*>(data_.data());
    unit_count_ = header_->unit_count;
    offset_table_ =
        reinterpret_cast<const int*>(data_.data() + sizeof(NwaHeader));
    current_unit_ = 0;
    ValidateData();
  }

  std::vector<avsample_s16_t> DecodeAll() override {
    std::vector<avsample_s16_t> ret;
    ret.reserve(header_->samples_per_unit * header_->unit_count);
    while (current_unit_ < unit_count_) {
      auto samples = DecodeNext();
      ret.insert(ret.end(), samples.begin(), samples.end());
    }
    return ret;
  }

  std::vector<avsample_s16_t> DecodeNext() override {
    if (current_unit_ >= unit_count_)
      throw std::logic_error(
          "DecodeNext() called when no more data is available for decoding.");
    return DecodeUnit(current_unit_++);
  }

  bool HasNext() override { return current_unit_ < unit_count_; }

  std::vector<avsample_s16_t> DecodeUnit(int id) {
    int start_offset = offset_table_[id],
        unit_size = (id == (unit_count_ - 1))
                        ? header_->last_unit_packed_size
                        : (offset_table_[id + 1] - offset_table_[id]);
    std::string_view unit_data = data_.substr(start_offset, unit_size);
    BitStream reader(unit_data.data(), unit_data.size());

    int current_sample_count = 0;
    const int unit_sample_count = (id == header_->unit_count - 1)
                                      ? header_->last_unit_sample_count
                                      : header_->samples_per_unit;

    std::vector<avsample_s16_t> samples;
    samples.reserve(unit_sample_count);

    avsample_s16_t sample[2] = {}, channel = 0;
    sample[0] = reader.PopAs<avsample_s16_t>(16);
    if (header_->channel_count == 2)
      sample[1] = reader.PopAs<avsample_s16_t>(16);
    const int compression = header_->compression_level;

    auto ReadSignedMagnitude = [](int value, int bits) {
      bool sign = (value >> (bits - 1)) & 1;
      int magnitude = value & ((1 << ((bits - 1))) - 1);
      return (sign ? -1 : 1) * magnitude;
    };
    auto AppendSample = [&samples,
                         &current_sample_count](avsample_s16_t sample) {
      samples.push_back(sample);
      ++current_sample_count;
    };

    while (current_sample_count < unit_sample_count) {
      if (reader.Position() >= reader.Size()) {
        std::ostringstream oss;
        oss << "Data section length mismatch in unit " << id << ": expected "
            << unit_size << " bytes, but reached end of data.";
        throw std::runtime_error(oss.str());
      }

      unsigned int type = reader.Popbits(3);
      switch (type) {
        case 0:
          if (header_->zero_mode_flag) {
            int zero_count = reader.Popbits(1);
            if (zero_count == 0b1)
              zero_count = reader.Popbits(2);
            if (zero_count == 0b11)
              zero_count = reader.Popbits(8);

            while (--zero_count)  // generate (zero_count-1) samples here, the
                                  // last one will be created outside switch
              AppendSample(sample[channel]);
          }
          break;

        case 1:
        case 2:
        case 3:
        case 4:
        case 5:
        case 6: {
          int bits = compression >= 3 ? 3 + compression : 5 - compression;
          int shift = compression >= 3 ? 1 + type : 2 + type + compression;
          int base = ReadSignedMagnitude(reader.Popbits(bits), bits);
          sample[channel] += base << shift;
          break;
        }

        case 7: {
          bool flag = reader.Popbits(1);
          if (flag)
            sample[channel] = 0;
          else {
            int bits = compression >= 3 ? 8 : 8 - compression;
            int shift = compression >= 3 ? 9 : 9 + compression;
            int base = ReadSignedMagnitude(reader.Popbits(bits), bits);
            sample[channel] += base << shift;
          }
          break;
        }
        default:
          break;
      }

      AppendSample(sample[channel]);
      if (header_->channel_count == 2)
        channel ^= 1;
    }

    return samples;
  }

 private:
  void ValidateData() {
    std::ostringstream os;

    if (data_.size() != header_->packed_size) {
      os << "File size mismatch: expected " << header_->packed_size
         << " bytes, but got " << data_.size() << " bytes.\n";
    }

    if (header_->bits_per_sample * header_->total_sample_count !=
        header_->original_size * 8) {
      os << "Data stream length mismatch: expected "
         << (header_->total_sample_count * header_->bits_per_sample) / 8
         << " bytes, but got " << header_->original_size << " bytes.\n";
    }

    if (header_->unit_count <= 0) {
      os << "Invalid unit count: got " << header_->unit_count << ".\n";
    }

    if (header_->samples_per_unit * (header_->unit_count - 1) +
            header_->last_unit_sample_count !=
        header_->total_sample_count) {
      os << "Sample count mismatch: expected " << header_->total_sample_count
         << " samples, but calculated "
         << (header_->samples_per_unit * (header_->unit_count - 1) +
             header_->last_unit_sample_count)
         << " samples.\n";
    }

    std::string errorMsg = os.str();
    if (!errorMsg.empty())
      throw std::runtime_error(errorMsg);
  }

  std::string_view data_;
  const NwaHeader* header_;
  int unit_count_;
  const int* offset_table_;
  int current_unit_;
};

// -----------------------------------------------------------------------
// class NwaDecoder
// -----------------------------------------------------------------------
NwaDecoder::NwaDecoder(std::string_view data)
    : data_(data), header_(reinterpret_cast<const NwaHeader*>(data.data())) {
  ValidateHeader();
  CreateImplementation();
}
NwaDecoder::NwaDecoder(char* data, size_t length)
    : NwaDecoder(std::string_view(data, length)) {}

AudioData NwaDecoder::DecodeNext() {
  return AudioData{.spec = GetSpec(), .data = impl_->DecodeNext()};
}

AudioData NwaDecoder::DecodeAll() {
  return AudioData{.spec = GetSpec(), .data = impl_->DecodeAll()};
}

bool NwaDecoder::HasNext() { return impl_->HasNext(); }

AVSpec NwaDecoder::GetSpec() const {
  return AVSpec{.sample_rate = header_->sample_rate,
                .sample_format = AV_SAMPLE_FMT::S16,
                .channel_count = header_->channel_count};
}

void NwaDecoder::ValidateHeader() const {
  if (data_.size() <= sizeof(NwaHeader)) {
    throw std::runtime_error(
        "Invalid NWA data: data size is too small to contain a valid header.");
  }

  std::ostringstream os;
  if (header_->channel_count != 1 && header_->channel_count != 2) {
    os << "Invalid channel count: expected 1 or 2, but got "
       << header_->channel_count << ".\n";
  }
  if (header_->bits_per_sample != 16) {
    os << "Invalid bit depth: expected 16-bit audio, but got "
       << header_->bits_per_sample << "-bit.\n";
  }
  if (header_->compression_level < -1 || header_->compression_level > 5) {
    os << "Invalid compression level: " << header_->compression_level
       << " is not supported.\n";
  }

  std::string errorMsg = os.str();
  if (!errorMsg.empty())
    throw std::runtime_error(errorMsg);
}

void NwaDecoder::CreateImplementation() {
  if (header_->compression_level == -1) {
    impl_ = std::make_unique<NwaHQDecoder>(data_);
  } else {
    impl_ = std::make_unique<NwaCompDecoder>(data_);
  }
}
