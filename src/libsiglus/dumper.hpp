// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2025 Serina Sakurai
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

#include "core/asset_scanner.hpp"
#include "core/gameexe.hpp"
#include "idumper.hpp"
#include "libsiglus/archive.hpp"
#include "utilities/mapped_file.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace libsiglus {

class Dumper : public IDumper {
 public:
  Dumper(const std::filesystem::path& gexe_path,
         const std::filesystem::path& scene_path,
         const std::filesystem::path& root_path);

  std::vector<IDumper::Task> GetTasks(std::vector<int> scenarios) final;

 private:
  void DumpGexe(std::ostream& out);
  void DumpArchive(std::ostream& out);
  void DumpScene(size_t id, std::ostream& out);
  void DumpAudio(std::filesystem::path path, std::ostream& out);

  MappedFile gexe_data_, archive_data_;
  Gameexe gexe_;
  Archive archive_;
  AssetScanner scanner_;
};

}  // namespace libsiglus
