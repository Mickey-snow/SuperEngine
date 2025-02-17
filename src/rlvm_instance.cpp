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

#include "core/gameexe.hpp"
#include "core/memory.hpp"
#include "libreallive/game_loader.hpp"
#include "log/domain_logger.hpp"
#include "machine/debugger.hpp"
#include "machine/rlmachine.hpp"
#include "machine/rloperation.hpp"
#include "machine/serialization.hpp"
#include "platforms/implementor.hpp"
#include "systems/base/system_error.hpp"
#include "systems/sdl/sdl_system.hpp"
#include "utf8.h"
#include "utilities/clock.hpp"
#include "utilities/exception.hpp"
#include "utilities/gettext.hpp"
#include "utilities/string_utilities.hpp"

#include <iostream>
#include <string>
#include <thread>

namespace fs = std::filesystem;

RLVMInstance::RLVMInstance() : seen_start_(-1), platform_implementor_(nullptr) {
  srand(time(NULL));
}

RLVMInstance::~RLVMInstance() {}

void RLVMInstance::SetPlatformImplementor(
    std::shared_ptr<IPlatformImplementor> impl) {
  platform_implementor_ = impl;
}

void RLVMInstance::Main(const std::filesystem::path& gameroot) {
  std::unique_ptr<libreallive::GameLoader> loader = nullptr;
  try {
    loader = std::make_unique<libreallive::GameLoader>(gameroot);
    machine_ = loader->machine_;
    debugger_ = loader->debugger_;
    system_ = loader->system_;
  } catch (std::exception& e) {
    static DomainLogger logger("Main");
    logger(Severity::Error) << "Failed to load game:\n" << e.what();
  }

  try {
    Clock clock;

    while (!machine_->IsHalted()) {
      // Give SDL a chance to respond to events, redraw the screen,
      // etc.
      system_->Run(*machine_);

      constexpr auto frame_time = std::chrono::seconds(1) / 144.0;
      auto start = clock.GetTime();
      auto end = start;

      for (bool should_continue = true; should_continue;) {
        // In one cycle in game loop, execute long operation at most once
        if (machine_->CurrentLongOperation() != nullptr)
          should_continue = false;

        debugger_->Execute();
        Step();

        end = clock.GetTime();

        if (machine_->IsHalted() || system_->force_wait() ||
            (end - start) >= frame_time)
          should_continue = false;
      }

      // Sleep to be nice to the processor and to give the GPU a chance to
      // catch up.
      if (!system_->ShouldFastForward()) {
        auto real_sleep_time = frame_time - (end - start);
        using std::chrono_literals::operator""ms;
        if (real_sleep_time < 1ms)
          real_sleep_time = 1ms;

        std::this_thread::sleep_for(real_sleep_time);
      }

      system_->set_force_wait(false);
    }

    Serialization::saveGlobalMemory(*machine_);
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

void RLVMInstance::Step() {
  static DomainLogger logger("RLVMInstance::Step");

  try {
    if (auto long_op = machine_->CurrentLongOperation()) {
      const bool finished = machine_->ExecuteLongop(long_op);

      if (finished)
        machine_->GetStack().Pop();

    } else {
      std::shared_ptr<Instruction> instruction = machine_->ReadInstruction();
      machine_->AdvanceIP();
      machine_->ExecuteInstruction(instruction);
    }

  } catch (rlvm::UnimplementedOpcode& e) {
    static DomainLogger logger("Unimplemented");
    logger(Severity::Info) << DescribeCurrentIP() << ' ' << e.FormatCommand()
                           << e.FormatParameters();
  } catch (rlvm::Exception& e) {
    auto rec = logger(Severity::Error);
    rec << DescribeCurrentIP() << ' ';

    // We specialcase rlvm::Exception because we might have the name of the
    // opcode.
    if (e.operation()) {
      rec << "[" << e.operation()->Name() << "]";
    }

    rec << ":  " << e.what();
  } catch (std::exception& e) {
    auto rec = logger(Severity::Error);
    rec << DescribeCurrentIP() << ' ';
    rec << e.what();
  }
}

std::string RLVMInstance::DescribeCurrentIP() const {
  const auto scene_num = machine_->SceneNumber();
  const auto line_num = machine_->LineNumber();
  return std::format("({:0>4d}:{:d})", scene_num, line_num);
}

// -----------------------------------------------------------------------

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
