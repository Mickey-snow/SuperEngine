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

#include "core/avdec/nwa.hpp"

#include <algorithm>
#include <sstream>

using pcm_count_t = NwaDecoder::pcm_count_t;

// -----------------------------------------------------------------------
// class NwaHQDecoder
// -----------------------------------------------------------------------
class NwaHQDecoder : public NwaDecoderImpl {
 public:
  NwaHQDecoder(std::string_view data) : data_(data) {
    header_ = reinterpret_cast<const NwaHeader*>(data_.data());
    sample_stream_ = reinterpret_cast<const avsample_s16_t*>(data_.data() +
                                                             sizeof(NwaHeader));
    cursor_ = sample_stream_;
    ValidateData();

    static constexpr size_t default_chunk_size = 512;
    SetChunkSize(default_chunk_size);
  }

  std::string DecoderName() const override { return "NwaHQDecoder"; }

  std::vector<avsample_s16_t> DecodeAll() override {
    const int sample_count = header_->total_sample_count;
    std::vector<avsample_s16_t> ret(sample_count);
    std::copy_n(sample_stream_, ret.size(), ret.begin());
    cursor_ += sample_count;
    return ret;
  }

  std::vector<avsample_s16_t> DecodeNext() override {
    if (Location() >= header_->total_sample_count)
      throw std::logic_error(
          "DecodeNext() called when no more data is available for decoding.");

    const size_t remain_samples = header_->total_sample_count - Location();
    const size_t sample_count =
        std::min(remain_samples, chunk_size_ * header_->channel_count);

    std::vector<avsample_s16_t> ret(sample_count);
    std::copy_n(cursor_, ret.size(), ret.begin());
    cursor_ += sample_count;
    return ret;
  }

  bool HasNext() override { return Location() < header_->total_sample_count; }

  void Rewind() override { cursor_ = sample_stream_; }

  size_t Location() const noexcept { return cursor_ - sample_stream_; }

  void SetChunkSize(size_t size) { chunk_size_ = size; }

  SEEK_RESULT Seek(long long offset, SEEKDIR whence) override {
    offset *= header_->channel_count;
    switch (whence) {
      case SEEKDIR::BEG:
        cursor_ = sample_stream_ + offset;
        break;
      case SEEKDIR::CUR:
        cursor_ = sample_stream_ + (Location() + offset);
        break;
      case SEEKDIR::END:
        cursor_ = (sample_stream_ + header_->total_sample_count) + offset;
        break;
      default:
        return SEEK_RESULT::FAIL;
    }
    return SEEK_RESULT::PRECISE_SEEK;
  }

  pcm_count_t Tell() override { return Location() / header_->channel_count; }

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
  const avsample_s16_t *sample_stream_, *cursor_;
  size_t chunk_size_;
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

  std::string DecoderName() const override { return "NwaCompDecoder"; }

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
          "DecodeNext() called when no more data is available for "
          "decoding.");
    return DecodeUnit(current_unit_++);
  }

  bool HasNext() override { return current_unit_ < unit_count_; }

  void Rewind() override { current_unit_ = 0; }

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

            for (int i = 1; i < zero_count;
                 ++i)  // generate (zero_count-1) samples here, the
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

  pcm_count_t SamplesBefore(int unit_id) {
    if (unit_id == unit_count_) {
      return (unit_count_ - 1) * header_->samples_per_unit +
             header_->last_unit_sample_count;
    } else {
      return unit_id * header_->samples_per_unit;
    }
  }

  SEEK_RESULT Seek(long long offset, SEEKDIR whence) override {
    long long target_offset = 0;
    offset *= header_->channel_count;

    switch (whence) {
      case SEEKDIR::BEG:
        target_offset = offset;
        break;
      case SEEKDIR::CUR:
        target_offset = Tell() * header_->channel_count + offset;
        break;
      case SEEKDIR::END:
        target_offset = header_->total_sample_count + offset;
        break;
      default:
        return SEEK_RESULT::FAIL;
    }

    if (target_offset < 0 || target_offset >= header_->total_sample_count) {
      std::ostringstream oss;
      oss << DecoderName() << ": ";
      oss << "Seek out of range (" << 0 << ',' << header_->total_sample_count
          << ") ";
      oss << '[' << target_offset << ']';
      throw std::invalid_argument(oss.str());
    }

    long long result_offset = 0;
    for (int i = 0; i < unit_count_; ++i) {
      auto this_offset = SamplesBefore(i);
      if (this_offset <= target_offset) {
        result_offset = this_offset;
        current_unit_ = i;
      } else
        break;
    }

    return result_offset == target_offset ? SEEK_RESULT::PRECISE_SEEK
                                          : SEEK_RESULT::IMPRECISE_SEEK;
  }

  pcm_count_t Tell() override {
    return SamplesBefore(current_unit_) / header_->channel_count;
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

std::string NwaDecoder::DecoderName() const {
  if (impl_)
    return impl_->DecoderName();
  return "NwaDecoder(no implementation)";
}

AudioData NwaDecoder::DecodeNext() {
  return AudioData{.spec = GetSpec(), .data = impl_->DecodeNext()};
}

AudioData NwaDecoder::DecodeAll() {
  return AudioData{.spec = GetSpec(), .data = impl_->DecodeAll()};
}

bool NwaDecoder::HasNext() { return impl_->HasNext(); }

AVSpec NwaDecoder::GetSpec() {
  return AVSpec{.sample_rate = header_->sample_rate,
                .sample_format = AV_SAMPLE_FMT::S16,
                .channel_count = header_->channel_count};
}

void NwaDecoder::ValidateHeader() const {
  if (data_.size() <= sizeof(NwaHeader)) {
    throw std::runtime_error(
        "Invalid NWA data: data size is too small to contain a valid "
        "header.");
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

SEEK_RESULT NwaDecoder::Seek(long long offset, SEEKDIR whence) {
  return impl_->Seek(offset, whence);
}

pcm_count_t NwaDecoder::Tell() { return impl_->Tell(); }
