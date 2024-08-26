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

#include "base/acodec/nwa.h"

#include <sstream>

// Constructor definitions
NwaDecoder::NwaDecoder(std::string_view data) : sv_data_(data) { ReadHeader(); }

NwaDecoder::NwaDecoder(char* data, size_t length) : sv_data_(data, length) {
  ReadHeader();
}

// Method definitions
std::vector<float> NwaDecoder::DecodeAll() {
  // only handle 16 bit audio currently
  std::vector<float> ret(hdr_->origSize / sizeof(int16_t));
  for (size_t i = 0; i < ret.size(); ++i) {
    ret[i] = 1.0 * ((int16_t*)stream_)[i] / INT16_MAX;
    if (ret[i] > 1.0)
      ret[i] = 1.0;
    if (ret[i] < -1.0)
      ret[i] = -1.0;
  }
  return ret;
}

void NwaDecoder::ReadHeader() {
  const char* rawdata = sv_data_.data();
  hdr_ = (NwaHeader*)rawdata;

  CheckHeader();

  stream_ = (int16_t*)(rawdata + sizeof(NwaHeader));
}

void NwaDecoder::CheckHeader() {
  bool hasError = false;
  std::ostringstream os;

  if (hdr_->channels != 1 && hdr_->channels != 2) {
    hasError = true;
    os << "Expect mono or stereo audio, got " << hdr_->channels
       << " channels\n";
  }

  if (hdr_->bitsPerSample != 16) {
    hasError = true;
    os << "Expect 16 bit audio, got " << hdr_->bitsPerSample << "bit\n";
  }

  if (hdr_->compressionMode != -1) {
    hasError = true;
    os << "Current implementation only supports no compression, audio has "
          "compression level "
       << hdr_->compressionMode << '\n';
  }

  if (sizeof(NwaHeader) + hdr_->origSize != sv_data_.size()) {
    hasError = true;
    os << "File size mismatch\n";
  }

  if (hasError)
    throw std::runtime_error(os.str());
}
