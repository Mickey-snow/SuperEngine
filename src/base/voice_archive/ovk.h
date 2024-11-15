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
// -----------------------------------------------------------------------

#pragma once

#include <filesystem>
#include <vector>

#include "base/voice_archive/ivoicearchive.h"

struct OVK_Header {
  int32_t size;
  int32_t offset;
  int32_t id;
  int32_t sample_count;

  bool operator<(const OVK_Header& rhs) const { return id < rhs.id; }
  bool operator<(const int rhs) const { return id < rhs; }
};

// A VoiceArchive that reads the Ogg Vorbis archives (OVK files).
class OVKVoiceArchive : public IVoiceArchive {
 public:
  OVKVoiceArchive(std::filesystem::path file, int file_no);
  virtual ~OVKVoiceArchive();

  virtual VoiceClip LoadContent(int sample_num) override;

 private:
  void ReadEntry();

  // The file to read from
  std::filesystem::path file_;

  int file_no_;

  std::shared_ptr<MappedFile> file_content_;

  // A list of samples in this archive
  std::vector<OVK_Header> entries_;
};  // class OVKVoiceArchive
