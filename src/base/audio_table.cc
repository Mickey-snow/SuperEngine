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
