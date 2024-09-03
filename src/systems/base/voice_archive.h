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
// -----------------------------------------------------------------------

#ifndef SRC_SYSTEMS_BASE_VOICE_ARCHIVE_H_
#define SRC_SYSTEMS_BASE_VOICE_ARCHIVE_H_

#include "utilities/mapped_file.h"

#include <string>

struct VoiceClip {
  FilePos content;
  std::string format_name;
};

class IVoiceArchive {
 public:
  virtual ~IVoiceArchive() = default;

  virtual VoiceClip LoadContent(int sample_num) = 0;
};

#endif  // SRC_SYSTEMS_BASE_VOICE_ARCHIVE_H_
