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
#include "utilities/string_utilities.hpp"

#include <sstream>

namespace siglus_test {
using namespace libsiglus;
using namespace libsiglus::lex;

using ::testing::Return;
using ::testing::ReturnRef;

class MockParserContext : public Parser::Context {
 public:
  MockParserContext(std::vector<token::Token_t>& out_buf) : output(out_buf) {}

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

  void Warn(std::string msg) final { ADD_FAILURE() << msg; }
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

  struct TokenArray {
    std::vector<token::Token_t> tokens;
    friend std::ostream& operator<<(std::ostream& os, const TokenArray& t) {
      for (const auto& it : t.tokens)
        os << ToString(it) << '\n';
      return os;
    }
    bool operator==(const TokenArray& other) const {
      return tokens == other.tokens;
    }
    bool operator==(char const* str) const {
      std::ostringstream oss;
      oss << *this;
      std::string_view sv(str);
      return trim_cp(oss.str()) == trim_sv(sv);
    }
  };
  inline TokenArray Tokens() const { return TokenArray(tokens); }
};

TEST_F(SiglusParserTest, Gosub) {
  EXPECT_NO_THROW(
      Parse(Marker{}, Push{Type::Int, 83}, Push{Type::Int, 0},
            Push{Type::Int, -1}, Push{Type::Int, 0},
            Gosub{.return_type_ = Type::Int, .label_ = 5, .argt_ = {}},
            Assign{.ltype_ = Type::Int, .rtype_ = Type::Int, .v1_ = 1}));
}

TEST_F(SiglusParserTest, Operate1) {
  Parse(Push{Type::Int, 5}, Operate1{Type::Int, OperatorCode::Minus});

  ASSERT_EQ(tokens.size(), 1u);
  const auto& tok = std::get<token::Operate1>(tokens[0]);
  EXPECT_EQ(tok.op, OperatorCode::Minus);
  EXPECT_EQ(AsInt(tok.rhs), 5);
  ASSERT_TRUE(tok.val.has_value());
  EXPECT_EQ(AsInt(*tok.val), -5);
  EXPECT_EQ(std::get<Variable>(tok.dst).id, 0);
}

TEST_F(SiglusParserTest, Operate2) {
  Parse(Push{Type::Int, 10}, Push{Type::Int, 20},
        Operate2{Type::Int, Type::Int, OperatorCode::Plus});

  ASSERT_EQ(tokens.size(), 1u);
  const auto& tok = std::get<token::Operate2>(tokens[0]);
  EXPECT_EQ(tok.op, OperatorCode::Plus);
  EXPECT_EQ(AsInt(tok.lhs), 10);
  EXPECT_EQ(AsInt(tok.rhs), 20);
  ASSERT_TRUE(tok.val.has_value());
  EXPECT_EQ(AsInt(*tok.val), 30);
  EXPECT_EQ(std::get<Variable>(tok.dst).id, 0);
}

TEST_F(SiglusParserTest, ConditionalGoto) {
  Parse(Push{Type::Int, 1}, Goto{lex::Goto::Condition::True, 42});

  ASSERT_EQ(tokens.size(), 1u);
  const auto& tok = std::get<token::GotoIf>(tokens[0]);
  EXPECT_TRUE(tok.cond);
  EXPECT_EQ(tok.label, 42);
  EXPECT_EQ(AsInt(tok.src), 1);
}

TEST_F(SiglusParserTest, AssignElement) {
  Parse(Marker{}, Push{Type::Int, 25}, Push{Type::Int, 7},
        Assign{Type::IntRef, Type::Int, 1});

  ASSERT_EQ(tokens.size(), 1u);
  ASSERT_TRUE(std::holds_alternative<token::Assign>(tokens[0]));
  const auto& tok = std::get<token::Assign>(tokens[0]);
  EXPECT_EQ(tok.dst_elmcode, elm::ElementCode{25});
  EXPECT_EQ(AsInt(tok.src), 7);
}

TEST_F(SiglusParserTest, ObjectElmArg) {
  std::vector<std::string> strs{"bg47"};
  EXPECT_CALL(ctx, Strings).WillRepeatedly(ReturnRef(strs));
  std::vector<libsiglus::Command> g_cmd{
      libsiglus::Command{.scene_id = 78, .offset = 913, .name = "$$usr_cmd"}};
  EXPECT_CALL(ctx, GlobalCommands).WillRepeatedly(ReturnRef(g_cmd));

  Parse(Marker{}, Push{Type::Int, 0x7e000000}, Marker{}, Push{Type::Int, 37},
        Push{Type::Int, 2}, Push{Type::Int, -1}, Push{Type::Int, 0},
        Push{Type::String, 0},
        lex::Command(elm::Signature{
            .overload_id = 0,
            .arglist = elm::ArgumentList({Type::Object, Type::String}),
            .argtags = {},
            .rettype = Type::Int}));

  EXPECT_EQ(Tokens(), R"(
object v0 = <int:37,int:2,int:-1,int:0>
int v1 = @78.913:$$usr_cmd(v0,str:bg47) ;cmd<int:2113929216>
)");
}

}  // namespace siglus_test
