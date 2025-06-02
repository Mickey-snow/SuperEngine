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
using namespace std::placeholders;

static void DumpImpl(libreallive::Scenario* scenario, std::ostream& out) {
  const auto& script = scenario->script;
  std::map<unsigned long, int> in_degree;
  std::map<unsigned long, std::string> lines;

  /* first pass – collect vertices and initialise in-degree */
  for (auto const& [loc, bytecode_ptr] : script.elements_)
    in_degree.emplace(loc, 0);

  /* second pass – produce textual representation & graph edges */
  for (auto const& [loc, bytecode_ptr] : script.elements_) {
    static auto prototype = ModuleManager::CreatePrototype();
    libreallive::DebugStringVisitor visitor(&prototype);

    auto ptr = bytecode_ptr->DownCast();
    lines.emplace(loc, std::visit(visitor, ptr));

    if (std::holds_alternative<libreallive::CommandElement const*>(ptr)) {
      auto cmd = std::get<libreallive::CommandElement const*>(ptr);
      for (size_t i = 0; i < cmd->GetLocationCount(); ++i)
        ++in_degree[cmd->GetLocation(i)];
    }
  }

  /* final pass – emit */
  for (auto const& [loc, txt] : lines) {
    if (in_degree[loc] != 0)
      out << ".L" << loc << '\n';
    out << txt << '\n';
  }
}

Dumper::Dumper(fs::path gexe_path, fs::path seen_path)
    : gexe_path_(std::move(gexe_path)),
      seen_path_(std::move(seen_path)),
      gexe_(gexe_path_),
      regname_(gexe_("REGNAME")),
      archive_(seen_path_, regname_) {
  regname_ = archive_.regname_;  // ensure canonical name
}

std::vector<IDumper::Task> Dumper::GetTasks() {
  std::vector<IDumper::Task> tasks;

  using packaged = std::packaged_task<void(std::ostream&)>;

  for (int i = 0; i < 10'000; ++i) {
    libreallive::Scenario* sc = archive_.GetScenario(i);
    if (!sc)
      continue;

    packaged job(std::bind(DumpImpl, sc, _1));

    tasks.push_back(IDumper::Task{
        .name = std::format("{}.{:04}.txt", regname_, sc->scene_number()),
        .task = std::move(job)});
  }

  return tasks;
}
