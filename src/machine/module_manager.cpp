// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2024 Serina Sakurai
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

#include "module_manager.hpp"

#include "libreallive/elements/command.hpp"
#include "machine/rlmodule.hpp"
#include "machine/rloperation.hpp"
#include "modules/modules.hpp"

#include <stdexcept>
#include <utility>

ModuleManager::ModuleManager() : modules_() {}

ModuleManager::~ModuleManager() = default;

void ModuleManager::AttachModule(RLModule* mod) {
  AttachModule(std::unique_ptr<RLModule>(mod));
}

void ModuleManager::AttachModule(std::unique_ptr<RLModule> mod) {
  if (!mod)
    return;

  const auto hash = GetModuleHash(mod->module_type(), mod->module_number());
  auto result = modules_.try_emplace(hash, std::move(mod));
  if (!result.second) {
    throw std::invalid_argument("Module hash clash: " + std::to_string(hash));
  }
}

RLModule* ModuleManager::GetModule(int module_type, int module_id) const {
  auto it = modules_.find(GetModuleHash(module_type, module_id));
  if (it != modules_.cend())
    return it->second.get();
  return nullptr;
}

RLOperation* ModuleManager::Dispatch(
    const libreallive::CommandElement& cmd) const {
  auto mod = GetModule(cmd.modtype(), cmd.module());
  if (mod == nullptr)
    return nullptr;

  return mod->Dispatch(cmd);
}

std::string ModuleManager::GetCommandName(
    const libreallive::CommandElement& f) const {
  auto op = Dispatch(f);
  if (op == nullptr)
    return {};
  return op->name();
}

int ModuleManager::GetModuleHash(int module_type, int module_id) noexcept {
  return (module_type << 8) | module_id;
}
