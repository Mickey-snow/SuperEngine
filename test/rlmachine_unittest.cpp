// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2025 Serina Sakurai
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

#include <gtest/gtest.h>

#include "machine/instruction.hpp"
#include "machine/rlmachine.hpp"

TEST(RLMachineTest, init) {
  // For now, not crashing is enough
  std::shared_ptr<RLMachine> machine = nullptr;
  EXPECT_NO_THROW({
    machine = std::make_shared<RLMachine>(nullptr, nullptr, ScriptLocation(),
                                          nullptr);
    machine->operator()(End());
  });
  EXPECT_NE(machine, nullptr);
  EXPECT_TRUE(machine->IsHalted());
}
