// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
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

#include "base/sound_settings.hpp"

#include "base/gameexe.hpp"

rlSoundSettings::rlSoundSettings()
    : sound_quality(5),
      bgm_enabled(true),
      bgm_volume(255),
      pcm_enabled(true),
      pcm_volume(255),
      se_enabled(true),
      se_volume(255),
      koe_mode(0),
      koe_enabled(true),
      koe_volume(255),
      bgm_koe_fade(true),
      bgm_koe_fade_vol(128) {}

rlSoundSettings::rlSoundSettings(Gameexe& gexe)
    : sound_quality(gexe("SOUND_DEFAULT").ToInt(5)),
      bgm_enabled(true),
      bgm_volume(255),
      pcm_enabled(true),
      pcm_volume(255),
      se_enabled(true),
      se_volume(255),
      koe_mode(0),
      koe_enabled(true),
      koe_volume(255),
      bgm_koe_fade(true),
      bgm_koe_fade_vol(128) {}
