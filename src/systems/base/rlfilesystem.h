
#ifndef SRC_SYSTEMS_BASE_RLFILESYSTEM_H_
#define SRC_SYSTEMS_BASE_RLFILESYSTEM_H_

#include "libreallive/gameexe.h"

#include <boost/algorithm/string.hpp>
#include <filesystem>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class rlFileSystem {
 public:
  rlFileSystem() = default;
  rlFileSystem(Gameexe& gexe) { BuildFromGameexe(gexe); }
  ~rlFileSystem() = default;

  /* read and scan all the directories defined in the #FOLDNAME section. */
  void BuildFromGameexe(Gameexe& gexe) {
    namespace fs = std::filesystem;

    std::set<std::string> valid_directories;
    for (GameexeFilteringIterator it = gexe.FilterBegin("FOLDNAME"),
                                  end = gexe.FilterEnd();
         it != end; ++it) {
      std::string dir = it->ToString();
      if (!dir.empty()) {
        boost::to_lower(dir);
        valid_directories.insert(dir);
      }
    }

    static const std::set<std::string> rlvm_file_types{
        "g00", "pdt", "anm", "gan", "hik", "wav",
        "ogg", "nwa", "mp3", "ovk", "koe", "nwk"};

    fs::path gamepath;
    // TODO: the key '__GAMEPATH' might not be set correctly in full system
    // tests, remove those tests (as they really aren't testing anything),
    // remove underhanded xxxSystem subclasses, and throw a proper error here.
    try {
      gamepath = gexe("__GAMEPATH").ToString();
    } catch (...) {
      return;
    }

    for (const auto& dir : fs::directory_iterator(gamepath)) {
      if (fs::is_directory(dir.status())) {
        std::string lowername = dir.path().filename().string();
        boost::to_lower(lowername);
        if (valid_directories.count(lowername))
          IndexDirectory(dir, rlvm_file_types);
      }
    }
  }

  /* recursively scan directory, index all recongnized files */
  void IndexDirectory(const std::filesystem::path& dir,
                      const std::set<std::string>& extension_filter = {}) {
    namespace fs = std::filesystem;

    if (!fs::exists(dir) || !fs::is_directory(dir))
      throw std::invalid_argument("The provided path " + dir.string() +
                                  " is not a valid directory.");

    try {
      for (const auto& entry : fs::recursive_directory_iterator(dir)) {
        if (!fs::is_regular_file(entry))
          continue;

        std::string extension = entry.path().extension().string();
        if (extension.size() > 1 && extension[0] == '.')
          extension = extension.substr(1);
        boost::to_lower(extension);
        if (!extension_filter.empty() && extension_filter.count(extension) == 0)
          continue;

        std::string stem = entry.path().stem().string();
        boost::to_lower(stem);

        filesystem_cache_.emplace(stem,
                                  std::make_pair(extension, entry.path()));
      }

    } catch (const fs::filesystem_error& e) {
      std::ostringstream os;
      os << "Filesystem error: " << e.what();
      os << " at iterating over directory " << dir.string() << '.';
      throw std::runtime_error(os.str());
    }
  }

  std::filesystem::path FindFile(
      std::string filename,
      const std::set<std::string>& extension_filter = {}) {
    // Hack to get around fileNames like "REALNAME?010", where we only
    // want REALNAME.
    filename = filename.substr(0, filename.find('?'));
    boost::to_lower(filename);

    const auto [begin, end] = filesystem_cache_.equal_range(filename);
    for (auto it = begin; it != end; ++it) {
      const auto [extension, path] = it->second;
      if (extension_filter.empty()  // no filter applied
          || extension_filter.count(extension))
        return path;
    }

    throw std::runtime_error("rlFileSystem::FindFile: file " + filename +
                             " not found.");
  }

  /* mapping lowercase filename to extension and path pairs */
  using fs_cache_t =
      std::multimap<std::string, std::pair<std::string, std::filesystem::path>>;
  fs_cache_t filesystem_cache_;
};

#endif
