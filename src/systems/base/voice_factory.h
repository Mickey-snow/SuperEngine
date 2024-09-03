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

#ifndef SRC_SYSTEMS_BASE_VOICE_FACTORY_H_
#define SRC_SYSTEMS_BASE_VOICE_FACTORY_H_

#include <filesystem>
#include <memory>

#include "base/avdec/iadec.h"
#include "lru_cache.hpp"
#include "systems/base/voice_archive.h"
#include "utilities/mapped_file.h"

class IAssetScanner;
class IVoiceArchive;

class VoiceFactory {
 public:
  VoiceFactory(std::shared_ptr<IAssetScanner> assets);
  ~VoiceFactory();

  std::shared_ptr<IAudioDecoder> Find(int id);

  VoiceClip LoadSample(int id);

  std::filesystem::path LocateArchive(int file_no) const;

  std::filesystem::path LocateUnpackedSample(int file_no, int index) const;

  // Searches for a file archive of voices.
  std::shared_ptr<IVoiceArchive> FindArchive(int file_no) const;

 private:
  std::shared_ptr<IAssetScanner> assets_;
};  // class VoiceCache

#endif  // SRC_SYSTEMS_BASE_VOICE_CACHE_H_
