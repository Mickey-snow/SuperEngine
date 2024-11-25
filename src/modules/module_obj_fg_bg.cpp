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

// Contains definitions for object handling functions for the Modules 81
// "ObjFg", 82 "ObjBg", 90 "ObjRange", and 91 "ObjBgRange".

#include "modules/module_obj_fg_bg.hpp"

#include <cmath>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#include "base/gameexe.hpp"
#include "libreallive/parser.hpp"
#include "machine/long_operation.hpp"
#include "machine/properties.hpp"
#include "machine/rlmachine.hpp"
#include "machine/rlmodule.hpp"
#include "machine/rloperation.hpp"
#include "machine/rloperation/default_value.hpp"
#include "machine/rloperation/rect_t.hpp"
#include "modules/module_obj.hpp"
#include "modules/object_mutator_operations.hpp"
#include "object/drawer/colour_filter.hpp"
#include "object/drawer/text.hpp"
#include "object/mutator.hpp"
#include "object/objdrawer.hpp"
#include "systems/base/event_system.hpp"
#include "systems/base/graphics_object.hpp"
#include "systems/base/graphics_system.hpp"
#include "systems/base/system.hpp"
#include "utilities/exception.hpp"
#include "utilities/graphics.hpp"
#include "utilities/string_utilities.hpp"

namespace {

struct dispArea_0 : public RLOpcode<IntConstant_T> {
  void operator()(RLMachine& machine, int buf) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    obj.Param().ClearClipRect();
    machine.GetSystem().graphics().mark_object_state_as_dirty();
  }
};

struct dispArea_1 : RLOpcode<IntConstant_T,
                             IntConstant_T,
                             IntConstant_T,
                             IntConstant_T,
                             IntConstant_T> {
  void operator()(RLMachine& machine, int buf, int x1, int y1, int x2, int y2) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    obj.Param().SetClipRect(Rect::GRP(x1, y1, x2, y2));
    machine.GetSystem().graphics().mark_object_state_as_dirty();
  }
};

struct dispRect_1 : RLOpcode<IntConstant_T,
                             IntConstant_T,
                             IntConstant_T,
                             IntConstant_T,
                             IntConstant_T> {
  void operator()(RLMachine& machine, int buf, int x, int y, int w, int h) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    obj.Param().SetClipRect(Rect::REC(x, y, w, h));
    machine.GetSystem().graphics().mark_object_state_as_dirty();
  }
};

struct dispCorner_1 : RLOpcode<IntConstant_T, IntConstant_T, IntConstant_T> {
  void operator()(RLMachine& machine, int buf, int x, int y) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    obj.Param().SetClipRect(Rect::GRP(0, 0, x, y));
    machine.GetSystem().graphics().mark_object_state_as_dirty();
  }
};

struct dispOwnArea_0 : public RLOpcode<IntConstant_T> {
  void operator()(RLMachine& machine, int buf) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    obj.Param().ClearOwnClipRect();
    machine.GetSystem().graphics().mark_object_state_as_dirty();
  }
};

struct dispOwnArea_1 : RLOpcode<IntConstant_T,
                                IntConstant_T,
                                IntConstant_T,
                                IntConstant_T,
                                IntConstant_T> {
  void operator()(RLMachine& machine, int buf, int x1, int y1, int x2, int y2) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    obj.Param().SetOwnClipRect(Rect::GRP(x1, y1, x2, y2));
    machine.GetSystem().graphics().mark_object_state_as_dirty();
  }
};

struct dispOwnRect_1 : RLOpcode<IntConstant_T,
                                IntConstant_T,
                                IntConstant_T,
                                IntConstant_T,
                                IntConstant_T> {
  void operator()(RLMachine& machine, int buf, int x, int y, int w, int h) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    obj.Param().SetOwnClipRect(Rect::REC(x, y, w, h));
    machine.GetSystem().graphics().mark_object_state_as_dirty();
  }
};

struct adjust
    : RLOpcode<IntConstant_T, IntConstant_T, IntConstant_T, IntConstant_T> {
  void operator()(RLMachine& machine, int buf, int idx, int x, int y) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    obj.Param().SetXAdjustment(idx, x);
    obj.Param().SetYAdjustment(idx, y);
    machine.GetSystem().graphics().mark_object_state_as_dirty();
  }
};

struct tint
    : RLOpcode<IntConstant_T, IntConstant_T, IntConstant_T, IntConstant_T> {
  void operator()(RLMachine& machine, int buf, int r, int g, int b) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    obj.Param().SetTint(RGBColour(r, g, b));
    machine.GetSystem().graphics().mark_object_state_as_dirty();
  }
};

struct colour : RLOpcode<IntConstant_T,
                         IntConstant_T,
                         IntConstant_T,
                         IntConstant_T,
                         IntConstant_T> {
  void operator()(RLMachine& machine, int buf, int r, int g, int b, int level) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    obj.Param().SetColour(RGBAColour(r, g, b, level));
    machine.GetSystem().graphics().mark_object_state_as_dirty();
  }
};

struct objSetRect_1 : public RLOpcode<IntConstant_T, Rect_T<rect_impl::GRP>> {
  void operator()(RLMachine& machine, int buf, Rect rect) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    if (obj.has_object_data()) {
      ColourFilterObjectData* data =
          dynamic_cast<ColourFilterObjectData*>(&obj.GetObjectData());
      if (data) {
        data->set_rect(rect);
        machine.GetSystem().graphics().mark_object_state_as_dirty();
      }
    }
  }
};

struct objSetRect_0 : public RLOpcode<IntConstant_T> {
  void operator()(RLMachine& machine, int buf) {
    Rect rect(0, 0, GetScreenSize(machine.GetSystem().gameexe()));
    objSetRect_1()(machine, buf, rect);
  }
};

struct objSetText : public RLOpcode<IntConstant_T, DefaultStrValue_T> {
  void operator()(RLMachine& machine, int buf, string val) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    std::string utf8str = cp932toUTF8(val, machine.GetTextEncoding());
    obj.Param().SetTextText(utf8str);
    machine.GetSystem().graphics().mark_object_state_as_dirty();
  }
};

// Note: It appears that the RealLive API changed sometime between when Haeleth
// was working on Kanon and RealLive Max. Previously, the zeroth overload took
// the shadow color. Now, the zeroth doesn't and a new first overload does. Use
// DefaultIntValue to try to handle both cases at once.
struct objTextOpts : public RLOpcode<IntConstant_T,
                                     IntConstant_T,
                                     IntConstant_T,
                                     IntConstant_T,
                                     IntConstant_T,
                                     IntConstant_T,
                                     DefaultIntValue_T<-1>> {
  void operator()(RLMachine& machine,
                  int buf,
                  int size,
                  int xspace,
                  int yspace,
                  int char_count,
                  int colour,
                  int shadow) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    obj.Param().SetTextOps(size, xspace, yspace, char_count, colour, shadow);
    machine.GetSystem().graphics().mark_object_state_as_dirty();
  }
};

struct objDriftOpts : public RLOpcode<IntConstant_T,
                                      IntConstant_T,
                                      IntConstant_T,
                                      IntConstant_T,
                                      IntConstant_T,
                                      IntConstant_T,
                                      IntConstant_T,
                                      IntConstant_T,
                                      IntConstant_T,
                                      IntConstant_T,
                                      IntConstant_T,
                                      IntConstant_T,
                                      Rect_T<rect_impl::GRP>> {
  void operator()(RLMachine& machine,
                  int buf,
                  int count,
                  int use_animation,
                  int start_pattern,
                  int end_pattern,
                  int total_animaton_time_ms,
                  int yspeed,
                  int period,
                  int amplitude,
                  int use_drift,
                  int unknown,
                  int driftspeed,
                  Rect drift_area) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    obj.Param().SetDriftOpts(count, use_animation, start_pattern, end_pattern,
                             total_animaton_time_ms, yspeed, period, amplitude,
                             use_drift, unknown, driftspeed, drift_area);
    machine.GetSystem().graphics().mark_object_state_as_dirty();
  }
};

struct objNumOpts : public RLOpcode<IntConstant_T,
                                    IntConstant_T,
                                    IntConstant_T,
                                    IntConstant_T,
                                    IntConstant_T,
                                    IntConstant_T> {
  void operator()(RLMachine& machine,
                  int buf,
                  int digits,
                  int zero,
                  int sign,
                  int pack,
                  int space) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    obj.Param().SetDigitOpts(digits, zero, sign, pack, space);
    machine.GetSystem().graphics().mark_object_state_as_dirty();
  }
};

struct objAdjustAlpha
    : public RLOpcode<IntConstant_T, IntConstant_T, IntConstant_T> {
  void operator()(RLMachine& machine, int buf, int idx, int alpha) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    obj.Param().SetAlphaAdjustment(idx, alpha);
    machine.GetSystem().graphics().mark_object_state_as_dirty();
  }
};

struct objButtonOpts : public RLOpcode<IntConstant_T,
                                       IntConstant_T,
                                       IntConstant_T,
                                       IntConstant_T,
                                       IntConstant_T> {
  void operator()(RLMachine& machine,
                  int buf,
                  int action,
                  int se,
                  int group,
                  int button_number) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    obj.Param().SetButtonOpts(action, se, group, button_number);
    machine.GetSystem().graphics().mark_object_state_as_dirty();
  }
};

// -----------------------------------------------------------------------

class objEveAdjust : public RLOpcode<IntConstant_T,
                                     IntConstant_T,
                                     IntConstant_T,
                                     IntConstant_T,
                                     IntConstant_T,
                                     IntConstant_T,
                                     IntConstant_T> {
 public:
  virtual void operator()(RLMachine& machine,
                          int obj,
                          int repno,
                          int x,
                          int y,
                          int duration_time,
                          int delay,
                          int type) {
    unsigned int creation_time = machine.GetSystem().event().GetTicks();

    GraphicsObject& object = GetGraphicsObject(machine, this, obj);
    int start_x = object.Param().x_adjustment(repno);
    int start_y = object.Param().y_adjustment(repno);

    object.AddObjectMutator(std::unique_ptr<ObjectMutator>(
        new AdjustMutator(machine, repno, creation_time, duration_time, delay,
                          type, start_x, x, start_y, y)));
  }
};

struct objEveDisplay_1 : public RLOpcode<IntConstant_T,
                                         IntConstant_T,
                                         IntConstant_T,
                                         IntConstant_T,
                                         IntConstant_T> {
  void operator()(RLMachine& machine,
                  int obj,
                  int display,
                  int duration_time,
                  int delay,
                  int param) {
    Gameexe& gameexe = machine.GetSystem().gameexe();
    const std::vector<int> disp = gameexe("OBJDISP", param).ToIntVector();

    GraphicsObject& object = GetGraphicsObject(machine, this, obj);
    unsigned int creation_time = machine.GetSystem().event().GetTicks();
    object.AddObjectMutator(std::make_unique<DisplayMutator>(
        object.Param(), creation_time, duration_time, delay, display,
        disp.at(0), disp.at(1), disp.at(2), disp.at(3), disp.at(4), disp.at(5),
        disp.at(6), disp.at(7), disp.at(8), disp.at(9), disp.at(10),
        disp.at(11), disp.at(12), disp.at(13)));
  }
};

struct objEveDisplay_2 : public RLOpcode<IntConstant_T,
                                         IntConstant_T,
                                         IntConstant_T,
                                         IntConstant_T,
                                         IntConstant_T,
                                         IntConstant_T,
                                         IntConstant_T,
                                         IntConstant_T,
                                         IntConstant_T> {
  void operator()(RLMachine& machine,
                  int obj,
                  int display,
                  int duration_time,
                  int delay,
                  int disp_event_mod,
                  int tr_mod,
                  int move_mod,
                  int move_len_x,
                  int move_len_y) {
    GraphicsObject& object = GetGraphicsObject(machine, this, obj);
    unsigned int creation_time = machine.GetSystem().event().GetTicks();
    object.AddObjectMutator(std::make_unique<DisplayMutator>(
        object.Param(), creation_time, duration_time, delay, display,
        disp_event_mod, tr_mod, move_mod, move_len_x, move_len_y, 0, 0, 0, 0, 0,
        0, 0, 0, 0));
  }
};

struct objEveDisplay_3 : public RLOpcode<IntConstant_T,
                                         IntConstant_T,
                                         IntConstant_T,
                                         IntConstant_T,
                                         IntConstant_T,
                                         IntConstant_T,
                                         IntConstant_T,
                                         IntConstant_T,
                                         IntConstant_T,
                                         IntConstant_T,
                                         IntConstant_T,
                                         IntConstant_T,
                                         IntConstant_T,
                                         IntConstant_T,
                                         IntConstant_T,
                                         IntConstant_T,
                                         IntConstant_T,
                                         IntConstant_T> {
  void operator()(RLMachine& machine,
                  int obj,
                  int display,
                  int duration_time,
                  int delay,
                  int disp_event_mod,
                  int tr_mod,
                  int move_mod,
                  int move_len_x,
                  int move_len_y,
                  int rotate_mod,
                  int rotate_count,
                  int scale_x_mod,
                  int scale_x_percent,
                  int scale_y_mod,
                  int scale_y_percent,
                  int sin_mod,
                  int sin_len,
                  int sin_count) {
    GraphicsObject& object = GetGraphicsObject(machine, this, obj);
    unsigned int creation_time = machine.GetSystem().event().GetTicks();
    object.AddObjectMutator(std::make_unique<DisplayMutator>(
        object.Param(), creation_time, duration_time, delay, display,
        disp_event_mod, tr_mod, move_mod, move_len_x, move_len_y, rotate_mod,
        rotate_count, scale_x_mod, scale_x_percent, scale_y_mod,
        scale_y_percent, sin_mod, sin_len, sin_count));
  }
};

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

void addUnifiedFunctions(ObjectModule& h) {
  h.AddDoubleObjectCommands(0, "Move",
                            CreateGetter<ObjectProperty::PositionX>(),
                            CreateSetter<ObjectProperty::PositionX>(),
                            CreateGetter<ObjectProperty::PositionY>(),
                            CreateSetter<ObjectProperty::PositionY>());
  h.AddSingleObjectCommands(1, "Left",
                            CreateGetter<ObjectProperty::PositionX>(),
                            CreateSetter<ObjectProperty::PositionX>());
  h.AddSingleObjectCommands(2, "Top", CreateGetter<ObjectProperty::PositionY>(),
                            CreateSetter<ObjectProperty::PositionY>());
  h.AddSingleObjectCommands(3, "Alpha",
                            CreateGetter<ObjectProperty::AlphaSource>(),
                            CreateSetter<ObjectProperty::AlphaSource>());

  // ----

  h.AddCustomRepno<adjust, objEveAdjust>(6, "Adjust");
  h.AddRepnoObjectCommands(7, "AdjustX",
                           CreateGetter<ObjectProperty::AdjustmentOffsetsX>(),
                           CreateSetter<ObjectProperty::AdjustmentOffsetsX>());
  h.AddRepnoObjectCommands(8, "AdjustY",
                           CreateGetter<ObjectProperty::AdjustmentOffsetsY>(),
                           CreateSetter<ObjectProperty::AdjustmentOffsetsY>());
  h.AddSingleObjectCommands(
      9, "Mono", CreateGetter<ObjectProperty::MonochromeTransform>(),
      CreateSetter<ObjectProperty::MonochromeTransform>());
  h.AddSingleObjectCommands(10, "Invert",
                            CreateGetter<ObjectProperty::InvertTransform>(),
                            CreateSetter<ObjectProperty::InvertTransform>());
  h.AddSingleObjectCommands(11, "Light",
                            CreateGetter<ObjectProperty::LightLevel>(),
                            CreateSetter<ObjectProperty::LightLevel>());

  // ---

  h.AddSingleObjectCommands(
      13, "TintR",
      [](const ParameterManager& param) {
        auto colour = param.Get<ObjectProperty::TintColour>();
        return colour.r();
      },
      [](ParameterManager& param, int value) {
        auto colour = param.Get<ObjectProperty::TintColour>();
        colour.set_red(value);
        param.Set(ObjectProperty::TintColour, colour);
      });
  h.AddSingleObjectCommands(
      14, "TintG",
      [](const ParameterManager& param) {
        auto colour = param.Get<ObjectProperty::TintColour>();
        return colour.g();
      },
      [](ParameterManager& param, int value) {
        auto colour = param.Get<ObjectProperty::TintColour>();
        colour.set_green(value);
        param.Set(ObjectProperty::TintColour, colour);
      });
  h.AddSingleObjectCommands(
      15, "TintB",
      [](const ParameterManager& param) {
        auto colour = param.Get<ObjectProperty::TintColour>();
        return colour.b();
      },
      [](ParameterManager& param, int value) {
        auto colour = param.Get<ObjectProperty::TintColour>();
        colour.set_blue(value);
        param.Set(ObjectProperty::TintColour, colour);
      });

  // ---

  h.AddSingleObjectCommands(
      17, "ColR",
      [](const ParameterManager& param) {
        auto colour = param.Get<ObjectProperty::BlendColour>();
        return colour.r();
      },
      [](ParameterManager& param, int value) {
        auto colour = param.Get<ObjectProperty::BlendColour>();
        colour.set_red(value);
        param.Set(ObjectProperty::BlendColour, colour);
      });
  h.AddSingleObjectCommands(
      18, "ColG",
      [](const ParameterManager& param) {
        auto colour = param.Get<ObjectProperty::BlendColour>();
        return colour.g();
      },
      [](ParameterManager& param, int value) {
        auto colour = param.Get<ObjectProperty::BlendColour>();
        colour.set_green(value);
        param.Set(ObjectProperty::BlendColour, colour);
      });
  h.AddSingleObjectCommands(
      19, "ColB",
      [](const ParameterManager& param) {
        auto colour = param.Get<ObjectProperty::BlendColour>();
        return colour.b();
      },
      [](ParameterManager& param, int value) {
        auto colour = param.Get<ObjectProperty::BlendColour>();
        colour.set_blue(value);
        param.Set(ObjectProperty::BlendColour, colour);
      });
  h.AddSingleObjectCommands(
      20, "ColLevel",
      [](const ParameterManager& param) {
        auto colour = param.Get<ObjectProperty::BlendColour>();
        return colour.a();
      },
      [](ParameterManager& param, int value) {
        auto colour = param.Get<ObjectProperty::BlendColour>();
        colour.set_alpha(value);
        param.Set(ObjectProperty::BlendColour, colour);
      });

  // ---

  h.AddSingleObjectCommands(36, "AdjustVert",
                            CreateGetter<ObjectProperty::AdjustmentVertical>(),
                            CreateSetter<ObjectProperty::AdjustmentVertical>());

  h.AddRepnoObjectCommands(40, "AdjustAlpha",
                           CreateGetter<ObjectProperty::AdjustmentAlphas>(),
                           CreateSetter<ObjectProperty::AdjustmentAlphas>());

  // --
  h.AddDoubleObjectCommands(46, "Scale",
                            CreateGetter<ObjectProperty::WidthPercent>(),
                            CreateSetter<ObjectProperty::WidthPercent>(),
                            CreateGetter<ObjectProperty::HeightPercent>(),
                            CreateSetter<ObjectProperty::HeightPercent>());
  h.AddSingleObjectCommands(47, "Width",
                            CreateGetter<ObjectProperty::WidthPercent>(),
                            CreateSetter<ObjectProperty::WidthPercent>());
  h.AddSingleObjectCommands(48, "Height",
                            CreateGetter<ObjectProperty::HeightPercent>(),
                            CreateSetter<ObjectProperty::HeightPercent>());
  h.AddSingleObjectCommands(49, "Rotate",
                            CreateGetter<ObjectProperty::RotationDiv10>(),
                            CreateSetter<ObjectProperty::RotationDiv10>());
  h.AddDoubleObjectCommands(50, "RepOrigin",
                            CreateGetter<ObjectProperty::RepetitionOriginX>(),
                            CreateSetter<ObjectProperty::RepetitionOriginX>(),
                            CreateGetter<ObjectProperty::RepetitionOriginY>(),
                            CreateSetter<ObjectProperty::RepetitionOriginY>());
  h.AddSingleObjectCommands(51, "RepOriginX",
                            CreateGetter<ObjectProperty::RepetitionOriginX>(),
                            CreateSetter<ObjectProperty::RepetitionOriginX>());
  h.AddSingleObjectCommands(52, "RepOriginY",
                            CreateGetter<ObjectProperty::RepetitionOriginY>(),
                            CreateSetter<ObjectProperty::RepetitionOriginY>());
  h.AddDoubleObjectCommands(53, "Origin",
                            CreateGetter<ObjectProperty::OriginX>(),
                            CreateSetter<ObjectProperty::OriginX>(),
                            CreateGetter<ObjectProperty::OriginY>(),
                            CreateSetter<ObjectProperty::OriginY>());
  h.AddSingleObjectCommands(54, "OriginX",
                            CreateGetter<ObjectProperty::OriginX>(),
                            CreateSetter<ObjectProperty::OriginX>());
  h.AddSingleObjectCommands(55, "OriginY",
                            CreateGetter<ObjectProperty::OriginY>(),
                            CreateSetter<ObjectProperty::OriginY>());

  // ---

  h.AddDoubleObjectCommands(
      61, "HqScale", CreateGetter<ObjectProperty::HighQualityWidthPercent>(),
      CreateSetter<ObjectProperty::HighQualityWidthPercent>(),
      CreateGetter<ObjectProperty::HighQualityHeightPercent>(),
      CreateSetter<ObjectProperty::HighQualityHeightPercent>());
  h.AddSingleObjectCommands(
      62, "HqWidth", CreateGetter<ObjectProperty::HighQualityWidthPercent>(),
      CreateSetter<ObjectProperty::HighQualityWidthPercent>());
  h.AddSingleObjectCommands(
      63, "HqHeight", CreateGetter<ObjectProperty::HighQualityHeightPercent>(),
      CreateSetter<ObjectProperty::HighQualityHeightPercent>());
}

void addObjectFunctions(RLModule& m) {
  m.AddOpcode(
      1004, 0, "objShow",
      new Obj_SetOneIntOnObj(CreateSetter<ObjectProperty::IsVisible>()));
  m.AddOpcode(1005, 0, "objDispArea", new dispArea_0);
  m.AddOpcode(1005, 1, "objDispArea", new dispArea_1);

  m.AddOpcode(1012, 0, "objTint", new tint);

  m.AddOpcode(1016, 0, "objColour", new colour);

  m.AddOpcode(
      1021, 0, "objComposite",
      new Obj_SetOneIntOnObj(CreateSetter<ObjectProperty::CompositeMode>()));

  m.AddOpcode(1022, 0, "objSetRect", new objSetRect_0);
  m.AddOpcode(1022, 1, "objSetRect", new objSetRect_1);

  m.AddOpcode(1024, 0, "objSetText", new objSetText);
  m.AddOpcode(1024, 1, "objSetText", new objSetText);
  m.AddOpcode(1025, 0, "objTextOpts", new objTextOpts);
  m.AddOpcode(1025, 1, "objTextOpts", new objTextOpts);

  m.AddOpcode(1026, 0, "objLayer",
              new Obj_SetOneIntOnObj(CreateSetter<ObjectProperty::ZLayer>()));
  m.AddOpcode(1027, 0, "objDepth",
              new Obj_SetOneIntOnObj(CreateSetter<ObjectProperty::ZDepth>()));
  m.AddUnsupportedOpcode(1028, 0, "objScrollRate");
  m.AddOpcode(
      1029, 0, "objScrollRateX",
      new Obj_SetOneIntOnObj(CreateSetter<ObjectProperty::ScrollRateX>()));
  m.AddOpcode(
      1030, 0, "objScrollRateY",
      new Obj_SetOneIntOnObj(CreateSetter<ObjectProperty::ScrollRateY>()));
  m.AddOpcode(1031, 0, "objDriftOpts", new objDriftOpts);
  m.AddOpcode(1032, 0, "objOrder",
              new Obj_SetOneIntOnObj(CreateSetter<ObjectProperty::ZOrder>()));
  m.AddUnsupportedOpcode(1033, 0, "objQuarterView");

  m.AddOpcode(1034, 0, "objDispRect", new dispArea_0);
  m.AddOpcode(1034, 1, "objDispRect", new dispRect_1);
  m.AddOpcode(1035, 0, "objDispCorner", new dispArea_0);
  m.AddOpcode(1035, 1, "objDispCorner", new dispArea_1);
  m.AddOpcode(1035, 2, "objDispCorner", new dispCorner_1);

  m.AddOpcode(1037, 0, "objSetDigitValue",
              new Obj_SetOneIntOnObj([](ParameterManager& param, int value) {
                auto digit = param.Get<ObjectProperty::DigitProperties>();
                digit.value = value;
                param.Set(ObjectProperty::DigitProperties, std::move(digit));
              }));
  m.AddOpcode(1038, 0, "objNumOpts", new objNumOpts);
  m.AddOpcode(
      1039, 0, "objPattNo",
      new Obj_SetOneIntOnObj(CreateSetter<ObjectProperty::PatternNumber>()));

  m.AddUnsupportedOpcode(1041, 0, "objAdjustAll");
  m.AddUnsupportedOpcode(1042, 0, "objAdjustAllX");
  m.AddUnsupportedOpcode(1043, 0, "objAdjustAllY");

  m.AddUnsupportedOpcode(1056, 0, "objFadeOpts");

  m.AddOpcode(1064, 2, "objButtonOpts", new objButtonOpts);
  m.AddOpcode(1066, 0, "objBtnState",
              new Obj_SetOneIntOnObj([](ParameterManager& param, int value) {
                auto btn = param.Get<ObjectProperty::ButtonProperties>();
                btn.state = value;
                param.Set(ObjectProperty::ButtonProperties, std::move(btn));
              }));

  m.AddOpcode(1070, 0, "objOwnDispArea", new dispOwnArea_0);
  m.AddOpcode(1070, 1, "objOwnDispArea", new dispOwnArea_1);
  m.AddOpcode(1071, 0, "objOwnDispRect", new dispOwnArea_0);
  m.AddOpcode(1071, 1, "objOwnDispRect", new dispOwnRect_1);
}

void addEveObjectFunctions(RLModule& m) {
  m.AddOpcode(
      2004, 0, "objEveDisplay",
      new Obj_SetOneIntOnObj(CreateSetter<ObjectProperty::IsVisible>()));
  m.AddOpcode(2004, 1, "objEveDisplay", new objEveDisplay_1);
  m.AddOpcode(2004, 2, "objEveDisplay", new objEveDisplay_2);
  m.AddOpcode(2004, 3, "objEveDisplay", new objEveDisplay_3);

  m.AddOpcode(3004, 0, "objEveDisplayCheck",
              new Op_MutatorCheck("objEveDisplay"));

  m.AddOpcode(4004, 0, "objEveDisplayWait",
              new Op_MutatorWaitNormal("objEveDisplay"));

  m.AddOpcode(5004, 0, "objEveDisplayWaitC",
              new Op_MutatorWaitCNormal("objEveDisplay"));

  m.AddOpcode(6004, 0, "objEveDisplayEnd",
              new Op_EndObjectMutation_Normal("objEveDisplay"));
}

}  // namespace

// -----------------------------------------------------------------------
// -----------------------------------------------------------------------
// -----------------------------------------------------------------------

ObjFgModule::ObjFgModule() : RLModule("ObjFg", 1, 81), helper_("obj", this) {
  addUnifiedFunctions(helper_);

  addObjectFunctions(*this);
  addEveObjectFunctions(*this);
  SetProperty(P_FGBG, OBJ_FG);
}

// -----------------------------------------------------------------------

ObjBgModule::ObjBgModule() : RLModule("ObjBg", 1, 82), helper_("objBg", this) {
  addUnifiedFunctions(helper_);

  addObjectFunctions(*this);
  addEveObjectFunctions(*this);
  SetProperty(P_FGBG, OBJ_BG);
}

// -----------------------------------------------------------------------

ChildObjFgModule::ChildObjFgModule()
    : MappedRLModule(ChildObjMappingFun, "ChildObjFg", 2, 81),
      helper_("objChild", this) {
  addUnifiedFunctions(helper_);

  addObjectFunctions(*this);
  addEveObjectFunctions(*this);
  SetProperty(P_FGBG, OBJ_FG);
}

// -----------------------------------------------------------------------

ChildObjBgModule::ChildObjBgModule()
    : MappedRLModule(ChildObjMappingFun, "ChildObjBg", 2, 82),
      helper_("objChildBg", this) {
  addUnifiedFunctions(helper_);

  addObjectFunctions(*this);
  addEveObjectFunctions(*this);
  SetProperty(P_FGBG, OBJ_BG);
}

// -----------------------------------------------------------------------

ObjRangeFgModule::ObjRangeFgModule()
    : MappedRLModule(RangeMappingFun, "ObjRangeFg", 1, 90),
      helper_("objRange", this) {
  addUnifiedFunctions(helper_);

  addObjectFunctions(*this);
  SetProperty(P_FGBG, OBJ_FG);
}

// -----------------------------------------------------------------------

ObjRangeBgModule::ObjRangeBgModule()
    : MappedRLModule(RangeMappingFun, "ObjRangeBg", 1, 91),
      helper_("objRangeBg", this) {
  addUnifiedFunctions(helper_);

  addObjectFunctions(*this);
  SetProperty(P_FGBG, OBJ_BG);
}

// -----------------------------------------------------------------------

ChildObjRangeFgModule::ChildObjRangeFgModule()
    : MappedRLModule(ChildRangeMappingFun, "ObjChildRangeFg", 2, 90),
      helper_("objChildRange", this) {
  addUnifiedFunctions(helper_);

  addObjectFunctions(*this);
  SetProperty(P_FGBG, OBJ_FG);
}

// -----------------------------------------------------------------------

ChildObjRangeBgModule::ChildObjRangeBgModule()
    : MappedRLModule(ChildRangeMappingFun, "ObjChildRangeBg", 2, 91),
      helper_("objChildRangeBg", this) {
  addUnifiedFunctions(helper_);

  addObjectFunctions(*this);
  SetProperty(P_FGBG, OBJ_BG);
}
