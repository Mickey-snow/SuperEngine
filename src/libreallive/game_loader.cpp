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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
// -----------------------------------------------------------------------

#include "libreallive/game_loader.hpp"

#include "core/gameexe.hpp"
#include "libreallive/archive.hpp"
#include "libreallive/scriptor.hpp"
#include "log/domain_logger.hpp"
#include "machine/debugger.hpp"
#include "machine/game_hacks.hpp"
#include "machine/rlmachine.hpp"
#include "machine/serialization.hpp"
#include "systems/event_system.hpp"
#include "systems/sdl/sdl_system.hpp"
#include "utilities/file.hpp"

#include <stdexcept>

namespace libreallive {

namespace fs = std::filesystem;

namespace {
fs::path FindGameFile(const std::filesystem::path& gamerootPath,
                      const std::string& filename) {
  fs::path search_for = gamerootPath / filename;
  fs::path corrected_path = CorrectPathCase(search_for);
  if (corrected_path.empty()) {
    throw std::runtime_error("Could not open " + search_for.string());
  }

  return corrected_path;
}

// AVG32 file checks. We can't run AVG32 games.
static const std::vector<std::string> avg32_exes{"avg3216m.exe",
                                                 "avg3217m.exe"};

// Siglus engine filenames. We can't run VisualArts' newer engine.
static const std::vector<std::string> siglus_exes{
    "siglus.exe", "siglusengine-ch.exe", "siglusengine.exe",
    "siglusenginechs.exe"};

void CheckBadEngine(const std::filesystem::path& gamerootPath,
                    const std::vector<std::string> filenames,
                    const std::string& message_text) {
  for (const auto& cur_file : filenames) {
    if (fs::exists(CorrectPathCase(gamerootPath / cur_file))) {
      throw std::runtime_error(message_text);
    }
  }
}

}  // namespace

GameLoader::GameLoader(fs::path gameroot) {
  fs::path gameexePath = FindGameFile(gameroot, "Gameexe.ini");
  fs::path seenPath = FindGameFile(gameroot, "Seen.txt");

  // Check for VisualArt's older and newer engines, which we can't emulate:
  CheckBadEngine(gameroot, avg32_exes, "Can't run AVG32 games");
  CheckBadEngine(gameroot, siglus_exes, "Can't run Siglus games");

  gameexe_ = std::make_shared<Gameexe>(gameexePath);
  (*gameexe_)("__GAMEPATH") = gameroot.string();

  archive_ = std::make_shared<libreallive::Archive>(seenPath.string(),
                                                    (*gameexe_)("REGNAME"));

  system_ = std::make_shared<SDLSystem>(*gameexe_);

  // Instantiate the rl machine
  auto memory = std::make_unique<Memory>();
  memory->LoadFrom(*gameexe_);
  int first_seen = -1;
  if ((*gameexe_).Exists("SEEN_START"))
    first_seen = (*gameexe_)("SEEN_START").ToInt();
  if (first_seen < 0) {
    // if SEEN_START is undefined, then just grab the first SEEN.
    first_seen = archive_->GetFirstScenarioID();
  }
  auto scriptor = std::make_shared<Scriptor>(*archive_);
  ScenarioConfig default_config;
  auto savepoint_decide = [&](std::string key) -> bool {
    int value = -1;
    if ((*gameexe_).Exists(key))
      value = (*gameexe_)(key);
    switch (value) {
      case 0:
        return false;
      case 1:
      default:
        return true;
    }
  };
  default_config.enable_message_savepoint =
      savepoint_decide("SAVEPOINT_MESSAGE");
  default_config.enable_seentop_savepoint =
      savepoint_decide("SAVEPOINT_SEENTOP");
  default_config.enable_selcom_savepoint = savepoint_decide("SAVEPOINT_SELCOM");
  scriptor->SetDefaultScenarioConfig(std::move(default_config));

  // instantiate virtual machine
  machine_ = std::make_shared<RLMachine>(
      *system_, scriptor, scriptor->Load(first_seen), std::move(memory));

  // instantiate debugger
  debugger_ = std::make_shared<Debugger>(*machine_);
  system_->event().AddListener(debugger_);

  // Load the "DLLs" required
  for (auto it : gameexe_->Filter("DLL.")) {
    const std::string& name = it.ToString("");
    try {
      std::string index_str = it.key().substr(it.key().find_first_of(".") + 1);
      int index = std::stoi(index_str);
      machine_->LoadDLL(index, name);
    } catch (std::exception& e) {
      static DomainLogger logger("GameLoader");
      logger(Severity::Warn)
          << "Don't know what to do with DLL '" << name << "'";
    }
  }
  AddGameHacks(*machine_);

  Serialization::loadGlobalMemory(*machine_);
}

}  // namespace libreallive
