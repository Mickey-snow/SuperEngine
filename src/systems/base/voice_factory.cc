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

#include "systems/base/voice_factory.h"

#include "base/avdec/audio_decoder.h"
#include "systems/base/nwk_voice_archive.h"
#include "systems/base/ovk_voice_archive.h"
#include "systems/base/voice_archive.h"

#include <boost/algorithm/string.hpp>

const int ID_RADIX = 100000;

namespace fs = std::filesystem;

VoiceFactory::VoiceFactory(std::shared_ptr<IAssetScanner> assets)
    : file_cache_(7), assets_(assets) {}

VoiceFactory::~VoiceFactory() {}

std::shared_ptr<IAudioDecoder> VoiceFactory::Find(int id) {
  int file_no = id / ID_RADIX;
  int index = id % ID_RADIX;

  if (std::shared_ptr<IVoiceArchive> archive = FindArchive(file_no))
    return archive->MakeDecoder(index);

  fs::path sample = LocateUnpackedSample(file_no, index);
  if (fs::exists(sample)) {
    // return unpacked audio
  }

  throw std::runtime_error("No such voice archive or sample");
}

fs::path VoiceFactory::LocateArchive(int file_no) const {
  std::ostringstream oss;
  oss << "z" << std::setw(4) << std::setfill('0') << file_no;

  static const std::set<std::string> koe_archive_filetypes{"ovk", "koe", "nwk"};
  fs::path file;
  try {
    file = assets_->FindFile(oss.str(), koe_archive_filetypes);
  } catch (...) {
    return {};
  }
  return file;
}

fs::path VoiceFactory::LocateUnpackedSample(int file_no, int index) const {
  // Loose voice files are packed into directories, like:
  // /KOE/0008/z000800073.ogg. We only need to search for the filename though.
  std::ostringstream oss;
  oss << "z" << std::setw(4) << std::setfill('0') << file_no << std::setw(5)
      << std::setfill('0') << index;

  static const std::set<std::string> koe_loose_filetypes{"ogg"};
  fs::path file;
  try {
    file = assets_->FindFile(oss.str(), koe_loose_filetypes);
  } catch (...) {
    return {};
  }
  return file;
}

std::shared_ptr<IVoiceArchive> VoiceFactory::FindArchive(int file_no) const {
  using boost::iends_with;

  fs::path file = LocateArchive(file_no);
  std::string file_str = file.string();
  if (iends_with(file_str, "ovk")) {
    return std::make_shared<OVKVoiceArchive>(file, file_no);
  } else if (iends_with(file_str, "nwk")) {
    return std::make_shared<NWKVoiceArchive>(file, file_no);
  }
  return nullptr;
}
