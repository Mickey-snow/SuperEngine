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

#ifndef SRC_SYSTEMS_BASE_KOEPAC_VOICE_ARCHIVE_H_
#define SRC_SYSTEMS_BASE_KOEPAC_VOICE_ARCHIVE_H_

#include <filesystem>
#include <vector>

#include "systems/base/voice_archive.h"

// A VoiceArchive that reads the older KOEPAC archives (KOE files).
class KOEPACVoiceArchive : public VoiceArchive {
 public:
  KOEPACVoiceArchive(std::filesystem::path file, int file_no);
  virtual ~KOEPACVoiceArchive();

  // Overridden from VoiceArchive:
  virtual std::shared_ptr<VoiceSample> FindSample(int sample_num) override;

 private:
  void ReadTable(std::filesystem::path file);

  // The file to read from
  std::filesystem::path file_;

  // The rate of the samples in this file.
  int rate_;

  // A list of samples in this archive
  std::vector<Entry> entries_;
};  // class KOEPACVoiceArchive

#endif  // SRC_SYSTEMS_BASE_KOEPAC_VOICE_ARCHIVE_H_
