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

#include <string_view>

std::string MakeRiffHeader(AVSpec spec);

std::string EncodeWav(AudioData audio);

struct fmtHeader {
  uint16_t wFormatTag;
  int16_t nChannels;
  int32_t nSamplesPerSec;
  int32_t nAvgBytesPerSec;
  int16_t nBlockAlign;
  int16_t wBitsPerSample;
  uint16_t cbSize;
};

class WavDecoder {
 public:
  WavDecoder(std::string_view sv);
  ~WavDecoder() = default;

  AudioData DecodeAll();

 private:
  std::string_view wavdata_;
  fmtHeader* fmt_;
  std::string_view data_;

  void ValidateWav();
  void ParseChunks();
};

#endif
