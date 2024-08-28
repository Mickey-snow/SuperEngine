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

// Constructor definitions
NwaHQDecoder::NwaHQDecoder(std::string_view data) : sv_data_(data) {
  ReadHeader();
}

NwaHQDecoder::NwaHQDecoder(char* data, size_t length) : sv_data_(data, length) {
  ReadHeader();
}

// Method definitions
std::vector<avsample_s16_t> NwaHQDecoder::DecodeAll() {
  const int sample_count = hdr_->totalSamples;
  std::vector<avsample_s16_t> ret(sample_count);
  std::copy_n(stream_, ret.size(), ret.begin());
  return ret;
}

void NwaHQDecoder::ReadHeader() {
  const char* rawdata = sv_data_.data();
  hdr_ = (NwaHeader*)rawdata;

  CheckHeader();

  stream_ = (int16_t*)(rawdata + sizeof(NwaHeader));
}

void NwaHQDecoder::CheckHeader() {
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
