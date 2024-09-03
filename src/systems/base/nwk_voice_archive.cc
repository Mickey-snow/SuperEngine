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

#include "base/avdec/audio_decoder.h"
#include "utilities/byte_reader.h"

namespace fs = std::filesystem;

NWKVoiceArchive::NWKVoiceArchive(fs::path file, int file_no)
    : file_(file),
      file_no_(file_no),
      file_content_(std::make_shared<MappedFile>(file)) {
  ReadEntry();
}

NWKVoiceArchive::~NWKVoiceArchive() {}

VoiceClip NWKVoiceArchive::LoadContent(int sample_num) {
  auto it = std::lower_bound(entries_.cbegin(), entries_.cend(), sample_num);
  if (it == entries_.end() || it->id != sample_num)
    throw std::runtime_error("Couldn't find sample in NWKVoiceArchive: " +
                             std::to_string(sample_num));

  auto content = FilePos{
      .file_ = file_content_, .position = it->offset, .length = it->size};
  return VoiceClip{.content = std::move(content), .format_name = "nwa"};
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
