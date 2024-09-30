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

#ifndef SRC_MACHINE_MODULE_MANAGER_H_
#define SRC_MACHINE_MODULE_MANAGER_H_

#include "libreallive/elements/command.h"
#include "machine/rlmodule.h"
#include "modules/modules.h"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

class IModuleManager {
 public:
  virtual ~IModuleManager() = default;

  virtual void AttachModule(RLModule*) = 0;
  virtual std::string GetCommandName(
      const libreallive::CommandElement&) const = 0;
};

class ModuleManager : public IModuleManager {
 public:
  ModuleManager() : modules_() {};
  ~ModuleManager() = default;

  void AttachModule(RLModule* mod) override {
    AttachModule(std::unique_ptr<RLModule>(mod));
  }

  void AttachModule(std::unique_ptr<RLModule> mod) {
    if (!mod)
      return;

    const auto hash = GetModuleHash(mod->module_type(), mod->module_number());
    auto result = modules_.try_emplace(hash, std::move(mod));
    if (!result.second) {
      throw std::invalid_argument("Module hash clash: " + std::to_string(hash));
    }
  }

  RLModule* GetModule(int module_type, int module_id) {
    auto it = modules_.find(GetModuleHash(module_type, module_id));
    if (it != modules_.end())
      return it->second.get();
    return nullptr;
  }

  std::string GetCommandName(
      const libreallive::CommandElement& f) const override {
    const auto hash = GetModuleHash(f.modtype(), f.module());
    auto it = modules_.find(hash);
    if (it == modules_.cend())
      return {};
    return it->second->GetCommandName(f);
  }

 private:
  static int GetModuleHash(int module_type, int module_id) noexcept {
    return (module_type << 8) | module_id;
  }

  std::unordered_map<int, std::unique_ptr<RLModule>> modules_;
};

#endif
