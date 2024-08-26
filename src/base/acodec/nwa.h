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

#include <boost/filesystem.hpp>

#include <string_view>
#include <vector>

namespace fs = boost::filesystem;

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

class NwaDecoder {
 public:
  NwaDecoder(std::string_view data);
  NwaDecoder(char* data, size_t length);
  ~NwaDecoder() = default;

  std::vector<float> DecodeAll();

 private:
  void ReadHeader();
  void CheckHeader();

  std::string_view sv_data_;
  NwaHeader* hdr_;
  int16_t* stream_;
};

#endif
