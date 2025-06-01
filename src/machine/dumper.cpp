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

#include "machine/dumper.hpp"

#include "libreallive/elements/bytecode.hpp"
#include "libreallive/elements/command.hpp"
#include "libreallive/scenario.hpp"
#include "libreallive/visitors.hpp"
#include "machine/module_manager.hpp"

#include <format>
#include <functional>
#include <future>

namespace fs = std::filesystem;

static std::string DumpImpl(libreallive::Scenario* scenario) {
  const auto& script = scenario->script;
  std::map<unsigned long, int> in_degree;
  std::map<unsigned long, std::string> output;

  for (auto const& [loc, bytecode_ptr] : script.elements_) {
    in_degree.emplace(loc, 0);
  }

  for (auto const& [loc, bytecode_ptr] : script.elements_) {
    static auto prototype = ModuleManager::CreatePrototype();
    libreallive::DebugStringVisitor visitor(&prototype);

    auto ptr = bytecode_ptr->DownCast();

    output.emplace(loc, std::visit(visitor, ptr));

    if (std::holds_alternative<libreallive::CommandElement const*>(ptr)) {
      auto cmd = std::get<libreallive::CommandElement const*>(ptr);
      for (size_t i = 0; i < cmd->GetLocationCount(); ++i) {
        in_degree[cmd->GetLocation(i)]++;
      }
    }
  }

  std::string result;
  for (auto const& [loc, str] : output) {
    if (in_degree[loc] != 0) {
      result += ".L" + std::to_string(loc) + '\n';
    }
    result += str + '\n';
  }

  return result;
}

Dumper::Dumper(fs::path gexe_path, fs::path seen_path)
    : gexe_path_(std::move(gexe_path)),
      seen_path_(std::move(seen_path)),
      gexe_(gexe_path_),
      regname_(gexe_("REGNAME")),
      archive_(seen_path_, regname_) {
  regname_ = archive_.regname_;
}

std::vector<IDumper::Task> Dumper::GetTasks() {
  std::vector<IDumper::Task> result;

  for (int i = 0; i < 10000; ++i) {
    libreallive::Scenario* sc = archive_.GetScenario(i);
    if (!sc)
      continue;

    std::packaged_task<std::string()> task(std::bind(DumpImpl, sc));

    result.emplace_back(IDumper::Task{
        .name = std::format("{}.{:04}.txt", regname_, sc->scene_number()),
        .task = std::move(task)});
  }

  return result;
}
