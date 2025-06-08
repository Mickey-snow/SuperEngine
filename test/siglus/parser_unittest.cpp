// -----------------------------------------------------------------------
//
// This file is part of RLVM
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
// -----------------------------------------------------------------------

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "libsiglus/parser.hpp"

#include <sstream>

namespace siglus_test {
using namespace libsiglus;
using namespace libsiglus::lex;

class MockParserContext : public Parser::Context {
 public:
  MockParserContext(std::vector<token::Token_t>& out_buf) : output(out_buf) {}

  // Mock all the virtual methods:
  MOCK_METHOD(std::string_view, SceneData, (), (const, override));
  MOCK_METHOD(const std::vector<std::string>&, Strings, (), (const, override));
  MOCK_METHOD(const std::vector<int>&, Labels, (), (const, override));

  MOCK_METHOD(const std::vector<libsiglus::Property>&,
              SceneProperties,
              (),
              (const, override));
  MOCK_METHOD(const std::vector<libsiglus::Property>&,
              GlobalProperties,
              (),
              (const, override));
  MOCK_METHOD(const std::vector<libsiglus::Command>&,
              SceneCommands,
              (),
              (const, override));
  MOCK_METHOD(const std::vector<libsiglus::Command>&,
              GlobalCommands,
              (),
              (const, override));

  MOCK_METHOD(int, SceneId, (), (const, override));
  MOCK_METHOD(std::string, GetDebugTitle, (), (const, override));

  void Emit(token::Token_t t) final { output.emplace_back(std::move(t)); }
  std::vector<token::Token_t>& output;
};

class SiglusParserTest : public ::testing::Test {
 protected:
  std::vector<token::Token_t> tokens;
  MockParserContext ctx;
  Parser parser;

  SiglusParserTest() : tokens(), ctx(tokens), parser(ctx) {}

  inline void Parse(auto&&... params) {
    (parser.Add(Lexeme(std::forward<decltype(params)>(params))), ...);
  }
};

TEST_F(SiglusParserTest, Gosub) {
  EXPECT_NO_THROW(
      Parse(Marker{}, Push{Type::Int, 83}, Push{Type::Int, 0},
            Push{Type::Int, -1}, Push{Type::Int, 0},
            Gosub{.return_type_ = Type::Int, .label_ = 5, .argt_ = {}},
            Assign{.ltype_ = Type::Int, .rtype_ = Type::Int, .v1_ = 1}));
}

}  // namespace siglus_test
