// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
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
//
// -----------------------------------------------------------------------

#include "sgvm_instance.hpp"

#include "libsiglus/archive.hpp"
#include "libsiglus/parser.hpp"
#include "libsiglus/parser_context.hpp"
#include "libsiglus/recompiler.hpp"
#include "libsiglus/sgvm_factory.hpp"
#include "libsiglus/token.hpp"
#include "vm/function.hpp"
#include "vm/instruction.hpp"
#include "vm/object.hpp"
#include "vm/vm.hpp"

#include <exception>
#include <filesystem>
#include <iostream>
#include <stdexcept>

using namespace libsiglus;
namespace sr = serilang;

struct Context : public ParserContext {
  Context(Archive& ar, Scene& sc, std::vector<token::Token_t>& tok)
      : ParserContext(ar, sc), tokens(tok) {}
  std::vector<token::Token_t>& tokens;
  void Emit(token::Token_t tok) final { tokens.emplace_back(std::move(tok)); }
  void Warn(std::string msg) final { std::cerr << msg << std::endl; }
};

void SgvmInstance::Main(const std::filesystem::path& game_root) {
  try {
    SiglusRuntime rt = SGVMFactory(game_root).Create();
    sr::VM& vm = *rt.vm;

    sr::Module* mod = rt.loader->Load(start_scene_);
    if (!mod) {
      std::string errmsg =
          std::format("Failed to parse start scene: {}", start_scene_);
      throw std::runtime_error(std::move(errmsg));
    }

    sr::Function* script = (*mod->globals)["%%script"].Get_if<sr::Function>();
    if (!script) {
      std::string errmsg =
          std::format("Scene {}: %%script not found", start_scene_);
      throw std::runtime_error(std::move(errmsg));
    }

    sr::Code* thunk = vm.gc_->Allocate<sr::Code>();
    thunk->const_pool.emplace_back(script);
    thunk->Append(sr::Push{0});
    thunk->Append(sr::Call{.argcnt = 0, .kwargcnt = 0});
    thunk->Append(sr::Return{});

    sr::Value result = vm.Evaluate(thunk);
    std::cout << "Result is: " << result.Desc() << std::endl;

  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
}
