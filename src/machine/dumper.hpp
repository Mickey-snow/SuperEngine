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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
// -----------------------------------------------------------------------

#pragma once

#include "libreallive/elements/bytecode.hpp"
#include "libreallive/elements/command.hpp"
#include "libreallive/scenario.hpp"
#include "libreallive/visitors.hpp"
#include "machine/module_manager.hpp"
#include "modules/modules.hpp"

#include <format>
#include <map>
#include <string>

namespace libreallive {
class Scenario;
}

// A really cheap disassembler
class Dumper {
 public:
  Dumper(libreallive::Scenario* scenario) : scenario_(scenario) {}

  std::string Doit() {
    const auto& script = scenario_->script;
    std::map<unsigned long, int> in_degree;
    std::map<unsigned long, std::string> output;

    for (auto [loc, bytecode] : script.elements_)
      in_degree.emplace(loc, 0);
    for (auto [loc, bytecode] : script.elements_) {
      static ModuleManager manager = []() {
        ModuleManager manager;
        AddAllModules(manager);
        return manager;
      }();
      libreallive::DebugStringVisitor visitor(&manager);

      auto ptr = bytecode->DownCast();

      output.emplace(loc, std::visit(visitor, ptr));
      if (std::holds_alternative<libreallive::CommandElement const*>(ptr)) {
        auto cmd = std::get<libreallive::CommandElement const*>(ptr);
        for (size_t i = 0; i < cmd->GetLocationCount(); ++i)
          in_degree[cmd->GetLocation(i)]++;
      }
    }

    std::string result;
    for (const auto& [loc, str] : output) {
      if (in_degree[loc])
        result += ".L" + std::to_string(loc) + '\n';
      result += str + '\n';
    }

    return result;
  }

 private:
  libreallive::Scenario* scenario_;
};
