// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2006, 2007 Elliot Glaysher
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

#include "util.hpp"

#include "m6/evaluator.hpp"
#include "m6/parser.hpp"
#include "m6/symbol_table.hpp"
#include "m6/tokenizer.hpp"
#include "m6/value.hpp"

using namespace m6;

class StrTest : public ::testing::Test {
 protected:
  void SetUp() override {
    symtab = std::make_shared<SymbolTable>();
    evaluator = std::make_shared<Evaluator>(symtab);
  }

  auto Eval(const std::string_view input) {
    Tokenizer tokenizer(input);
    auto expr = ParseExpression(std::span(tokenizer.parsed_tok_));
    return expr->Apply(*evaluator);
  }

  std::shared_ptr<SymbolTable> symtab;
  std::shared_ptr<Evaluator> evaluator;
};

TEST_F(StrTest, CompoundAssignment) {
  Eval("s1 = \"hello\"");
  Eval("s2 = s1 + \" \" ");
  Eval("s1 += \", world\"");
  Eval("s2 *= 3");

  EXPECT_VALUE_EQ(symtab->Get("s1"), "hello, world");
  EXPECT_VALUE_EQ(symtab->Get("s2"), "hello hello hello ");
}

TEST_F(StrTest, strcpy) {
  Eval(R"( strcpy(s0, "valid", 2) )");
  EXPECT_VALUE_EQ(symtab->Get("s0"), "va");
}
