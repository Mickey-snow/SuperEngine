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

#include "libsiglus/archive.hpp"
#include "libsiglus/bindings/memory.hpp"
#include "libsiglus/recompiler.hpp"
#include "libsiglus/siglus_runtime.hpp"
#include "libsiglus/xorkey.hpp"

#include "libsiglus/intern_name.hpp"
#include "systems/sdl/sdl_system.hpp"
#include "vm/dict.hpp"
#include "vm/exception.hpp"
#include "vm/function.hpp"
#include "vm/instruction.hpp"
#include "vm/list.hpp"
#include "vm/object.hpp"
#include "vm/string.hpp"
#include "vm/vm.hpp"

#include <memory>
#include <sstream>
#include <unordered_map>
#include <vector>

namespace siglus_test {

namespace ls = libsiglus;
namespace sr = serilang;
namespace tk = libsiglus::token;

using ::testing::ElementsAre;
using ::testing::HasSubstr;

class RecompilerTest : public ::testing::Test {
 protected:
  std::shared_ptr<sr::GarbageCollector> gc =
      std::make_shared<sr::GarbageCollector>();
  ls::Recompiler recompiler{gc};

  static ls::Variable IntVar(int id) { return {ls::Type::Int, id}; }
  static ls::Variable StrVar(int id) { return {ls::Type::String, id}; }

  static ls::elm::AccessChain UserProperty(int scene, int idx) {
    return ls::elm::AccessChain{
        .root = ls::elm::Root(
            ls::Type::Int,
            ls::elm::Usrprop{.scene = scene, .idx = idx, .name = "flag"}),
        .nodes = {}};
  }

  template <typename... Ts>
  void Emit(Ts&&... toks) {
    (recompiler.Gen(tk::Token_t(std::forward<Ts>(toks))), ...);
    recompiler.Finish();
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
    sr::Code* chunk = recompiler.GetCode();
    EXPECT_NE(chunk, nullptr);
    return chunk;
  }

  sr::Function* GetGlobalFunction(std::string const& name) {
    auto it = recompiler.module_->globals->find(name);
    EXPECT_NE(it, recompiler.module_->globals->end());
    if (it == recompiler.module_->globals->end())
      return nullptr;
    auto* fn = it->second.Get_if<sr::Function>();
    EXPECT_NE(fn, nullptr);
    return fn;
  }

  void Bootstrap(sr::VM& vm) {
    vm.gc_threshold_ = 0;
    vm.globals_ = recompiler.module_->globals;
    vm.Evaluate(GetChunk());
  }

  sr::Value Call(sr::VM& vm,
                 sr::Value callee,
                 std::vector<sr::Value> args = {}) {
    sr::Code* thunk = gc->Allocate<sr::Code>();
    thunk->const_pool.emplace_back(callee);
    thunk->Append(sr::Push{0});
    for (sr::Value& arg : args) {
      thunk->const_pool.emplace_back(std::move(arg));
      thunk->Append(
          sr::Push{static_cast<uint32_t>(thunk->const_pool.size() - 1)});
    }
    thunk->Append(
        sr::Call{.argcnt = static_cast<uint32_t>(args.size()), .kwargcnt = 0});
    thunk->Append(sr::Return{});
    return vm.Evaluate(thunk);
  }

  sr::Value Run(std::unordered_map<std::string, sr::Value> globals = {},
                std::unordered_map<std::string, sr::Value> builtins = {}) {
    builtins.insert(globals.begin(), globals.end());
    sr::VM vm(gc, {}, std::move(builtins));
    Bootstrap(vm);
    return Call(vm, sr::Value(GetGlobalFunction("%%script")));
  }

  std::unordered_map<std::string, sr::Value> DirectUserCommandBuiltins() {
    std::unordered_map<std::string, sr::Value> builtins;
    builtins["__builtin_usrcmd"] = sr::Value(gc->Allocate<sr::NativeFunction>(
        "__builtin_usrcmd",
        [this](sr::VM&, sr::Fiber& f, uint8_t nargs,
               uint8_t nkwargs) -> sr::TempValue {
          EXPECT_EQ(nargs, 3);
          EXPECT_EQ(nkwargs, 0);

          const int* scene = f.op_stack.end()[-3].Get_if<int>();
          const int* entry = f.op_stack.end()[-2].Get_if<int>();
          const auto* name = f.op_stack.end()[-1].Get_if<sr::String>();
          EXPECT_NE(scene, nullptr);
          EXPECT_NE(entry, nullptr);
          EXPECT_NE(name, nullptr);
          f.op_stack.resize(f.op_stack.size() - 3);

          if (!scene || !entry || !name)
            throw sr::RuntimeError("__builtin_usrcmd received invalid args");

          auto it = recompiler.module_->globals->find(ls::GetUsercmdId(*entry));
          if (it == recompiler.module_->globals->end())
            throw sr::RuntimeError("missing command entry " +
                                   std::to_string(*entry));

          auto* command = it->second.Get_if<sr::Function>();
          if (!command)
            throw sr::RuntimeError("command entry is not callable");

          return sr::Value(command);
        }));
    return builtins;
  }

  sr::Module* MakeSceneModule(std::vector<sr::Value> properties) {
    auto* list = gc->Allocate<sr::List>(std::move(properties));

    auto* module = gc->Allocate<sr::Module>("SCENE");
    (*module->globals)["__usrprop"] = sr::Value(list);
    return module;
  }

  std::unordered_map<std::string, sr::Value> LoadSceneBuiltins(
      sr::Module* module,
      std::vector<int>* calls) {
    std::unordered_map<std::string, sr::Value> builtins;
    builtins["__builtin_load_scn"] = sr::Value(gc->Allocate<sr::NativeFunction>(
        "__builtin_load_scn",
        [module, calls](sr::VM&, sr::Fiber& f, uint8_t nargs,
                        uint8_t nkwargs) -> sr::TempValue {
          EXPECT_EQ(nargs, 1);
          EXPECT_EQ(nkwargs, 0);

          const int* scene = f.op_stack.back().Get_if<int>();
          EXPECT_NE(scene, nullptr);
          if (!scene)
            throw sr::RuntimeError("__builtin_load_scn received invalid args");
          calls->push_back(*scene);

          f.op_stack.resize(f.op_stack.size() - 1);
          return sr::Value(module);
        }));
    return builtins;
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

  EXPECT_THAT(GetChunk()->fast_locals, ElementsAre("v0", "v1", "v2"));
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

  EXPECT_THAT(GetChunk()->fast_locals, ElementsAre("v0", "v1", "v2", "v3"));
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

  EXPECT_THAT(GetChunk()->fast_locals, ElementsAre("v0", "v1", "v2", "v3"));
  EXPECT_EQ(Run(), 42);
}

TEST_F(RecompilerTest, CompiledChunkStartsWithJumpToBootstrap) {
  Emit(tk::Return{.ret_vals = {ls::Integer{7}}});

  sr::Code* chunk = GetChunk();
  ASSERT_FALSE(chunk->code.empty());
  ASSERT_EQ(static_cast<sr::OpCode>((*chunk)[0]), sr::OpCode::Jump);

  const auto jump = chunk->Read<sr::Jump>(1);
  const std::size_t bootstrap =
      1 + sizeof(sr::Jump) + static_cast<std::size_t>(jump.offset);
  ASSERT_LT(bootstrap, chunk->code.size());
  EXPECT_EQ(static_cast<sr::OpCode>((*chunk)[bootstrap]), sr::OpCode::Push);
}

TEST_F(RecompilerTest, BootstrapCreatesScriptAndCommandEntryFunctions) {
  Emit(
      tk::Subroutine{
          .name = "foo", .source_entry = 123, .args = {ls::Type::Int}},
      tk::Return{.ret_vals = {ls::Integer{9}}});

  sr::VM vm(gc);
  Bootstrap(vm);

  EXPECT_NE(GetGlobalFunction("%%script"), nullptr);
  sr::Function* command = GetGlobalFunction(libsiglus::GetUsercmdId(123));
  ASSERT_NE(command, nullptr);
  EXPECT_EQ(command->entry, recompiler.subroutine_entries_.at(123));
  EXPECT_EQ(command->arglist.nparam, 1u);
  EXPECT_EQ(GetGlobalFunction(libsiglus::GetUsercmdId(123)), command);
}

TEST_F(RecompilerTest, BootstrapInitializesSceneLocalProperties) {
  recompiler.SetSceneProperties(
      42, {ls::Property{.form = ls::Type::Int, .size = 0, .name = "flag"},
           ls::Property{.form = ls::Type::String, .size = 0, .name = "label"}});

  Emit(tk::Return{.ret_vals = {}});

  sr::VM vm(gc);
  Bootstrap(vm);

  auto usrprop_it = recompiler.module_->globals->find("__usrprop");
  ASSERT_NE(usrprop_it, recompiler.module_->globals->end());
  auto* usrprop = usrprop_it->second.Get_if<sr::List>();
  ASSERT_NE(usrprop, nullptr);
  ASSERT_THAT(usrprop->items, ElementsAre(0, ""));
}

TEST_F(RecompilerTest, CurrentSceneUserPropertyReadsFromBootstrappedUsrprop) {
  recompiler.SetSceneProperties(
      42, {ls::Property{.form = ls::Type::Int, .size = 0, .name = "flag"}});

  Emit(tk::GetProperty{.chain = UserProperty(42, 0), .dst = IntVar(0)},
       tk::Return{.ret_vals = {IntVar(0)}});

  EXPECT_EQ(Run(), 0);
}

TEST_F(RecompilerTest, GlobalUserPropertyReadsFromGlobalprop) {
  auto* globalprop =
      gc->Allocate<sr::List>(std::vector<sr::Value>{sr::Value(12)});

  Emit(tk::GetProperty{.chain = UserProperty(-1, 0), .dst = IntVar(0)},
       tk::Return{.ret_vals = {IntVar(0)}});

  EXPECT_EQ(Run({{"__globalprop", sr::Value(globalprop)}}), 12);
}

TEST_F(RecompilerTest, GlobalUserPropertyWritesToGlobalprop) {
  auto* globalprop =
      gc->Allocate<sr::List>(std::vector<sr::Value>{sr::Value(0)});

  Emit(tk::Assign{.dst = UserProperty(-1, 0), .src = ls::Integer{33}},
       tk::GetProperty{.chain = UserProperty(-1, 0), .dst = IntVar(0)},
       tk::Return{.ret_vals = {IntVar(0)}});

  EXPECT_EQ(Run({{"__globalprop", sr::Value(globalprop)}}), 33);
  EXPECT_THAT(globalprop->items, ElementsAre(33));
}

TEST_F(RecompilerTest, CurrentSceneUserPropertyWritesToBootstrappedUsrprop) {
  recompiler.SetSceneProperties(
      42, {ls::Property{.form = ls::Type::Int, .size = 0, .name = "flag"}});

  Emit(tk::Assign{.dst = UserProperty(42, 0), .src = ls::Integer{44}},
       tk::GetProperty{.chain = UserProperty(42, 0), .dst = IntVar(0)},
       tk::Return{.ret_vals = {IntVar(0)}});

  EXPECT_EQ(Run(), 44);
}

TEST_F(RecompilerTest, CrossSceneUserPropertyReadsFromLoadedModuleUsrprop) {
  recompiler.SetSceneProperties(42, {});
  sr::Module* scene43 = MakeSceneModule({sr::Value(55)});
  std::vector<int> load_calls;

  Emit(tk::GetProperty{.chain = UserProperty(43, 0), .dst = IntVar(0)},
       tk::Return{.ret_vals = {IntVar(0)}});

  EXPECT_EQ(Run({}, LoadSceneBuiltins(scene43, &load_calls)), 55);
  EXPECT_THAT(load_calls, ElementsAre(43));
}

TEST_F(RecompilerTest, CrossSceneUserPropertyWritesToLoadedModuleUsrprop) {
  recompiler.SetSceneProperties(42, {});
  sr::Module* scene43 = MakeSceneModule({sr::Value(0)});
  std::vector<int> load_calls;

  Emit(tk::Assign{.dst = UserProperty(43, 0), .src = ls::Integer{66}},
       tk::GetProperty{.chain = UserProperty(43, 0), .dst = IntVar(0)},
       tk::Return{.ret_vals = {IntVar(0)}});

  EXPECT_EQ(Run({}, LoadSceneBuiltins(scene43, &load_calls)), 66);
  EXPECT_THAT(load_calls, ElementsAre(43, 43));

  auto* usrprop = scene43->globals->at("__usrprop").Get_if<sr::List>();
  ASSERT_NE(usrprop, nullptr);
  EXPECT_THAT(usrprop->items, ElementsAre(66));
}

TEST_F(RecompilerTest, SceneRuntimeGlobalsUseCanonicalPropertyNamesOnly) {
  recompiler.SetSceneProperties(
      42, {ls::Property{.form = ls::Type::Int, .size = 0, .name = "flag"}});

  Emit(tk::Return{.ret_vals = {}});

  sr::VM vm(gc);
  Bootstrap(vm);

  EXPECT_TRUE(recompiler.module_->globals->contains("__usrprop"));
  for (const std::string& name :
       {std::string("__") + "locprop", std::string("get_") + "proplist",
        std::string("%%") + "usrprop", std::string("%%") + "scnprop"}) {
    EXPECT_FALSE(recompiler.module_->globals->contains(name)) << name;
  }
}

TEST_F(RecompilerTest, UserCommandCallWithNoPositionalArgsCallsFunction) {
  ls::elm::Usrcmd call;
  call.scene = 0;
  call.entry = 321;
  call.name = "zero";

  Emit(tk::Command{.chain =
                       ls::elm::AccessChain{.root = ls::elm::Root(
                                                ls::Type::Int, std::move(call)),
                                            .nodes = {}},
                   .dst = IntVar(0)},
       tk::Return{.ret_vals = {IntVar(0)}},
       tk::Subroutine{.name = "zero", .source_entry = 321, .args = {}},
       tk::Return{.ret_vals = {ls::Integer{77}}});

  EXPECT_EQ(Run({}, DirectUserCommandBuiltins()), 77);
}

TEST_F(RecompilerTest, UserCommandCallWithOnePositionalArgPassesScalar) {
  ls::elm::Usrcmd call;
  call.scene = 0;
  call.entry = 322;
  call.name = "identity";
  call.arguments.arg = {ls::Integer{123}};

  Emit(tk::Command{.chain =
                       ls::elm::AccessChain{.root = ls::elm::Root(
                                                ls::Type::Int, std::move(call)),
                                            .nodes = {}},
                   .dst = IntVar(0)},
       tk::Return{.ret_vals = {IntVar(0)}},
       tk::Subroutine{
           .name = "identity", .source_entry = 322, .args = {ls::Type::Int}},
       tk::Return{.ret_vals = {IntVar(1)}});

  EXPECT_EQ(Run({}, DirectUserCommandBuiltins()), 123);
}

TEST_F(RecompilerTest, UserCommandCallWithMultiplePositionalArgsFillsSlots) {
  ls::elm::Usrcmd call;
  call.scene = 0;
  call.entry = 323;
  call.name = "pack";
  call.arguments.arg = {ls::Integer{10}, ls::Integer{20}, ls::Integer{30}};

  Emit(tk::Command{.chain = ls::elm::AccessChain{.root = ls::elm::Root(
                                                     ls::Type::IntList,
                                                     std::move(call)),
                                                 .nodes = {}},
                   .dst = IntVar(0)},
       tk::Return{.ret_vals = {IntVar(0)}},
       tk::Subroutine{.name = "pack",
                      .source_entry = 323,
                      .args = {ls::Type::Int, ls::Type::Int, ls::Type::Int}},
       tk::Return{.ret_vals = {IntVar(1), IntVar(2), IntVar(3)}});

  sr::Value result = Run({}, DirectUserCommandBuiltins());
  const auto* list = result.Get_if<sr::List>();
  ASSERT_NE(list, nullptr);
  EXPECT_THAT(list->items, ElementsAre(10, 20, 30));
}

TEST_F(RecompilerTest, UserCommandCallWithNamedArgsFailsWhenExecuted) {
  ls::elm::Usrcmd call;
  call.scene = 0;
  call.entry = 324;
  call.name = "tagged";
  call.arguments.named_arg = {{7, ls::Integer{99}}};

  Emit(tk::Command{.chain =
                       ls::elm::AccessChain{.root = ls::elm::Root(
                                                ls::Type::Int, std::move(call)),
                                            .nodes = {}},
                   .dst = IntVar(0)},
       tk::Return{.ret_vals = {IntVar(0)}},
       tk::Subroutine{.name = "tagged", .source_entry = 324, .args = {}},
       tk::Return{.ret_vals = {ls::Integer{1}}});

  sr::VM vm(gc, {}, DirectUserCommandBuiltins());
  Bootstrap(vm);

  try {
    (void)Call(vm, sr::Value(GetGlobalFunction("%%script")));
    FAIL() << "expected unsupported named/tagged user-command args to fail";
  } catch (const sr::UnhandledError& e) {
    EXPECT_THAT(e.what(),
                HasSubstr("unsupported named/tagged user-command arguments"));
  }
}

TEST_F(RecompilerTest, DuplicateSubroutineDefinitions) {
  recompiler.Gen(
      tk::Token_t(tk::Subroutine{.name = "foo", .source_entry = 10}));
  EXPECT_TRUE(recompiler.Ok());

  recompiler.Gen(
      tk::Token_t(tk::Subroutine{.name = "bar", .source_entry = 10}));
  EXPECT_FALSE(recompiler.Ok());
}

TEST(SiglusMemoryBindingTest, InstallsCanonicalGlobalPropertiesOnly) {
  auto gc = std::make_shared<sr::GarbageCollector>();
  std::string raw_archive(sizeof(ls::Pack_hdr), '\0');
  auto archive = std::make_shared<ls::Archive>(std::string_view(raw_archive),
                                               ls::empty_key);
  archive->prop_ = {
      ls::Property{.form = ls::Type::Int, .size = 0, .name = "flag"},
      ls::Property{.form = ls::Type::String, .size = 0, .name = "label"}};

  ls::SiglusRuntime runtime;
  runtime.vm = std::make_unique<sr::VM>(gc);
  runtime.archive = std::move(archive);

  ls::binding::Context ctx;
  ls::binding::Memory memory(ctx);
  memory.Bind(runtime);

  auto& globals = *runtime.vm->globals_;
  auto globalprop_it = globals.find("__globalprop");
  ASSERT_NE(globalprop_it, globals.end());
  auto* globalprop = globalprop_it->second.Get_if<sr::List>();
  ASSERT_NE(globalprop, nullptr);
  EXPECT_THAT(globalprop->items, ElementsAre(0, ""));

  EXPECT_FALSE(globals.contains("__usrprop"));
  for (const std::string& name :
       {std::string("__") + "locprop", std::string("get_") + "proplist",
        std::string("%%") + "usrprop", std::string("%%") + "scnprop"}) {
    EXPECT_FALSE(globals.contains(name)) << name;
  }
}

}  // namespace siglus_test
