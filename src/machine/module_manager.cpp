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

RLModule* ModuleManager::GetModule(int module_type, int module_id) {
  auto it = modules_.find(GetModuleHash(module_type, module_id));
  if (it != modules_.end())
    return it->second.get();
  return nullptr;
}

std::string ModuleManager::GetCommandName(
    const libreallive::CommandElement& f) const {
  const auto hash = GetModuleHash(f.modtype(), f.module());
  auto it = modules_.find(hash);
  if (it == modules_.cend())
    return {};
  return it->second->GetCommandName(f);
}

int ModuleManager::GetModuleHash(int module_type, int module_id) noexcept {
  return (module_type << 8) | module_id;
}
