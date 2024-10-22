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

#ifndef SRC_BASE_AUDIO_TABLE_H_
#define SRC_BASE_AUDIO_TABLE_H_

#include <map>
#include <string>

// Defines a piece of background music who's backed by a file, usually
// VisualArt's nwa format.
struct DSTrack {
  DSTrack();
  DSTrack(const std::string name,
          const std::string file,
          int from,
          int to,
          int loop);

  std::string name;
  std::string file;
  int from;
  int to;
  int loop;

  bool operator==(const DSTrack& rhs) const {
    return from == rhs.from && to == rhs.to && loop == rhs.loop &&
           name == rhs.name && file == rhs.file;
  }
  bool operator!=(const DSTrack& rhs) const { return !(*this == rhs); }
};

// Defines a piece of background music who's backed by a cd audio track.
struct CDTrack {
  CDTrack();
  CDTrack(const std::string name, int from, int to, int loop);

  std::string name;
  int from;
  int to;
  int loop;

  bool operator==(const CDTrack& rhs) const {
    return from == rhs.from && to == rhs.to && loop == rhs.loop &&
           name == rhs.name;
  }
  bool operator!=(const CDTrack& rhs) const { return !(*this == rhs); }
};

#endif
