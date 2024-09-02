// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2009 Elliot Glaysher
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
//
// Parts of this file have been copied from koedec_ogg.cc in the xclannad
// distribution and they are:
//
// Copyright (c) 2004-2006  Kazunori "jagarl" Ueno
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
// 1. Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright
//    notice, this list of conditions and the following disclaimer in the
//    documentation and/or other materials provided with the distribution.
// 3. The name of the author may not be used to endorse or promote products
//    derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
// IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
// OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
// IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
// NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
// THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// -----------------------------------------------------------------------

#include "systems/base/ovk_voice_archive.h"
#include "utilities/byte_reader.h"

#include <filesystem>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "systems/base/ovk_voice_sample.h"
#include "utilities/exception.h"
#include "xclannad/endian.hpp"

namespace fs = std::filesystem;

// -----------------------------------------------------------------------
// OVKVoiceArchive
// -----------------------------------------------------------------------
OVKVoiceArchive::OVKVoiceArchive(fs::path file, int file_no)
    : VoiceArchive(file_no),
      file_(file),
      file_no_(file_no),
      file_content_(std::make_shared<MappedFile>(file)) {
  ReadEntry();
}

// -----------------------------------------------------------------------

OVKVoiceArchive::~OVKVoiceArchive() {}

// -----------------------------------------------------------------------

std::shared_ptr<VoiceSample> OVKVoiceArchive::FindSample(int sample_num) {
  auto it = std::lower_bound(entries_.begin(), entries_.end(), sample_num);
  if (it != entries_.end()) {
    return std::shared_ptr<VoiceSample>(
        new OVKVoiceSample(file_, it->offset, it->size));
  }

  throw rlvm::Exception("Couldn't find sample in OVKVoiceArchive");
}

FilePos OVKVoiceArchive::LoadContent(int sample_num) {
  auto it = std::lower_bound(entries_.cbegin(), entries_.cend(), sample_num);
  if (it == entries_.end() || it->id != sample_num)
    throw std::runtime_error("Couldn't find sample in OVKVoiceArchive: " +
                             std::to_string(sample_num));

  return FilePos{
      .file_ = file_content_, .position = it->offset, .length = it->size};
}

void OVKVoiceArchive::ReadEntry() {
  int entry_count = ByteReader(file_content_->Read(0, 4)).PopBytes(4);

  ByteReader reader(file_content_->Read(4, entry_count * sizeof(OVK_Header)));
  entries_.reserve(entry_count);
  for (int i = 0; i < entry_count; ++i) {
    OVK_Header entry;
    reader >> entry.size >> entry.offset >> entry.id >> entry.sample_count;
    entries_.emplace_back(std::move(entry));
  }

  std::sort(entries_.begin(), entries_.end());
}
