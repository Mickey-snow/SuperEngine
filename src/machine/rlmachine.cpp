// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2006, 2007 Elliot Glaysher
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

#include "base/gameexe.hpp"
#include "libreallive/archive.hpp"
#include "libreallive/elements/bytecode.hpp"
#include "libreallive/elements/comma.hpp"
#include "libreallive/elements/command.hpp"
#include "libreallive/elements/expression.hpp"
#include "libreallive/elements/meta.hpp"
#include "libreallive/elements/textout.hpp"
#include "libreallive/expression.hpp"
#include "libreallive/intmemref.hpp"
#include "libreallive/scenario.hpp"
#include "long_operations/pause_long_operation.hpp"
#include "long_operations/textout_long_operation.hpp"
#include "machine/long_operation.hpp"
#include "machine/reallive_dll.hpp"
#include "machine/rlmodule.hpp"
#include "machine/rloperation.hpp"
#include "machine/serialization.hpp"
#include "machine/stack_frame.hpp"
#include "memory/memory.hpp"
#include "memory/serialization_local.hpp"
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
#include <functional>
#include <iostream>
#include <iterator>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

using std::cerr;
using std::cout;
using std::endl;

// -----------------------------------------------------------------------
// RLMachine
// -----------------------------------------------------------------------

RLMachine::RLMachine(System& in_system, libreallive::Archive& in_archive)
    : memory_(std::make_unique<Memory>()),
      archive_(in_archive),
      scriptor_(in_archive),
      system_(in_system) {
  // Search in the Gameexe for #SEEN_START and place us there
  Gameexe& gameexe = in_system.gameexe();
  memory_->LoadFrom(gameexe);

  int first_seen = -1;
  if (gameexe.Exists("SEEN_START"))
    first_seen = gameexe("SEEN_START").ToInt();

  if (first_seen < 0) {
    // if SEEN_START is undefined, then just grab the first SEEN.
    first_seen = in_archive.GetFirstScenarioID();
  }

  call_stack_.Push(
      StackFrame(scriptor_.Load(first_seen), StackFrame::TYPE_ROOT));

  // Initial value of the savepoint
  MarkSavepoint();

  // Load the "DLLs" required
  for (auto it : gameexe.Filter("DLL.")) {
    const std::string& name = it.ToString("");
    try {
      std::string index_str = it.key().substr(it.key().find_first_of(".") + 1);
      int index = std::stoi(index_str);
      LoadDLL(index, name);
    } catch (rlvm::Exception& e) {
      cerr << "WARNING: Don't know what to do with DLL '" << name << "'"
           << endl;
    }
  }
}

RLMachine::~RLMachine() {}

void RLMachine::HardResetMemory() {
  memory_ = std::make_unique<Memory>();
  memory_->LoadFrom(system().gameexe());
}

void RLMachine::MarkSavepoint() {
  savepoint_call_stack_ = call_stack_.Clone();
  savepoint_memory_ = memory();
  system().graphics().TakeSavepointSnapshot();
  system().text().TakeSavepointSnapshot();
}

bool RLMachine::SavepointDecide(AttributeFunction func,
                                const std::string& gameexe_key) const {
  if (!mark_savepoints_)
    return false;

  int attribute = (Scenario().*func)();
  if (attribute == 1)
    return true;
  else if (attribute == 2)
    return false;

  // check Gameexe key
  Gameexe& gexe = system_.gameexe();
  if (gexe.Exists(gameexe_key)) {
    int value = gexe(gameexe_key);
    if (value == 0)
      return false;
    else if (value == 1)
      return true;
  }

  // Assume default of true
  return true;
}

void RLMachine::SetMarkSavepoints(const int in) { mark_savepoints_ = in; }

bool RLMachine::ShouldSetMessageSavepoint() const {
  return SavepointDecide(&libreallive::Scenario::savepoint_message,
                         "SAVEPOINT_MESSAGE");
}

bool RLMachine::ShouldSetSelcomSavepoint() const {
  return SavepointDecide(&libreallive::Scenario::savepoint_selcom,
                         "SAVEPOINT_SELCOM");
}

bool RLMachine::ShouldSetSeentopSavepoint() const {
  return SavepointDecide(&libreallive::Scenario::savepoint_seentop,
                         "SAVEPOINT_SEENTOP");
}

void RLMachine::ExecuteNextInstruction() {
  // Do not execute any more instructions if the machine is halted.
  if (halted() == true) {
    return;
  } else {
    const auto top_frame = call_stack_.Top();
    if (top_frame == nullptr) {
      std::cerr << "RLMachine: Stack underflow" << std::endl;
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
        auto bytecode = *(top_frame->pos);
        std::visit(*this, bytecode->DownCast());
      }
    } catch (rlvm::UnimplementedOpcode& e) {
      AdvanceInstructionPointer();

      if (print_undefined_opcodes_) {
        cout << "(SEEN" << top_frame->pos.ScenarioNumber() << ")(Line " << line_
             << "):  " << e.what() << endl;
      }

    } catch (rlvm::Exception& e) {
      if (halt_on_exception_) {
        halted_ = true;
      } else {
        // Advance the instruction pointer so as to prevent infinite
        // loops where we throw an exception, and then try again.
        AdvanceInstructionPointer();
      }

      cout << "(SEEN" << top_frame->pos.ScenarioNumber() << ")(Line " << line_
           << ")";

      // We specialcase rlvm::Exception because we might have the name of the
      // opcode.
      if (e.operation()) {
        cout << "[" << e.operation()->name() << "]";
      }

      cout << ":  " << e.what() << endl;
    } catch (std::exception& e) {
      if (halt_on_exception_) {
        halted_ = true;
      } else {
        // Advance the instruction pointer so as to prevent infinite
        // loops where we throw an exception, and then try again.
        AdvanceInstructionPointer();
      }

      cout << "(SEEN" << top_frame->pos.ScenarioNumber() << ")(Line " << line_
           << "):  " << e.what() << endl;
    }
  }
}

void RLMachine::ExecuteUntilHalted() {
  while (!halted()) {
    ExecuteNextInstruction();
  }
}

void RLMachine::AdvanceInstructionPointer() {
  if (!replaying_graphics_stack()) {
    const auto it = call_stack_.FindTopRealFrame();
    if (it != nullptr) {
      it->pos++;
      if (!it->pos.HasNext())
        halted_ = true;
    }
  }
}

void RLMachine::Jump(int scenario_num, int entrypoint) {
  auto it = call_stack_.FindTopRealFrame();
  if (it != nullptr)
    it->pos = scriptor_.LoadEntry(scenario_num, entrypoint);
}

void RLMachine::Farcall(int scenario_num, int entrypoint) {
  if (entrypoint == 0 && ShouldSetSeentopSavepoint())
    MarkSavepoint();

  call_stack_.Push(StackFrame(scriptor_.LoadEntry(scenario_num, entrypoint),
                              StackFrame::TYPE_FARCALL));
}

void RLMachine::ReturnFromFarcall() { call_stack_.Pop(); }

void RLMachine::GotoLocation(unsigned long new_location) {
  const auto scenario_id = call_stack_.Top()->pos.ScenarioNumber();
  call_stack_.Top()->pos = scriptor_.Load(scenario_id, new_location);
}

void RLMachine::Gosub(unsigned long new_location) {
  const auto scenario_id = call_stack_.Top()->pos.ScenarioNumber();
  call_stack_.Push(StackFrame(scriptor_.Load(scenario_id, new_location),
                              StackFrame::TYPE_GOSUB));
}

void RLMachine::ReturnFromGosub() { call_stack_.Pop(); }

void RLMachine::PushStringValueUp(int index, const std::string& val) {
  if (index < 0 || index > 2) {
    throw rlvm::Exception("Invalid index in pushStringValue");
  }

  const auto it = call_stack_.FindTopRealFrame();
  if (it)
    it->previous_stack_snapshot->K.Set(index, val);
}

void RLMachine::PushLongOperation(LongOperation* long_operation) {
  const auto top_frame = call_stack_.Top();
  call_stack_.Push(
      StackFrame(top_frame->pos, long_operation));
}

void RLMachine::PopStackFrame() { call_stack_.Pop(); }

int RLMachine::GetStackSize() { return call_stack_.Size(); }

void RLMachine::ClearLongOperationsOffBackOfStack() {
  while (call_stack_.Size() &&
         call_stack_.Top() != call_stack_.FindTopRealFrame())
    call_stack_.Pop();
}

void RLMachine::Reset() {
  call_stack_ = CallStack();
  savepoint_call_stack_ = CallStack();
  system().Reset();
}

void RLMachine::LocalReset() {
  savepoint_call_stack_ = CallStack();
  memory_->PartialReset(LocalMemory());
  system().Reset();
}

std::shared_ptr<LongOperation> RLMachine::CurrentLongOperation() const {
  auto top = call_stack_.Top();
  if (top && top->frame_type == StackFrame::TYPE_LONGOP)
    return top->long_op;

  return std::shared_ptr<LongOperation>();
}

void RLMachine::ClearCallstack() { call_stack_ = CallStack(); }

int RLMachine::SceneNumber() const {
  return call_stack_.Top()->pos.ScenarioNumber();
}

const libreallive::Scenario& RLMachine::Scenario() const {
  return *(call_stack_.Top()->pos.GetScenario());
}

int RLMachine::GetTextEncoding() const {
  return Scenario().encoding();
}

int RLMachine::GetProbableEncodingType() const {
  return archive_.GetProbableEncodingType();
}

void RLMachine::PerformTextout(const std::string& cp932str) {
  std::string name_parsed_text;
  try {
    name_parsed_text = parseNames(memory(), cp932str);
  } catch (rlvm::Exception& e) {
    // WEIRD: Sometimes rldev (and the official compiler?) will generate strings
    // that aren't valid shift_jis. Fall back while I figure out how to handle
    // this.
    name_parsed_text = cp932str;
  }

  std::string utf8str = cp932toUTF8(name_parsed_text, GetTextEncoding());
  TextSystem& ts = system().text();

  // Display UTF-8 characters
  std::unique_ptr<TextoutLongOperation> ptr =
      std::make_unique<TextoutLongOperation>(*this, utf8str);

  if (system().ShouldFastForward() || ts.message_no_wait() ||
      ts.script_message_nowait()) {
    ptr->set_no_wait();
  }

  // Run the textout operation once. If it doesn't fully succeed, push it onto
  // the stack.
  if (!(*ptr)(*this)) {
    PushLongOperation(ptr.release());
  }
}

void RLMachine::SetKidokuMarker(int kidoku_number) {
  // Check to see if we mark savepoints on textout
  if (ShouldSetMessageSavepoint() &&
      system_.text().GetCurrentPage().number_of_chars_on_page() == 0)
    MarkSavepoint();

  // Mark if we've previously read this piece of text.
  system_.text().SetKidokuRead(
      memory().HasBeenRead(SceneNumber(), kidoku_number));

  // Record the kidoku pair in global memory.
  memory().RecordKidoku(SceneNumber(), kidoku_number);
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
    throw rlvm::Exception(oss.str());
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
    throw rlvm::Exception(oss.str());
  }
}

void RLMachine::SetPrintUndefinedOpcodes(bool in) {
  print_undefined_opcodes_ = in;
}

void RLMachine::Halt() { halted_ = true; }

void RLMachine::SetHaltOnException(bool halt_on_exception) {
  halt_on_exception_ = halt_on_exception;
}

void RLMachine::SetLineNumber(const int i) {
  line_ = i;

  if (on_line_actions_) {
    ActionMap::iterator it =
        on_line_actions_->find(std::make_pair(SceneNumber(), line_));
    if (it != on_line_actions_->end()) {
      it->second();
    }
  }
}

void RLMachine::AddLineAction(const int seen,
                              const int line,
                              std::function<void(void)> function) {
  if (!on_line_actions_)
    on_line_actions_.reset(new ActionMap);

  (*on_line_actions_)[std::make_pair(seen, line)] = function;
}

// -----------------------------------------------------------------------

void RLMachine::operator()(libreallive::CommaElement const*) {
  AdvanceInstructionPointer();
}

void RLMachine::operator()(libreallive::MetaElement const* m) {
  if (m->type_ == libreallive::MetaElement::Line_)
    SetLineNumber(m->value_);
  else if (m->type_ == libreallive::MetaElement::Kidoku_)
    SetKidokuMarker(m->value_);

  AdvanceInstructionPointer();
}

void RLMachine::operator()(libreallive::CommandElement const* f) {
  auto op = module_manager_.Dispatch(*f);
  if (op == nullptr) {  // unimplemented opcode
    throw rlvm::UnimplementedOpcode(*this, *f);
  }

  try {
    if (tracer_)
      tracer_->Log(SceneNumber(), line_number(), op, *f);
    op->DispatchFunction(*this, *f);
  } catch (rlvm::Exception& e) {
    e.setOperation(op);
    throw e;
  }
}

void RLMachine::operator()(libreallive::ExpressionElement const* e) {
  e->ParsedExpression()->GetIntegerValue(*this);  // (?)
  AdvanceInstructionPointer();
}

void RLMachine::operator()(libreallive::TextoutElement const* e) {
  // Seen files are terminated with the string "SeenEnd", which isn't NULL
  // terminated and has a bunch of random garbage after it.
  constexpr std::string_view SeenEnd{
      "\x82\x72"  // S
      "\x82\x85"  // e
      "\x82\x85"  // e
      "\x82\x8e"  // n
      "\x82\x64"  // E
      "\x82\x8e"  // n
      "\x82\x84"  // d
  };

  std::string unparsed_text = e->GetText();
  if (unparsed_text.starts_with(SeenEnd)) {
    unparsed_text = SeenEnd;
    Halt();
  }

  PerformTextout(unparsed_text);
  AdvanceInstructionPointer();
}

// -----------------------------------------------------------------------

template <class Archive>
void RLMachine::save(Archive& ar, unsigned int version) const {
  int line_num = line_number();
  ar & line_num;

  // Save the state of the stack when the last save point was hit
  // ar & savepoint_call_stack_;
}

template <class Archive>
void RLMachine::load(Archive& ar, unsigned int version) {
  ar & line_;

  // Just thaw the call_stack_; all preprocessing was done at freeze
  // time.
  // ar & call_stack_;
}

// -----------------------------------------------------------------------

// Explicit instantiations for text archives (since we hide the
// implementation)

template void RLMachine::save<boost::archive::text_oarchive>(
    boost::archive::text_oarchive& ar,
    unsigned int version) const;

template void RLMachine::load<boost::archive::text_iarchive>(
    boost::archive::text_iarchive& ar,
    unsigned int version);
