// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2024 Serina Sakurai
// Copyright (C) 2007 Elliot Glaysher
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

#include "base/audio_table.h"

#include "libreallive/gameexe.h"

#include <cctype>
#include <sstream>
#include <stdexcept>

DSTrack::DSTrack() : name(""), file(""), from(-1), to(-1), loop(-1) {}

DSTrack::DSTrack(const std::string in_name,
                 const std::string in_file,
                 int in_from,
                 int in_to,
                 int in_loop)
    : name(in_name), file(in_file), from(in_from), to(in_to), loop(in_loop) {}

CDTrack::CDTrack() : name(""), from(-1), to(-1), loop(-1) {}

CDTrack::CDTrack(const std::string in_name, int in_from, int in_to, int in_loop)
    : name(in_name), from(in_from), to(in_to), loop(in_loop) {}

inline void to_lower(std::string& str) {
  for (auto& it : str)
    it = tolower(it);
}

AudioTable::AudioTable(Gameexe& gexe) {
  // Read the \#SE.xxx entries from the Gameexe
  GameexeFilteringIterator se = gexe.FilterBegin("SE.");
  GameexeFilteringIterator end = gexe.FilterEnd();
  for (; se != end; ++se) {
    std::string raw_number = se->GetKeyParts().at(1);
    int entry_number = std::stoi(raw_number);

    std::string file_name = se->GetStringAt(0);
    int target_channel = se->GetIntAt(1);

    se_table_[entry_number] = std::make_pair(file_name, target_channel);
  }

  // Read the \#DSTRACK entries
  GameexeFilteringIterator dstrack = gexe.FilterBegin("DSTRACK");
  for (; dstrack != end; ++dstrack) {
    int from = dstrack->GetIntAt(0);
    int to = dstrack->GetIntAt(1);
    int loop = dstrack->GetIntAt(2);
    const std::string& file = dstrack->GetStringAt(3);
    std::string name = dstrack->GetStringAt(4);
    to_lower(name);
    ds_tracks_[name] = DSTrack(name, file, from, to, loop);
  }

  // Read the \#CDTRACK entries
  GameexeFilteringIterator cdtrack = gexe.FilterBegin("CDTRACK");
  for (; cdtrack != end; ++cdtrack) {
    int from = cdtrack->GetIntAt(0);
    int to = cdtrack->GetIntAt(1);
    int loop = cdtrack->GetIntAt(2);
    std::string name = cdtrack->GetStringAt(3);
    to_lower(name);

    cd_tracks_[name] = CDTrack(name, from, to, loop);
  }
}

DSTrack AudioTable::FindBgm(std::string bgm_name) const {
  to_lower(bgm_name);

  auto ds_it = ds_tracks_.find(bgm_name);
  if (ds_it != ds_tracks_.cend())
    return ds_it->second;

  auto cd_it = cd_tracks_.find(bgm_name);
  if (cd_it != cd_tracks_.cend()) {
    std::ostringstream oss;
    oss << "CD music not supported yet. Could not play track \"" << bgm_name
        << "\"";
    throw std::runtime_error(oss.str());
  }

  std::ostringstream oss;
  oss << "Could not find music track \"" << bgm_name << "\"";
  throw std::runtime_error(oss.str());
}

SETrack AudioTable::FindSE(int se_num) const {
  auto it = se_table_.find(se_num);
  if (it == se_table_.cend()) {
    std::ostringstream oss;
    oss << "No #SE entry found for sound effect number " << se_num;
    throw std::invalid_argument(oss.str());
  }

  return SETrack{it->second.first, it->second.second};
}
