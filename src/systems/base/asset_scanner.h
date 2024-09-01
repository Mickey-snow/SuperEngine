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

#ifndef SRC_SYSTEMS_BASE_RLFILESYSTEM_H_
#define SRC_SYSTEMS_BASE_RLFILESYSTEM_H_

#include "libreallive/gameexe.h"

#include <filesystem>
#include <map>
#include <set>

class IAssetScanner {
 public:
  virtual ~IAssetScanner() = default;

  virtual std::filesystem::path FindFile(
      std::string filename,
      const std::set<std::string>& extension_filter = {}) = 0;
};

class AssetScanner : public IAssetScanner {
 public:
  AssetScanner() = default;
  AssetScanner(Gameexe& gexe);
  ~AssetScanner() = default;

  /* read and scan all the directories defined in the #FOLDNAME section. */
  void BuildFromGameexe(Gameexe& gexe);

  /* recursively scan directory, index all recongnized files */
  void IndexDirectory(const std::filesystem::path& dir,
                      const std::set<std::string>& extension_filter = {});

  std::filesystem::path FindFile(
      std::string filename,
      const std::set<std::string>& extension_filter = {}) override;

  /* mapping lowercase filename to extension and path pairs */
  using fs_cache_t =
      std::multimap<std::string, std::pair<std::string, std::filesystem::path>>;
  fs_cache_t filesystem_cache_;
};

#endif
