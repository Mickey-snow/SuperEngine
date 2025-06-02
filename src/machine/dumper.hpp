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

#include "core/gameexe.hpp"
#include "idumper.hpp"
#include "libreallive/archive.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace libreallive {
class Scenario;
}

class Dumper : public IDumper {
 public:
  Dumper(std::filesystem::path gexe_path, std::filesystem::path seen_path);

  std::vector<IDumper::Task> GetTasks() final;

 private:
  std::filesystem::path gexe_path_;
  std::filesystem::path seen_path_;
  Gameexe            gexe_;
  std::string        regname_;
  libreallive::Archive archive_;
};
