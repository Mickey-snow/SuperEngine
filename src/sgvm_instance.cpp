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
#include "vm/vm.hpp"

#include <exception>
#include <filesystem>
#include <iostream>

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
    Archive& archive = *rt.archive;

    Scene scn = archive.ParseScene(start_scene_);
    std::vector<token::Token_t> tokens;
    Context ctx(archive, scn, tokens);
    Parser parser(ctx);

    parser.ParseAll();

    sr::VM& vm = *rt.vm;

    Recompiler compiler(rt.vm->gc_);
    compiler.is_debug_ = true;
    for (auto& it : tokens)
      compiler.Gen(std::move(it));
    tokens.clear();

    if (!compiler.Ok()) {
      for (auto& it : compiler.GetErrors())
        std::cerr << it.ToString() << std::endl;
      return;
    }

    sr::Code* code = compiler.chunk_;
    sr::Value result = vm.Evaluate(code);

    std::cout << "Result is: " << result.Desc() << std::endl;

  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
}
