// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2007 Elliot Glaysher
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

#include "modules/module_sel.hpp"

#include <iterator>
#include <string>
#include <vector>

#include "core/gameexe.hpp"
#include "libreallive/parser.hpp"
#include "long_operations/button_object_select_long_operation.hpp"
#include "long_operations/select_long_operation.hpp"
#include "machine/rlmachine.hpp"
#include "machine/rloperation.hpp"
#include "machine/rloperation/rlop_store.hpp"
#include "systems/base/event_system.hpp"
#include "systems/base/graphics_object.hpp"
#include "systems/base/graphics_system.hpp"
#include "systems/base/system.hpp"
#include "systems/base/text_system.hpp"
#include "systems/base/text_window.hpp"
#include "utilities/string_utilities.hpp"

using libreallive::CommandElement;
using libreallive::SelectElement;

namespace {

struct Sel_select : public RLOp_SpecialCase {
  void operator()(RLMachine& machine, const CommandElement& ce) override {
    if (machine.GetScenarioConfig().enable_selcom_savepoint)
      machine.MarkSavepoint();

    const SelectElement& element = dynamic_cast<const SelectElement&>(ce);
    machine.PushLongOperation(
        std::make_shared<NormalSelectLongOperation>(machine, element));
    machine.AdvanceInstructionPointer();
  }
};

struct Sel_select_s : public RLOp_SpecialCase {
  void operator()(RLMachine& machine, const CommandElement& ce) override {
    if (machine.GetScenarioConfig().enable_selcom_savepoint)
      machine.MarkSavepoint();

    const SelectElement& element = dynamic_cast<const SelectElement&>(ce);
    machine.PushLongOperation(
        std::make_shared<ButtonSelectLongOperation>(machine, element, 0));
    machine.AdvanceInstructionPointer();
  }
};

struct ClearAndRestoreWindow : public LongOperation {
  int to_restore_;
  explicit ClearAndRestoreWindow(int in) : to_restore_(in) {}

  bool operator()(RLMachine& machine) override {
    machine.GetSystem().text().HideAllTextWindows();
    machine.GetSystem().text().set_active_window(to_restore_);
    return true;
  }
};

struct Sel_select_w : public RLOp_SpecialCase {
  void operator()(RLMachine& machine, const CommandElement& ce) override {
    if (machine.GetScenarioConfig().enable_selcom_savepoint)
      machine.MarkSavepoint();

    const SelectElement& element = dynamic_cast<const SelectElement&>(ce);

    // Sometimes the RL bytecode will override DEFAULT_SEL_WINDOW.
    int window = machine.GetSystem().gameexe()("DEFAULT_SEL_WINDOW").ToInt(-1);
    libreallive::Expression window_exp = element.GetWindowExpression();
    int computed = window_exp->GetIntegerValue(machine);
    if (computed != -1)
      window = computed;

    // Restore the previous text state after the select operation completes.
    TextSystem& text = machine.GetSystem().text();
    int active_window = text.active_window();
    text.HideAllTextWindows();
    text.set_active_window(window);
    machine.PushLongOperation(
        std::make_shared<ClearAndRestoreWindow>(active_window));

    machine.PushLongOperation(
        std::make_shared<NormalSelectLongOperation>(machine, element));
    machine.AdvanceInstructionPointer();
  }
};

struct Sel_select_objbtn : public RLOpcode<IntConstant_T> {
  void operator()(RLMachine& machine, int group) {
    if (machine.GetScenarioConfig().enable_selcom_savepoint)
      machine.MarkSavepoint();

    machine.PushLongOperation(
        std::make_shared<ButtonObjectSelectLongOperation>(machine, group));
  }
};

struct Sel_select_objbtn_cancel_0 : public RLOpcode<IntConstant_T> {
  void operator()(RLMachine& machine, int group) {
    if (machine.GetScenarioConfig().enable_selcom_savepoint)
      machine.MarkSavepoint();

    auto obj =
        std::make_shared<ButtonObjectSelectLongOperation>(machine, group);
    obj->set_cancelable();
    machine.PushLongOperation(obj);
  }
};

struct Sel_select_objbtn_cancel_1
    : public RLOpcode<IntConstant_T, IntConstant_T> {
  void operator()(RLMachine& machine, int group, int se) {
    if (machine.GetScenarioConfig().enable_selcom_savepoint)
      machine.MarkSavepoint();

    auto obj =
        std::make_shared<ButtonObjectSelectLongOperation>(machine, group);
    obj->set_cancelable();
    machine.PushLongOperation(obj);
  }
};

struct Sel_select_objbtn_cancel_2 : public RLOpcode<> {
  void operator()(RLMachine& machine) {
    if (machine.GetScenarioConfig().enable_selcom_savepoint)
      machine.MarkSavepoint();

    auto& fg_objs = machine.GetSystem().graphics().GetForegroundObjects();
    int group = 0;
    for (GraphicsObject& obj : fg_objs)
      if (obj.Param().IsButton()) {
        group = obj.Param().GetButtonGroup();
        break;
      }

    auto obj =
        std::make_shared<ButtonObjectSelectLongOperation>(machine, group);
    obj->set_cancelable();
    machine.PushLongOperation(obj);
  }
};

// Our system doesn't need an explicit initialize.
struct objbtn_init_0 : public RLOpcode<IntConstant_T> {
  void operator()(RLMachine& machine, int ignored) {}
};

struct objbtn_init_1 : public RLOpcode<> {
  void operator()(RLMachine& machine) {}
};

struct btnobjnow_hit : public RLStoreOpcode<> {
  int operator()(RLMachine& machine) override { return -1; }
};

struct btnobjnow_push : public RLStoreOpcode<> {
  int operator()(RLMachine& machine) override { return -1; }
};

struct btnobjnow_decide : public RLStoreOpcode<> {
  int operator()(RLMachine& machine) override { return -2; }
};
}  // namespace

SelModule::SelModule() : RLModule("Sel", 0, 2) {
  AddOpcode(0, 0, "select_w", new Sel_select_w);
  AddOpcode(1, 0, "select", new Sel_select);
  AddOpcode(2, 0, "select_s2", new Sel_select_s);
  AddOpcode(3, 0, "select_s", new Sel_select_s);
  AddOpcode(4, 0, "select_objbtn", new Sel_select_objbtn);
  AddOpcode(14, 0, "select_objbtn_cancel", new Sel_select_objbtn_cancel_0);
  AddOpcode(14, 1, "select_objbtn_cancel", new Sel_select_objbtn_cancel_1);
  AddOpcode(14, 2, "select_objbtn_cancel", new Sel_select_objbtn_cancel_2);

  AddOpcode(20, 0, "objbtn_init", new objbtn_init_0);
  AddOpcode(20, 1, "objbtn_init", new objbtn_init_1);

  AddOpcode(30, 0, "select_btnobjnow_hit", new btnobjnow_hit);
  AddOpcode(31, 0, "select_btnobjnow_push", new btnobjnow_push);
  AddOpcode(32, 0, "select_btnobjnow_decide", new btnobjnow_decide);
}
