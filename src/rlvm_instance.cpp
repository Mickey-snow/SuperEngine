// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2011 Elliot Glaysher
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

#include "rlvm_instance.hpp"

#include "base/gameexe.hpp"
#include "libreallive/reallive.hpp"
#include "libreallive/scriptor.hpp"
#include "machine/game_hacks.hpp"
#include "machine/rlmachine.hpp"
#include "machine/serialization.hpp"
#include "memory/memory.hpp"
#include "memory/serialization_global.hpp"
#include "memory/serialization_local.hpp"
#include "modules/module_sys_save.hpp"
#include "platforms/implementor.hpp"
#include "systems/base/event_system.hpp"
#include "systems/base/graphics_system.hpp"
#include "systems/base/system_error.hpp"
#include "systems/sdl/sdl_system.hpp"
#include "utf8cpp/utf8.h"
#include "utilities/exception.hpp"
#include "utilities/file.hpp"
#include "utilities/find_font_file.hpp"
#include "utilities/gettext.hpp"
#include "utilities/string_utilities.hpp"

#include <iostream>
#include <string>
#include <thread>

namespace fs = std::filesystem;

// AVG32 file checks. We can't run AVG32 games.
static const std::vector<std::string> avg32_exes{"avg3216m.exe",
                                                 "avg3217m.exe"};

// Siglus engine filenames. We can't run VisualArts' newer engine.
static const std::vector<std::string> siglus_exes{
    "siglus.exe", "siglusengine-ch.exe", "siglusengine.exe",
    "siglusenginechs.exe"};

RLVMInstance::RLVMInstance()
    : seen_start_(-1),
      memory_(false),
      load_save_(-1),
      platform_implementor_(nullptr) {
  srand(time(NULL));
}

RLVMInstance::~RLVMInstance() {}

void RLVMInstance::SetPlatformImplementor(
    std::shared_ptr<IPlatformImplementor> impl) {
  platform_implementor_ = impl;
}

void RLVMInstance::Run(const std::filesystem::path& gamerootPath) {
  try {
    fs::path gameexePath = FindGameFile(gamerootPath, "Gameexe.ini");
    fs::path seenPath = FindGameFile(gamerootPath, "Seen.txt");

    // Check for VisualArt's older and newer engines, which we can't emulate:
    CheckBadEngine(gamerootPath, avg32_exes, _("Can't run AVG32 games"));
    CheckBadEngine(gamerootPath, siglus_exes, _("Can't run Siglus games"));

    Gameexe gameexe(gameexePath);
    gameexe("__GAMEPATH") = gamerootPath.string();

    // Possibly force starting at a different seen
    if (seen_start_ != -1)
      gameexe("SEEN_START") = seen_start_;

    if (memory_)
      gameexe("MEMORY") = 1;

    if (!custom_font_.empty()) {
      if (!fs::exists(custom_font_)) {
        throw rlvm::UserPresentableError(
            _("Could not open font file."),
            _("Please make sure the font file specified with --font exists and "
              "is a TrueType font."));
      }

      gameexe("__GAMEFONT") = custom_font_;
    }

    libreallive::Archive arc(seenPath.string(), gameexe("REGNAME"));
    SDLSystem sdlSystem(gameexe);

    // Instantiate the rl machine
    auto memory = std::make_unique<Memory>();
    memory->LoadFrom(gameexe);
    int first_seen = -1;
    if (gameexe.Exists("SEEN_START"))
      first_seen = gameexe("SEEN_START").ToInt();
    if (first_seen < 0) {
      // if SEEN_START is undefined, then just grab the first SEEN.
      first_seen = arc.GetFirstScenarioID();
    }
    auto scriptor = std::make_shared<libreallive::Scriptor>(arc);
    ScenarioConfig default_config;
    auto savepoint_decide = [&gameexe](std::string key) -> bool {
      int value = -1;
      if (gameexe.Exists(key))
        value = gameexe(key);
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
    default_config.enable_selcom_savepoint =
        savepoint_decide("SAVEPOINT_SELCOM");
    scriptor->SetDefaultScenarioConfig(std::move(default_config));
    RLMachine rlmachine(sdlSystem, scriptor, scriptor->Load(first_seen),
                        std::move(memory));
    // Load the "DLLs" required
    for (auto it : gameexe.Filter("DLL.")) {
      const std::string& name = it.ToString("");
      try {
        std::string index_str =
            it.key().substr(it.key().find_first_of(".") + 1);
        int index = std::stoi(index_str);
        rlmachine.LoadDLL(index, name);
      } catch (rlvm::Exception& e) {
        std::cerr << "WARNING: Don't know what to do with DLL '" << name << "'"
                  << std::endl;
      }
    }
    AddGameHacks(rlmachine);

    // Validate our font file
    // TODO(erg): Remove this when we switch to native font selection dialogs.
    fs::path fontFile = FindFontFile(sdlSystem);
    if (fontFile.empty() || !fs::exists(fontFile)) {
      throw rlvm::UserPresentableError(
          _("Could not find msgothic.ttc or a suitable fallback font."),
          _("Please place a copy of msgothic.ttc in either your home directory "
            "or in the game path."));
    }

    Serialization::loadGlobalMemory(rlmachine);

    // Now to preform a quick integrity check. If the user opened the Japanese
    // version of CLANNAD (or any other game), and then installed a patch, our
    // user data is going to be screwed!
    DoUserNameCheck(rlmachine, arc.GetProbableEncodingType());

    if (load_save_ != -1)
      Sys_load()(rlmachine, load_save_);

    while (!rlmachine.IsHalted()) {
      // Give SDL a chance to respond to events, redraw the screen,
      // etc.
      sdlSystem.Run(rlmachine);

      // Run the rlmachine through as many instructions as we can in a 10ms time
      // slice. Bail out if we switch to long operation mode, or if the screen
      // is marked as dirty.
      unsigned int start_ticks = sdlSystem.event().GetTicks();
      unsigned int end_ticks = start_ticks;
      do {
        rlmachine.ExecuteNextInstruction();
        end_ticks = sdlSystem.event().GetTicks();
      } while (!rlmachine.CurrentLongOperation() && !sdlSystem.force_wait() &&
               (end_ticks - start_ticks < 10));

      // Sleep to be nice to the processor and to give the GPU a chance to
      // catch up.
      if (!sdlSystem.ShouldFastForward()) {
        int real_sleep_time = 10 - (end_ticks - start_ticks);
        if (real_sleep_time < 1)
          real_sleep_time = 1;

        std::this_thread::sleep_for(std::chrono::milliseconds(real_sleep_time));
      }

      sdlSystem.set_force_wait(false);
    }

    Serialization::saveGlobalMemory(rlmachine);
  } catch (rlvm::UserPresentableError& e) {
    ReportFatalError(e.message_text(), e.informative_text());
  } catch (rlvm::Exception& e) {
    ReportFatalError(_("Fatal RLVM error"), e.what());
  } catch (libreallive::Error& e) {
    ReportFatalError(_("Fatal libreallive error"), e.what());
  } catch (SystemError& e) {
    ReportFatalError(_("Fatal local system error"), e.what());
  } catch (std::exception& e) {
    ReportFatalError(_("Uncaught exception"), e.what());
  } catch (const char* e) {
    ReportFatalError(_("Uncaught exception"), e);
  }
}

void RLVMInstance::ReportFatalError(const std::string& message_text,
                                    const std::string& informative_text) {
  if (platform_implementor_)
    platform_implementor_->ReportFatalError(message_text, informative_text);
}

bool RLVMInstance::AskUserPrompt(const std::string& message_text,
                                 const std::string& informative_text,
                                 const std::string& true_button,
                                 const std::string& false_button) {
  if (!platform_implementor_)
    return true;
  return platform_implementor_->AskUserPrompt(message_text, informative_text,
                                              true_button, false_button);
}

void RLVMInstance::DoUserNameCheck(RLMachine& machine, int encoding) {
  try {
    // Iterate over all the names in both global and local memory banks.
    GlobalMemory g = machine.GetMemory().GetGlobalMemory();
    LocalMemory l = machine.GetMemory().GetLocalMemory();
    std::string line;
    for (int i = 0; i < SIZE_OF_NAME_BANK; ++i) {
      line = g.global_names.Get(i);
      cp932toUTF8(line, encoding);
      g.global_names.Set(i, line);
    }

    for (int i = 0; i < SIZE_OF_NAME_BANK; ++i) {
      line = l.local_names.Get(i);
      cp932toUTF8(line, encoding);
      l.local_names.Set(i, line);
    }

    machine.GetMemory().PartialReset(std::move(g));
    machine.GetMemory().PartialReset(std::move(l));
  } catch (...) {
    // We've failed to interpret one of the name strings as a string in the
    // text encoding of the current native encoding. We're going to fail to
    // display any line that refers to the player's name.
    //
    // That's obviously bad and there's no real way to recover from this so
    // just reset all of global memory.
    std::cerr << "Corrupted global memory" << std::endl;
  }
}

std::filesystem::path RLVMInstance::FindGameFile(
    const std::filesystem::path& gamerootPath,
    const std::string& filename) {
  fs::path search_for = gamerootPath / filename;
  fs::path corrected_path = CorrectPathCase(search_for);
  if (corrected_path.empty()) {
    throw rlvm::UserPresentableError(
        _("Could not load game"),
        str(format(_("Could not open %1%. Please make sure it exists.")) %
            search_for));
  }

  return corrected_path;
}

void RLVMInstance::CheckBadEngine(const std::filesystem::path& gamerootPath,
                                  const std::vector<std::string> filenames,
                                  const std::string& message_text) {
  for (const auto& cur_file : filenames) {
    if (fs::exists(CorrectPathCase(gamerootPath / cur_file))) {
      throw rlvm::UserPresentableError(message_text,
                                       _("rlvm can only play RealLive games."));
    }
  }
}
