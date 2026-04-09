// -----------------------------------------------------------------------
//
// This file is part of RLVM
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2026 Serina Sakurai
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

#include "libsiglus/recompiler.hpp"

#include "vm/object.hpp"
#include "vm/string.hpp"
#include "vm/vm.hpp"

#include <sstream>
#include <unordered_map>

namespace siglus_test {

namespace ls = libsiglus;
namespace sr = serilang;
namespace tk = libsiglus::token;

using ::testing::ElementsAre;

class RecompilerTest : public ::testing::Test {
 protected:
  std::shared_ptr<sr::GarbageCollector> gc =
      std::make_shared<sr::GarbageCollector>();
  ls::Recompiler recompiler{gc};

  static ls::Variable IntVar(int id) { return {ls::Type::Int, id}; }
  static ls::Variable StrVar(int id) { return {ls::Type::String, id}; }

  template <typename... Ts>
  void Emit(Ts&&... toks) {
    (recompiler.Gen(tk::Token_t(std::forward<Ts>(toks))), ...);
    AssertOk();
  }

  std::string Errors() const {
    std::ostringstream oss;
    for (const auto& err : recompiler.GetErrors())
      oss << err.message << '\n';
    return oss.str();
  }

  void AssertOk() {
    if (recompiler.Ok())
      return;

    FAIL() << Errors();
  }

  sr::Code* GetChunk() {
    sr::Module* mod = recompiler.module_;
    EXPECT_NE(mod, nullptr);
    sr::Code* chunk = (*mod->globals)["@@script"].Get_if<sr::Code>();
    EXPECT_NE(chunk, nullptr);
    return chunk;
  }

  sr::Value Run(std::unordered_map<std::string, sr::Value> globals = {},
                std::unordered_map<std::string, sr::Value> builtins = {}) {
    sr::VM vm(gc, std::move(globals), std::move(builtins));
    return vm.Evaluate(GetChunk());
  }
};

TEST_F(RecompilerTest, BinaryOp) {
  Emit(tk::Duplicate{.src = ls::Integer{11}, .dst = IntVar(0)},
       tk::Duplicate{.src = ls::Integer{22}, .dst = IntVar(1)},
       tk::Operate2{.op = ls::OperatorCode::Plus,
                    .lhs = IntVar(0),
                    .rhs = IntVar(1),
                    .dst = IntVar(2),
                    .val = std::nullopt},
       tk::Return{.ret_vals = {IntVar(2)}});

  EXPECT_THAT(GetChunk()->fast_locals,
              ElementsAre("v0", "v1", "v2"));
  EXPECT_EQ(Run(), 33);
}

TEST_F(RecompilerTest, UnaryOp) {
  Emit(tk::Duplicate{.src = ls::Integer{7}, .dst = IntVar(0)},
       tk::Operate1{.op = ls::OperatorCode::Minus,
                    .rhs = IntVar(0),
                    .dst = IntVar(1),
                    .val = std::nullopt},
       tk::Return{.ret_vals = {IntVar(1)}});

  EXPECT_THAT(GetChunk()->fast_locals, ElementsAre("v0", "v1"));
  EXPECT_EQ(Run(), -7);
}

TEST_F(RecompilerTest, Name) {
  std::string seen;
  std::unordered_map<std::string, sr::Value> globals, builtins;
  builtins["__builtin_name"] = sr::Value(gc->Allocate<sr::NativeFunction>(
      "__builtin_name",
      [&](sr::VM&, sr::Fiber& f, uint8_t nargs,
          uint8_t nkwargs) -> sr::TempValue {
        EXPECT_EQ(nargs, 1);
        EXPECT_EQ(nkwargs, 0);

        const auto* str = f.op_stack.back().Get_if<sr::String>();
        EXPECT_NE(str, nullptr);
        if (str)
          seen = str->str_;
        f.op_stack.resize(f.op_stack.size() - 1);
        return sr::Value();
      }));

  Emit(tk::Name{.str = ls::String{"Alice"}});
  ASSERT_FALSE(GetChunk()->code.empty());

  EXPECT_EQ(Run(globals, builtins), std::monostate());
  EXPECT_EQ(seen, "Alice");
}

TEST_F(RecompilerTest, Textout) {
  int seen_kidoku = -1;
  std::string seen_text;
  std::unordered_map<std::string, sr::Value> globals, builtins;
  builtins["__builtin_textout"] = sr::Value(gc->Allocate<sr::NativeFunction>(
      "__builtin_textout",
      [&](sr::VM&, sr::Fiber& f, uint8_t nargs,
          uint8_t nkwargs) -> sr::TempValue {
        EXPECT_EQ(nargs, 2);
        EXPECT_EQ(nkwargs, 0);

        const int* kidoku = f.op_stack.end()[-2].Get_if<int>();
        const auto* str = f.op_stack.back().Get_if<sr::String>();
        EXPECT_NE(kidoku, nullptr);
        EXPECT_NE(str, nullptr);
        if (kidoku)
          seen_kidoku = *kidoku;
        if (str)
          seen_text = str->str_;
        f.op_stack.resize(f.op_stack.size() - 2);
        return sr::Value();
      }));

  Emit(tk::Duplicate{.src = ls::String{"Hello"}, .dst = StrVar(0)},
       tk::Textout{.kidoku = 7, .str = StrVar(0)}, tk::Return{.ret_vals = {}});

  EXPECT_THAT(GetChunk()->fast_locals, ElementsAre("v0"));
  EXPECT_EQ(Run(globals, builtins), std::monostate());
  EXPECT_EQ(seen_kidoku, 7);
  EXPECT_EQ(seen_text, "Hello");
}

TEST_F(RecompilerTest, ReturnWithoutValue) {
  Emit(tk::Return{.ret_vals = {}});

  EXPECT_EQ(Run(), std::monostate());
}

TEST_F(RecompilerTest, ReturnMultipleValues) {
  Emit(
      tk::Return{.ret_vals = {ls::Integer{1}, ls::Integer{2}, ls::Integer{3}}});

  sr::Value result = Run();
  const auto* list = result.Get_if<sr::List>();
  ASSERT_NE(list, nullptr);
  EXPECT_THAT(list->items, ElementsAre(1, 2, 3));
}

TEST_F(RecompilerTest, Goto) {
  Emit(tk::Duplicate{.src = ls::Integer{1}, .dst = IntVar(0)},
       tk::Goto{.label = 0},
       tk::Duplicate{.src = ls::Integer{99}, .dst = IntVar(0)},
       tk::Label{.id = 0}, tk::Return{.ret_vals = {IntVar(0)}});

  EXPECT_EQ(Run(), 1);
}

TEST_F(RecompilerTest, GotoIfTrue) {
  Emit(tk::Duplicate{.src = ls::Integer{12}, .dst = IntVar(0)},
       tk::GotoIf{.label = 0, .cond = true, .src = ls::Integer{1}},
       tk::Duplicate{.src = ls::Integer{99}, .dst = IntVar(0)},
       tk::Label{.id = 0}, tk::Return{.ret_vals = {IntVar(0)}});

  EXPECT_EQ(Run(), 12);
}

TEST_F(RecompilerTest, GotoIfFalse) {
  Emit(tk::Duplicate{.src = ls::Integer{24}, .dst = IntVar(0)},
       tk::GotoIf{.label = 0, .cond = false, .src = ls::Integer{0}},
       tk::Duplicate{.src = ls::Integer{99}, .dst = IntVar(0)},
       tk::Label{.id = 0}, tk::Return{.ret_vals = {IntVar(0)}});

  EXPECT_EQ(Run(), 24);
}

TEST_F(RecompilerTest, SparseFastLocals) {
  Emit(tk::Duplicate{.src = ls::Integer{7}, .dst = IntVar(3)},
       tk::Return{.ret_vals = {IntVar(3)}});

  EXPECT_THAT(GetChunk()->fast_locals,
              ElementsAre("v0", "v1", "v2", "v3"));
  EXPECT_EQ(Run(), 7);
}

TEST_F(RecompilerTest, SparseFastLocalsAboveByteRange) {
  constexpr int kHighSlot = 300;

  Emit(tk::Duplicate{.src = ls::Integer{7}, .dst = IntVar(kHighSlot)},
       tk::Return{.ret_vals = {IntVar(kHighSlot)}});

  EXPECT_EQ(GetChunk()->fast_locals.size(),
            static_cast<std::size_t>(kHighSlot) + 1);
  EXPECT_EQ(GetChunk()->fast_locals.back(), "v300");
  EXPECT_EQ(Run(), 7);
}

TEST_F(RecompilerTest, Gosub) {
  Emit(tk::Gosub{.entry_id = 0, .args = {}, .dst = IntVar(0)},
       tk::Return{.ret_vals = {IntVar(0)}}, tk::Label{.id = 0},
       tk::Return{.ret_vals = {ls::Integer{5}}});

  EXPECT_EQ(Run(), 5);
}

TEST_F(RecompilerTest, GosubWithArgs) {
  Emit(tk::Gosub{.entry_id = 0,
                 .args = {ls::Integer{20}, ls::Integer{22}},
                 .dst = IntVar(0)},
       tk::Return{.ret_vals = {IntVar(0)}}, tk::Label{.id = 0},
       tk::Operate2{.op = ls::OperatorCode::Plus,
                    .lhs = IntVar(1),
                    .rhs = IntVar(2),
                    .dst = IntVar(3),
                    .val = std::nullopt},
       tk::Return{.ret_vals = {IntVar(3)}});

  EXPECT_THAT(GetChunk()->fast_locals,
              ElementsAre("v0", "v1", "v2", "v3"));
  EXPECT_EQ(Run(), 42);
}

TEST_F(RecompilerTest, DuplicateSubroutineDefinitions) {
  recompiler.Gen(tk::Token_t(tk::Subroutine{.name = "foo", .args = {}}));
  EXPECT_TRUE(recompiler.Ok());

  recompiler.Gen(tk::Token_t(tk::Subroutine{.name = "foo", .args = {}}));
  EXPECT_FALSE(recompiler.Ok());
}

}  // namespace siglus_test
