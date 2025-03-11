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

#include "m6/exception.hpp"
#include "m6/symbol_table.hpp"
#include "machine/value.hpp"

using namespace m6;

TEST(SymbolTableTest, InsertAndExistsTest) {
  SymbolTable symtab;

  EXPECT_FALSE(symtab.Exists("x"));

  Value_ptr intValue = make_value(42);
  symtab.Set("x", intValue);
  EXPECT_TRUE(symtab.Exists("x"));

  Value_ptr retrieved = symtab.Get("x");
  EXPECT_EQ(retrieved->Desc(), "<int: 42>");
}

TEST(SymbolTableTest, GetNonExistingSymbolTest) {
  SymbolTable symtab;

  EXPECT_THROW({ symtab.Get("nonexistent"); }, NameError);
}

TEST(SymbolTableTest, RemoveSymbolTest) {
  SymbolTable symtab;
  Value_ptr strValue = make_value(std::string("hello"));
  symtab.Set("greeting", strValue);

  EXPECT_TRUE(symtab.Exists("greeting"));

  bool removed = symtab.Remove("greeting");
  EXPECT_TRUE(removed);
  EXPECT_FALSE(symtab.Exists("greeting"));
  EXPECT_FALSE(symtab.Remove("greeting"));
}

TEST(SymbolTableTest, ClearTest) {
  SymbolTable symtab;
  symtab.Set("a", make_value(1));
  symtab.Set("b", make_value(2));
  symtab.Set("c", make_value("three"));

  EXPECT_TRUE(symtab.Exists("a"));
  EXPECT_TRUE(symtab.Exists("b"));
  EXPECT_TRUE(symtab.Exists("c"));

  symtab.Clear();
  EXPECT_FALSE(symtab.Exists("a"));
  EXPECT_FALSE(symtab.Exists("b"));
  EXPECT_FALSE(symtab.Exists("c"));
}

TEST(SymbolTableTest, UpdateValueTest) {
  SymbolTable symtab;

  symtab.Set("var", make_value(100));
  EXPECT_EQ(symtab.Get("var")->Str(), "100");

  symtab.Set("var", make_value(200));
  EXPECT_EQ(symtab.Get("var")->Str(), "200");
}
