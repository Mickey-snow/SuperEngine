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
#include <string_view>
#include <vector>

#include "base/avdec/iadec.h"
#include "base/avspec.h"
#include "utilities/bitstream.h"

struct NwaHeader {
  uint16_t channel_count;
  uint16_t bits_per_sample;
  uint32_t sample_rate;
  int32_t compression_level;
  uint32_t zero_mode_flag;
  uint32_t unit_count;
  uint32_t original_size;
  uint32_t packed_size;
  uint32_t total_sample_count;
  uint32_t samples_per_unit;
  uint32_t last_unit_sample_count;
  uint32_t last_unit_packed_size;
};

// Base class for NWA Decoder implementations
class NwaDecoderImpl {
 public:
  virtual ~NwaDecoderImpl() = default;

  virtual bool HasNext() = 0;
  virtual std::vector<avsample_s16_t> DecodeNext() = 0;
  virtual std::vector<avsample_s16_t> DecodeAll() = 0;
  virtual void Rewind() = 0;
};

// Main NWA Decoder class
class NwaDecoder : public IAudioDecoder {
 public:
  NwaDecoder(std::string_view data);
  NwaDecoder(char* data, size_t length);

  std::string DecoderName() const override;

  AudioData DecodeNext() override;

  AudioData DecodeAll() override;

  bool HasNext() override;

  AVSpec GetSpec() override;

  SEEK_RESULT Seek(long long offset, SEEKDIR whence = SEEKDIR::CUR) override;

 private:
  std::string_view data_;
  const NwaHeader* header_;
  std::unique_ptr<NwaDecoderImpl> impl_;

  void ValidateHeader() const;
  void CreateImplementation();
};

#endif
