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

class ModuleManager {
 public:
  static ModuleManager CreatePrototype();

 public:
  ModuleManager() = default;

  void AttachModule(std::shared_ptr<RLModule> mod);

  std::shared_ptr<RLOperation> Dispatch(
      const libreallive::CommandElement&) const;
  std::string GetCommandName(const libreallive::CommandElement&) const;

  std::shared_ptr<RLModule> GetModule(int module_type, int module_id) const;
  std::shared_ptr<RLOperation> GetOperation(int module_type,
                                            int module_id,
                                            int opcode,
                                            int overload) const;

 private:
  std::map<std::pair<int, int>, std::shared_ptr<RLModule>> modules_;
  std::map<std::tuple<int, int, int, int>, std::shared_ptr<RLOperation>>
      cmd2operation_;
  std::multimap<std::string, std::shared_ptr<RLOperation>> name2operation_;
};
