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

#pragma once

#include <map>
#include <memory>
#include <string>

namespace libreallive {
class CommandElement;
}
class RLModule;
class RLOperation;

class IModuleManager {
 public:
  virtual ~IModuleManager() = default;

  virtual RLModule* GetModule(int module_type, int module_id) const = 0;
  virtual std::string GetCommandName(
      const libreallive::CommandElement&) const = 0;
  virtual RLOperation* Dispatch(const libreallive::CommandElement&) const = 0;
};

class ModuleManager {
 public:
  ModuleManager() = delete;

  static void AttachModule(std::unique_ptr<RLModule> mod);
  static void DetachAll();

  static IModuleManager& GetInstance();

 private:
  struct Ctx {
    std::map<std::pair<int, int>, std::unique_ptr<RLModule>> modules_;
  };
  static Ctx& Get();
};
