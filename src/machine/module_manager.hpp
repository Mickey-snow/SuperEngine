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

#include <memory>
#include <string>
#include <unordered_map>

namespace libreallive {
class CommandElement;
}
class RLModule;

class IModuleManager {
 public:
  virtual ~IModuleManager() = default;

  virtual void AttachModule(RLModule*) = 0;
  virtual std::string GetCommandName(
      const libreallive::CommandElement&) const = 0;
};

class ModuleManager : public IModuleManager {
 public:
  ModuleManager();
  ~ModuleManager();

  void AttachModule(RLModule* mod) override;
  void AttachModule(std::unique_ptr<RLModule> mod);

  RLModule* GetModule(int module_type, int module_id);

  std::string GetCommandName(
      const libreallive::CommandElement& f) const override;

 private:
  static int GetModuleHash(int module_type, int module_id) noexcept;

  std::unordered_map<int, std::unique_ptr<RLModule>> modules_;
};
