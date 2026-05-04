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

#include "libsiglus/bindings/loader.hpp"

#include "libsiglus/archive.hpp"
#include "libsiglus/parser.hpp"
#include "libsiglus/parser_context.hpp"
#include "libsiglus/recompiler.hpp"
#include "libsiglus/scene.hpp"
#include "libsiglus/token.hpp"
#include "vm/exception.hpp"

#include <format>
#include <iostream>
#include <sstream>
#include <unordered_map>

namespace libsiglus::binding {
namespace sr = serilang;

struct Context : public ParserContext {
  Context(Archive& ar, Scene& sc, std::vector<token::Token_t>& tok)
      : ParserContext(ar, sc), tokens(tok) {}
  std::vector<token::Token_t>& tokens;
  void Emit(token::Token_t tok) final { tokens.emplace_back(std::move(tok)); }
  void Warn(std::string msg) final { std::cerr << msg << std::endl; }
};

sr::Module* Loader::Load(int scene) {
  const std::string scene_str = std::format("SCENE_{:04}", scene);
  if (auto it = scene_module_cache_.find(scene_str);
      it != scene_module_cache_.cend()) {
    return it->second;
  }

  Scene scn = archive.ParseScene(scene);
  std::vector<token::Token_t> tokens;
  Context ctx(archive, scn, tokens);
  Parser parser(ctx);
  try {
    parser.ParseAll();
  } catch (const std::exception& e) {
    throw sr::RuntimeError(std::format("failed to parse scene {} ({}): {}",
                                       scene, scn.scnname_, e.what()));
  }

  Recompiler compiler(vm.gc_);
  compiler.SetSceneProperties(scn.id_, scn.property);
  compiler.is_debug_ = true;  // TODO: Add an option to control debug flag
  for (auto& it : tokens)
    compiler.Gen(std::move(it));
  compiler.Finish();
  tokens.clear();

  if (!compiler.Ok()) {
    std::ostringstream errors;
    for (auto& it : compiler.GetErrors())
      errors << it.ToString() << '\n';
    throw sr::RuntimeError(std::format("failed to compile scene {} ({}):\n{}",
                                       scene, scn.scnname_, errors.str()));
  }

  compiler.module_->name = scn.scnname_;
  sr::Module* mod = compiler.module_;

  std::unordered_map<std::string, sr::Value> bootstrap_builtins;
  bootstrap_builtins.reserve(vm.globals_->size() + vm.builtins_->size());
  bootstrap_builtins.insert(vm.globals_->begin(), vm.globals_->end());
  bootstrap_builtins.insert(vm.builtins_->begin(), vm.builtins_->end());

  sr::VM bootstrap_vm(vm.gc_);
  bootstrap_vm.gc_threshold_ = 0;
  bootstrap_vm.globals_ = mod->globals;
  bootstrap_vm.builtins_ =
      std::make_shared<std::unordered_map<std::string, sr::Value>>(
          std::move(bootstrap_builtins));

  try {
    bootstrap_vm.Evaluate(compiler.GetCode());
  } catch (const std::exception& e) {
    throw sr::RuntimeError(std::format("failed to bootstrap scene {} ({}): {}",
                                       scene, scn.scnname_, e.what()));
  }

  scene_module_cache_[scene_str] = mod;
  (*vm.builtins_)["%%siglus_module@" + scene_str] = sr::Value(mod);

  return mod;
}

}  // namespace libsiglus::binding
