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
// -----------------------------------------------------------------------

#include "core/gameexe.hpp"
#include "libreallive/archive.hpp"
#include "log/domain_logger.hpp"
#include "machine/dumper.hpp"
#include "utilities/file.hpp"
#include "utilities/string_utilities.hpp"

#include <atomic>
#include <boost/program_options.hpp>
#include <filesystem>
#include <format>
#include <fstream>
#include <future>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>

namespace fs = std::filesystem;
namespace po = boost::program_options;

int main(int argc, char* argv[]) {
  SetupLogging(Severity::Info);

  std::string output;
  std::string input;
  int jobs = 0;

  try {
    // Define options
    po::options_description desc("Allowed options");
    desc.add_options()("help,h", "Produce help message")(
        "output,o", po::value<std::string>(&output)->required(),
        "Specify output directory")(
        "input", po::value<std::string>(&input)->required(), "Input directory")(
        "jobs,j", po::value<int>(&jobs), "Jobs");

    // Positional arguments
    po::positional_options_description pos_desc;
    pos_desc.add("input", 1);

    // Parse arguments
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv)
                  .options(desc)
                  .positional(pos_desc)
                  .run(),
              vm);

    // Handle --help
    if (vm.count("help")) {
      std::cout << "rlkp" << '\n';
      std::cout << desc << '\n';
      return 0;
    }

    po::notify(vm);
  } catch (const po::error& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
    return 1;
  }

  fs::path game_root = input;
  if (!fs::exists(game_root)) {
    std::cerr << "ERROR: Path '" << game_root << "' does not exist."
              << std::endl;
    return -1;
  }

  if (jobs <= 0)
    jobs = 1;

  fs::path gameexe_path = CorrectPathCase(game_root / "Gameexe.ini");
  fs::path seen_path = CorrectPathCase(game_root / "Seen.txt");
  Gameexe gexe(gameexe_path);
  std::string regname = gexe("REGNAME");
  libreallive::Archive archive(seen_path, regname);

  fs::path output_path = output;
  regname = archive.regname_;

  std::atomic<bool> finished = false;
  std::mutex mtx;
  std::vector<std::future<void>> futures;
  std::queue<libreallive::Scenario*> que;

  futures.reserve(jobs);
  for (int i = 0; i < jobs; ++i)
    futures.emplace_back(std::async(std::launch::async, [&]() {
      while (true) {
        if (finished && que.empty())
          return;

        libreallive::Scenario* sc = nullptr;
        if (!que.empty()) {
          std::lock_guard<std::mutex> lock(mtx);
          if (que.empty())
            continue;
          sc = que.front();
          que.pop();
        }

        try {
          if (sc) {
            std::ofstream ofs(output_path / std::format("{}.{:04}.txt", regname,
                                                        sc->scene_number()));
            ofs << Dumper(sc).Doit() << std::endl;
          }
        } catch (std::exception& e) {
          static DomainLogger logger;
          logger(Severity::Error) << e.what();
        }
      }
    }));

  for (int i = 0; i < 10000; ++i) {
    libreallive::Scenario* scenario = archive.GetScenario(i);
    if (!scenario)
      continue;
    std::lock_guard<std::mutex> lock(mtx);
    que.push(scenario);
  }

  finished = true;
  for (auto& it : futures)
    it.get();

  return 0;
}
