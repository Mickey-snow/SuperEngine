// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
// -----------------------------------------------------------------------

#include "Precompiled.hpp"

// -----------------------------------------------------------------------

#include "Modules/Module_Koe.hpp"

// -----------------------------------------------------------------------

KoeModule::KoeModule()
  : RLModule("Koe", 1, 23)
{
  addUnsupportedOpcode(0, 0, "koePlay");
  addUnsupportedOpcode(0, 1, "koePlay");

  addUnsupportedOpcode(1, 0, "koePlayEx");
  addUnsupportedOpcode(1, 1, "koePlayEx");

  addUnsupportedOpcode(3, 0, "koeWait");
  addUnsupportedOpcode(4, 0, "koePlaying");
  addUnsupportedOpcode(5, 0, "koeStop");
  addUnsupportedOpcode(6, 0, "koeWaitC");

  addUnsupportedOpcode(7, 0, "koePlayExC");
  addUnsupportedOpcode(7, 1, "koePlayExC");

  addUnsupportedOpcode(8, 0, "koeDoPlay");
  addUnsupportedOpcode(8, 1, "koeDoPlay");

  addUnsupportedOpcode(9, 0, "koeDoPlayEx");
  addUnsupportedOpcode(9, 1, "koeDoPlayEx");

  addUnsupportedOpcode(10, 0, "koeDoPlayExC");
  addUnsupportedOpcode(10, 1, "koeDoPlayExC");

  addUnsupportedOpcode(11, 0, "koeVolume");

  addUnsupportedOpcode(12, 0, "koeSetVolume");
  addUnsupportedOpcode(12, 1, "koeSetVolume");

  addUnsupportedOpcode(13, 0, "koeUnMute");
  addUnsupportedOpcode(13, 1, "koeUnMute");

  addUnsupportedOpcode(14, 0, "koeMute");
  addUnsupportedOpcode(14, 1, "koeMute");
}
