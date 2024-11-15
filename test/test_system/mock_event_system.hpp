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

#include "systems/base/event_system.hpp"

class MockEventSystem : public EventSystem {
 public:
  MockEventSystem(Gameexe& gexe) : EventSystem(gexe) {}

  MOCK_METHOD(void, ExecuteEventSystem, (RLMachine & machine), (override));
  MOCK_METHOD(unsigned int, GetTicks, (), (const, override));
  MOCK_METHOD(void, Wait, (unsigned int milliseconds), (const, override));
  MOCK_METHOD(bool, ShiftPressed, (), (const, override));
  MOCK_METHOD(bool, CtrlPressed, (), (const, override));
  MOCK_METHOD(Point, GetCursorPos, (), (override));
  MOCK_METHOD(void,
              GetCursorPos,
              (Point & position, int& button1, int& button2),
              (override));
  MOCK_METHOD(void, FlushMouseClicks, (), (override));
  MOCK_METHOD(unsigned int, TimeOfLastMouseMove, (), (override));
  MOCK_METHOD(void,
              InjectMouseMovement,
              (RLMachine & machine, const Point& loc),
              (override));

  MOCK_METHOD(void, InjectMouseDown, (RLMachine & machine), (override));
  MOCK_METHOD(void, InjectMouseUp, (RLMachine & machine), (override));
};
