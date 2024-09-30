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

#include <string>

class IModuleManager {
 public:
  virtual ~IModuleManager() = default;

  virtual void AttachModule(RLModule*) = 0;
  virtual std::string GetCommandName(
      const libreallive::CommandElement&) const = 0;
};

class ModuleManager : public IModuleManager {
 public:
  ModuleManager() = default;
  ~ModuleManager() = default;

  void AttachModule(RLModule* mod) override {}
  std::string GetCommandName(
      const libreallive::CommandElement& f) const override {
    return "ModuleManager::GetCommandName called.";
  }
};

#endif
