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

#include "m6/native.hpp"
#include "machine/value.hpp"

using namespace m6;

TEST(NativeFunctionTest, Lambda) {
  Value len = make_fn_value(
      "len", [](std::vector<int> args) -> int { return args.size(); });
  EXPECT_EQ(len.Desc(), "<native function 'len'>");
  auto ptr = len.Get_if<NativeFunction>();
  EXPECT_EQ(ptr->Invoke(nullptr, {Value(1), Value(2), Value(3)}), 3);
}
