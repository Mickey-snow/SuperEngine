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

#include "base/asset_scanner.hpp"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

void to_lower(std::string& input) {
  std::transform(input.begin(), input.end(), input.begin(),
                 [](unsigned char c) { return std::tolower(c); });
}

AssetScanner::AssetScanner(Gameexe& gexe) { BuildFromGameexe(gexe); }

// Builds file system from Gameexe object
void AssetScanner::BuildFromGameexe(Gameexe& gexe) {
  namespace fs = std::filesystem;

  std::set<std::string> valid_directories;
  for (auto it : gexe.Filter("FOLDNAME")) {
    std::string dir = it.ToString();
    if (!dir.empty()) {
      to_lower(dir);
      valid_directories.insert(dir);
    }
  }

  static const std::set<std::string> rlvm_file_types{
      "g00", "pdt", "anm", "gan", "hik", "wav",
      "ogg", "nwa", "mp3", "ovk", "koe", "nwk"};

  fs::path gamepath;
  if (!gexe("__GAMEPATH").Exists())
    throw std::runtime_error("AssetScanner: __GAMEPATH not exist.");

  gamepath = gexe("__GAMEPATH").ToString();

  for (const auto& dir : fs::directory_iterator(gamepath)) {
    if (fs::is_directory(dir.status())) {
      std::string lowername = dir.path().filename().string();
      to_lower(lowername);
      if (valid_directories.count(lowername))
        IndexDirectory(dir, rlvm_file_types);
    }
  }
}

void AssetScanner::IndexDirectory(
    const std::filesystem::path& dir,
    const std::set<std::string>& extension_filter) {
  namespace fs = std::filesystem;

  if (!fs::exists(dir) || !fs::is_directory(dir))
    throw std::invalid_argument("AssetScanner: The provided path " +
                                dir.string() + " is not a valid directory.");

  try {
    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
      if (!fs::is_regular_file(entry))
        continue;

      std::string extension = entry.path().extension().string();
      if (extension.size() > 1 && extension[0] == '.')
        extension = extension.substr(1);
      to_lower(extension);
      if (!extension_filter.empty() && extension_filter.count(extension) == 0)
        continue;

      std::string stem = entry.path().stem().string();
      to_lower(stem);

      filesystem_cache_.emplace(stem, std::make_pair(extension, entry.path()));
    }

  } catch (const fs::filesystem_error& e) {
    std::ostringstream os;
    os << "Filesystem error: " << e.what();
    os << " at iterating over directory " << dir.string() << '.';
    throw std::runtime_error(os.str());
  }
}

std::filesystem::path AssetScanner::FindFile(
    std::string filename,
    const std::set<std::string>& extension_filter) {
  // Hack to get around fileNames like "REALNAME?010", where we only
  // want REALNAME.
  filename = filename.substr(0, filename.find('?'));
  to_lower(filename);

  const auto [begin, end] = filesystem_cache_.equal_range(filename);
  for (auto it = begin; it != end; ++it) {
    const auto [extension, path] = it->second;
    if (extension_filter.empty()  // no filter applied
        || extension_filter.count(extension))
      return path;
  }

  throw std::runtime_error("AssetScanner::FindFile: file " + filename +
                           " not found.");
}
