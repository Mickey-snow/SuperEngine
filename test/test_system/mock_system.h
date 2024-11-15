// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
// -----------------------------------------------------------------------

#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "systems/base/system.h"

class MockSystem : public System {
 public:
  MockSystem(Gameexe& gexe) : gexe_(gexe) {}

  MOCK_METHOD(void, Run, (RLMachine&), (override));
  MOCK_METHOD(GraphicsSystem&, graphics, (), (override));
  MOCK_METHOD(EventSystem&, event, (), (override));
  MOCK_METHOD(TextSystem&, text, (), (override));
  MOCK_METHOD(SoundSystem&, sound, (), (override));

  Gameexe& gameexe() override { return gexe_; }

  Gameexe& gexe_;
};
