// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2011 Elliot Glaysher
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
// -----------------------------------------------------------------------

#include "systems/base/nwk_voice_archive.h"
#include "utilities/byte_reader.h"

#include <cstdio>

#include "utilities/exception.h"
#include "xclannad/endian.hpp"
#include "xclannad/wavfile.h"

namespace fs = std::filesystem;

namespace {

// A VoiceSample that reads from a NWKVoiceArchive, which is just a bunch of
// NWA files thrown together with
class NWKVoiceSample : public VoiceSample {
 public:
  NWKVoiceSample(std::filesystem::path file, int offset, int length);
  virtual ~NWKVoiceSample();

  // Overridden from VoiceSample:
  virtual char* Decode(int* size) override;

 private:
  FILE* stream_;
  int offset_;
  int length_;
};

NWKVoiceSample::NWKVoiceSample(std::filesystem::path file,
                               int offset,
                               int length)
    : stream_(std::fopen(file.native().c_str(), "rb")),
      offset_(offset),
      length_(length) {}

NWKVoiceSample::~NWKVoiceSample() {
  if (stream_)
    fclose(stream_);
}

char* NWKVoiceSample::Decode(int* size) {
  // Defined in nwatowav.cc
  return decode_koe_nwa(stream_, offset_, length_, size);
}

}  // namespace

NWKVoiceArchive::NWKVoiceArchive(fs::path file, int file_no)
    : VoiceArchive(file_no),
      file_(file),
      file_no_(file_no),
      file_content_(std::make_shared<MappedFile>(file)) {
  ReadEntry();
}

NWKVoiceArchive::~NWKVoiceArchive() {}

std::shared_ptr<VoiceSample> NWKVoiceArchive::FindSample(int sample_num) {
  auto it = std::lower_bound(entries_.begin(), entries_.end(), sample_num);
  if (it != entries_.end()) {
    return std::shared_ptr<VoiceSample>(
        new NWKVoiceSample(file_, it->offset, it->size));
  }

  throw rlvm::Exception("Couldn't find sample in NWKVoiceArchive");
}

FilePos NWKVoiceArchive::LoadContent(int sample_num) {
  auto it = std::lower_bound(entries_.cbegin(), entries_.cend(), sample_num);
  if (it == entries_.end() || it->id != sample_num)
    throw std::runtime_error("Couldn't find sample in NWKVoiceArchive: " +
                             std::to_string(sample_num));

  return FilePos{
      .file_ = file_content_, .position = it->offset, .length = it->size};
}

void NWKVoiceArchive::ReadEntry() {
  int entry_count = ByteReader(file_content_->Read(0, 4)).PopBytes(4);

  entries_.reserve(entry_count);
  ByteReader reader(file_content_->Read(4, entry_count * sizeof(NWK_Header)));
  for (int i = 0; i < entry_count; ++i) {
    NWK_Header entry;
    reader >> entry.size >> entry.offset >> entry.id;
    entries_.emplace_back(std::move(entry));
  }

  std::sort(entries_.begin(), entries_.end());
}
