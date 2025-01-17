// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2006, 2007 Elliot Glaysher
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

#include "machine/rlmachine.hpp"

#include "core/memory.hpp"
#include "log/domain_logger.hpp"
#include "long_operations/pause_long_operation.hpp"
#include "long_operations/textout_long_operation.hpp"
#include "machine/long_operation.hpp"
#include "machine/reallive_dll.hpp"
#include "machine/rlmodule.hpp"
#include "machine/rloperation.hpp"
#include "machine/serialization.hpp"
#include "machine/stack_frame.hpp"
#include "systems/base/graphics_system.hpp"
#include "systems/base/system.hpp"
#include "systems/base/system_error.hpp"
#include "systems/base/text_page.hpp"
#include "systems/base/text_system.hpp"
#include "utilities/exception.hpp"
#include "utilities/string_utilities.hpp"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

#include <filesystem>
#include <format>
#include <functional>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

static DomainLogger logger("RLMachine");

// -----------------------------------------------------------------------
// RLMachine
// -----------------------------------------------------------------------

RLMachine::RLMachine(System& system,
                     std::shared_ptr<IScriptor> scriptor,
                     ScriptLocation starting_location,
                     std::unique_ptr<Memory> memory)
    : memory_(std::move(memory)),
      module_manager_(module_manager_prototype),
      scriptor_(scriptor),
      system_(system) {
  // Setup stack memory
  Memory::Stack stack_memory;
  stack_memory.K = MemoryBank<std::string>(
      std::make_shared<StackMemoryAdapter<StackBank::StrK>>(call_stack_));
  stack_memory.L = MemoryBank<int>(
      std::make_shared<StackMemoryAdapter<StackBank::IntL>>(call_stack_));
  memory_->PartialReset(std::move(stack_memory));

  // Setup call stack
  call_stack_.Push(StackFrame(starting_location, StackFrame::TYPE_ROOT));

  // Initial value of the savepoint
  MarkSavepoint();
}

RLMachine::~RLMachine() = default;

void RLMachine::MarkSavepoint() {
  savepoint_call_stack_ = call_stack_.Clone();
  savepoint_memory_ = Memory();
  GetSystem().graphics().TakeSavepointSnapshot();
  GetSystem().text().TakeSavepointSnapshot();
}

ScenarioConfig RLMachine::GetScenarioConfig() const {
  int current_scenario = call_stack_.Top()->pos.scenario_number;
  return scriptor_->GetScenarioConfig(current_scenario);
}

void RLMachine::SetMarkSavepoints(const int in) { mark_savepoints_ = in; }

void RLMachine::ExecuteNextInstruction() {
  // Do not execute any more instructions if the machine is halted.
  if (IsHalted() == true)
    return;

  const auto top_frame = call_stack_.Top();
  if (top_frame == nullptr) {
    logger(Severity::Error) << "Stack underflow";
    Halt();
    return;
  }

  try {
    if (top_frame->frame_type == StackFrame::TYPE_LONGOP) {
      bool finished = false;
      {
        auto lock = call_stack_.GetLock();
        finished = (*top_frame->long_op)(*this);
      }

      if (finished) {
        call_stack_.Pop();
      }

    } else {
      auto instruction = scriptor_->ResolveInstruction(top_frame->pos);
      std::visit(*this, std::move(instruction));
    }
  } catch (rlvm::UnimplementedOpcode& e) {
    AdvanceInstructionPointer();

    static DomainLogger logger("Unimplemented");
    logger(Severity::None) << std::format("({:0>4d}:{:d}) ",
                                          top_frame->pos.scenario_number, line_)
                           << e.FormatCommand() << e.FormatParameters();
  } catch (rlvm::Exception& e) {
    // Advance the instruction pointer so as to prevent infinite
    // loops where we throw an exception, and then try again.
    AdvanceInstructionPointer();

    static DomainLogger logger;
    auto rec = logger(Severity::Error);
    rec << std::format("({:0>4d}:{:d}) ", top_frame->pos.scenario_number,
                       line_);

    // We specialcase rlvm::Exception because we might have the name of the
    // opcode.
    if (e.operation()) {
      rec << "[" << e.operation()->Name() << "]";
    }

    rec << ":  " << e.what();
  } catch (std::exception& e) {
    // Advance the instruction pointer so as to prevent infinite
    // loops where we throw an exception, and then try again.
    AdvanceInstructionPointer();

    auto rec = logger(Severity::Error);
    rec << std::format("({:0>4d}:{:d}) ", top_frame->pos.scenario_number,
                       line_);
    rec << e.what();
  }
}

void RLMachine::AdvanceInstructionPointer() {
  if (replaying_graphics_stack())
    return;

  const auto it = call_stack_.FindTopRealFrame();
  if (it != nullptr) {
    it->pos = scriptor_->Next(it->pos);

    if (!scriptor_->HasNext(it->pos))
      halted_ = true;
  }
}

CallStack& RLMachine::GetStack() { return call_stack_; }

std::shared_ptr<IScriptor> RLMachine::GetScriptor() { return scriptor_; }

Gameexe& RLMachine::GetGameexe() { return system_.gameexe(); }

void RLMachine::PushLongOperation(
    std::shared_ptr<LongOperation> long_operation) {
  const auto top_frame = call_stack_.Top();
  call_stack_.Push(StackFrame(top_frame->pos, long_operation));
}
void RLMachine::PushLongOperation(LongOperation* long_operation) {
  PushLongOperation(std::shared_ptr<LongOperation>(long_operation));
}

void RLMachine::Reset() {
  call_stack_ = CallStack();
  savepoint_call_stack_ = CallStack();
  GetSystem().Reset();
}

void RLMachine::LocalReset() {
  savepoint_call_stack_ = CallStack();
  memory_->PartialReset(LocalMemory());
  GetSystem().Reset();
}

std::shared_ptr<LongOperation> RLMachine::CurrentLongOperation() const {
  auto top = call_stack_.Top();
  if (top && top->frame_type == StackFrame::TYPE_LONGOP)
    return top->long_op;

  return std::shared_ptr<LongOperation>();
}

int RLMachine::SceneNumber() const {
  return call_stack_.Top()->pos.scenario_number;
}

int RLMachine::GetTextEncoding() const {
  auto scenario_number = call_stack_.Top()->pos.scenario_number;
  return scriptor_->GetScenarioConfig(scenario_number).text_encoding;
}

bool RLMachine::DllLoaded(const std::string& name) {
  for (auto const& dll : loaded_dlls_) {
    if (dll.second->GetDLLName() == name)
      return true;
  }

  return false;
}

void RLMachine::LoadDLL(int slot, const std::string& name) {
  RealLiveDLL* dll = RealLiveDLL::BuildDLLNamed(*this, name);
  if (dll) {
    loaded_dlls_.emplace(slot, std::unique_ptr<RealLiveDLL>(dll));
  } else {
    std::ostringstream oss;
    oss << "Can't load emulated DLL named '" << name << "'";
    throw std::runtime_error(oss.str());
  }
}

void RLMachine::UnloadDLL(int slot) { loaded_dlls_.erase(slot); }

int RLMachine::CallDLL(int slot,
                       int one,
                       int two,
                       int three,
                       int four,
                       int five) {
  DLLMap::iterator it = loaded_dlls_.find(slot);
  if (it != loaded_dlls_.end()) {
    return it->second->CallDLL(*this, one, two, three, four, five);
  } else {
    std::ostringstream oss;
    oss << "Attempt to callDLL(" << one << ", " << two << ", " << three << ", "
        << four << ", " << five << ") on slot " << slot
        << " when no DLL is loaded there!";
    throw std::runtime_error(oss.str());
  }
}

void RLMachine::Halt() { halted_ = true; }

void RLMachine::AddLineAction(const int seen,
                              const int line,
                              std::function<void(void)> function) {
  if (!on_line_actions_)
    on_line_actions_.reset(new ActionMap);

  (*on_line_actions_)[std::make_pair(seen, line)] = function;
}

// -----------------------------------------------------------------------

void RLMachine::PerformTextout(std::string cp932str) {
  std::string name_parsed_text;
  try {
    name_parsed_text = parseNames(GetMemory(), cp932str);
  } catch (rlvm::Exception& e) {
    // WEIRD: Sometimes rldev (and the official compiler?) will generate strings
    // that aren't valid shift_jis. Fall back while I figure out how to handle
    // this.
    name_parsed_text = cp932str;
  }

  std::string utf8str = cp932toUTF8(name_parsed_text, GetTextEncoding());
  TextSystem& ts = GetSystem().text();

  // Display UTF-8 characters
  std::shared_ptr<TextoutLongOperation> ptr =
      std::make_shared<TextoutLongOperation>(*this, utf8str);

  if (GetSystem().ShouldFastForward() || ts.message_no_wait() ||
      ts.script_message_nowait()) {
    ptr->set_no_wait();
  }

  // Run the textout operation once. If it doesn't fully succeed, push it onto
  // the stack.
  if (!(*ptr)(*this)) {
    PushLongOperation(ptr);
  }
}

// -----------------------------------------------------------------------

void RLMachine::operator()(std::monostate) { AdvanceInstructionPointer(); }

void RLMachine::operator()(Kidoku k) {
  // Check to see if we mark savepoints on textout
  if (GetScenarioConfig().enable_message_savepoint &&
      system_.text().GetCurrentPage().number_of_chars_on_page() == 0) {
    MarkSavepoint();
  }

  // Mark if we've previously read this piece of text.
  system_.text().SetKidokuRead(kidoku_table_.HasBeenRead(SceneNumber(), k.num));

  // Record the kidoku pair in global memory.
  kidoku_table_.RecordKidoku(SceneNumber(), k.num);

  AdvanceInstructionPointer();
}

void RLMachine::operator()(Line l) {
  line_ = l.num;

  if (on_line_actions_) {
    ActionMap::iterator it =
        on_line_actions_->find(std::make_pair(SceneNumber(), line_));
    if (it != on_line_actions_->end()) {
      it->second();
    }
  }

  AdvanceInstructionPointer();
}

void RLMachine::operator()(rlCommand cmd) {
  auto f = cmd.cmd;

  auto op = module_manager_.Dispatch(*f);
  if (op == nullptr) {  // unimplemented opcode
    throw rlvm::UnimplementedOpcode("", *f);
  }

  try {
    {  // write trace log
      static DomainLogger tracer("TRACE");
      auto rec = tracer(Severity::None);
      rec << std::format("({:0>4d}:{:d}) ", SceneNumber(), line_);
      rec << module_manager_.GetCommandName(*f);
      rec << cmd.ToString();
    }

    op->DispatchFunction(*this, *f);
  } catch (rlvm::Exception& e) {
    e.setOperation(op);
    throw e;
  }
}

void RLMachine::operator()(rlExpression e) {
  static DomainLogger tracer("TRACE");
  tracer() << std::format("({:0>4d}:{:d}) ", SceneNumber(), line_)
           << e.ToString();
  e.Execute(*this);
  AdvanceInstructionPointer();
}

void RLMachine::operator()(Textout t) {
  static DomainLogger tracer("TEXT");
  tracer() << t.text;
  PerformTextout(std::move(t.text));
  AdvanceInstructionPointer();
}

void RLMachine::operator()(End) { Halt(); }
