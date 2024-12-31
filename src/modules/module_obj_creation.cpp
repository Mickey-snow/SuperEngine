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

#include "modules/module_obj_creation.hpp"

#include "machine/properties.hpp"
#include "machine/rlmachine.hpp"
#include "machine/rlmodule.hpp"
#include "machine/rloperation.hpp"
#include "machine/rloperation/default_value.hpp"
#include "machine/rloperation/rect_t.hpp"
#include "modules/module_obj.hpp"
#include "object/drawer/anm.hpp"
#include "object/drawer/colour_filter.hpp"
#include "object/drawer/digits.hpp"
#include "object/drawer/drift.hpp"
#include "object/drawer/file.hpp"
#include "object/drawer/gan.hpp"
#include "object/drawer/parent.hpp"
#include "object/drawer/text.hpp"
#include "systems/base/graphics_object.hpp"
#include "systems/base/graphics_system.hpp"
#include "systems/sdl_surface.hpp"
#include "systems/base/system.hpp"
#include "utilities/graphics.hpp"
#include "utilities/string_utilities.hpp"

#include <cmath>
#include <filesystem>
#include <memory>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

// -----------------------------------------------------------------------

namespace {

void SetObjectDataToGan(RLMachine& machine,
                        GraphicsObject& obj,
                        std::string& imgFilename,
                        const std::string& ganFilename) {
  // TODO(erg): This is a hack and probably a source of errors. Figure out what
  // '???' means when used as the first parameter to objOfFileGan.
  if (imgFilename == "???")
    imgFilename = ganFilename;
  obj.SetObjectData(
      new GanGraphicsObjectData(machine.GetSystem(), ganFilename, imgFilename));
}

typedef std::function<void(RLMachine&, GraphicsObject& obj, const string&)>
    DataFunction;

void objOfFileLoader(RLMachine& machine,
                     GraphicsObject& obj,
                     const std::string& filename) {
  static const std::set<std::string> OBJ_FILETYPES = {"anm", "g00", "pdt"};

  auto& system = machine.GetSystem();

  // Get the path to get the file type (which won't be in filename)
  fs::path full_path =
      machine.GetSystem().GetAssetScanner()->FindFile(filename, OBJ_FILETYPES);
  if (full_path.empty()) {
    std::stringstream oss;
    oss << "Could not find Object compatible file \"" << filename << "\".";
    throw std::runtime_error(oss.str());
  }

  string file_str = full_path.string();
  GraphicsObjectData* obj_data = nullptr;
  if (file_str.ends_with("g00") || file_str.ends_with("pdt")) {
    obj_data = new GraphicsObjectOfFile(system, filename);
  } else if (file_str.ends_with("anm")) {
    obj_data = new AnmGraphicsObjectData(system, filename);
  } else {
    std::ostringstream oss;
    oss << "Don't know how to handle object file: \"" << filename << "\"";
    throw std::runtime_error(oss.str());
  }

  obj.SetObjectData(obj_data);
}

void objOfTextBuilder(RLMachine& machine,
                      GraphicsObject& obj,
                      const std::string& val) {
  // The text at this point is still cp932. Convert it.
  std::string utf8str = cp932toUTF8(val, machine.GetTextEncoding());
  obj.Param().SetTextText(utf8str);
  GraphicsTextObject* text_obj = new GraphicsTextObject(machine.GetSystem());
  obj.SetObjectData(text_obj);
  text_obj->UpdateSurface(obj);
}

void objOfDriftLoader(RLMachine& machine,
                      GraphicsObject& obj,
                      const std::string& value) {
  obj.SetObjectData(new DriftGraphicsObject(machine.GetSystem(), value));
}

void objOfDigitsLoader(RLMachine& machine,
                       GraphicsObject& obj,
                       const std::string& value) {
  obj.SetObjectData(new DigitsGraphicsObject(machine.GetSystem(), value));
}

struct objGeneric_0 : public RLOpcode<IntConstant_T, StrConstant_T> {
  DataFunction data_fun_;
  explicit objGeneric_0(const DataFunction& fun) : data_fun_(fun) {}

  void operator()(RLMachine& machine, int buf, std::string filename) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    data_fun_(machine, obj, filename);
  }
};

struct objGeneric_1
    : public RLOpcode<IntConstant_T, StrConstant_T, IntConstant_T> {
  DataFunction data_fun_;
  explicit objGeneric_1(const DataFunction& fun) : data_fun_(fun) {}

  void operator()(RLMachine& machine,
                  int buf,
                  std::string filename,
                  int visible) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    data_fun_(machine, obj, filename);
    obj.Param().SetVisible(visible);
  }
};

struct objGeneric_2 : public RLOpcode<IntConstant_T,
                                      StrConstant_T,
                                      IntConstant_T,
                                      IntConstant_T,
                                      IntConstant_T> {
  DataFunction data_fun_;
  explicit objGeneric_2(const DataFunction& fun) : data_fun_(fun) {}

  void operator()(RLMachine& machine,
                  int buf,
                  std::string filename,
                  int visible,
                  int x,
                  int y) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    data_fun_(machine, obj, filename);
    obj.Param().SetVisible(visible);
    obj.Param().SetX(x);
    obj.Param().SetY(y);
  }
};

struct objGeneric_3 : public RLOpcode<IntConstant_T,
                                      StrConstant_T,
                                      IntConstant_T,
                                      IntConstant_T,
                                      IntConstant_T,
                                      IntConstant_T> {
  DataFunction data_fun_;
  explicit objGeneric_3(const DataFunction& fun) : data_fun_(fun) {}

  void operator()(RLMachine& machine,
                  int buf,
                  string filename,
                  int visible,
                  int x,
                  int y,
                  int pattern) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    data_fun_(machine, obj, filename);
    obj.Param().SetVisible(visible);
    obj.Param().SetX(x);
    obj.Param().SetY(y);
    obj.Param().SetPattNo(pattern);
  }
};

struct objGeneric_4 : public RLOpcode<IntConstant_T,
                                      StrConstant_T,
                                      IntConstant_T,
                                      IntConstant_T,
                                      IntConstant_T,
                                      IntConstant_T,
                                      IntConstant_T,
                                      IntConstant_T> {
  DataFunction data_fun_;
  explicit objGeneric_4(const DataFunction& fun) : data_fun_(fun) {}

  void operator()(RLMachine& machine,
                  int buf,
                  string filename,
                  int visible,
                  int x,
                  int y,
                  int pattern,
                  int scrollX,
                  int scrollY) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);

    data_fun_(machine, obj, filename);
    obj.Param().SetVisible(visible);
    obj.Param().SetX(x);
    obj.Param().SetY(y);
    obj.Param().SetPattNo(pattern);
    obj.Param().SetScrollRateX(scrollX);
    obj.Param().SetScrollRateY(scrollY);
  }
};

struct objOfFileGan_0
    : public RLOpcode<IntConstant_T, StrConstant_T, StrConstant_T> {
  void operator()(RLMachine& machine,
                  int buf,
                  string imgFilename,
                  string ganFilename) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    SetObjectDataToGan(machine, obj, imgFilename, ganFilename);
    obj.Param().SetVisible(true);
  }
};

struct objOfFileGan_1 : public RLOpcode<IntConstant_T,
                                        StrConstant_T,
                                        StrConstant_T,
                                        IntConstant_T> {
  void operator()(RLMachine& machine,
                  int buf,
                  string imgFilename,
                  string ganFilename,
                  int visible) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    SetObjectDataToGan(machine, obj, imgFilename, ganFilename);
    obj.Param().SetVisible(visible);
  }
};

struct objOfFileGan_2 : public RLOpcode<IntConstant_T,
                                        StrConstant_T,
                                        StrConstant_T,
                                        IntConstant_T,
                                        IntConstant_T,
                                        IntConstant_T> {
  void operator()(RLMachine& machine,
                  int buf,
                  string imgFilename,
                  string ganFilename,
                  int visible,
                  int x,
                  int y) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    SetObjectDataToGan(machine, obj, imgFilename, ganFilename);
    obj.Param().SetVisible(visible);
    obj.Param().SetX(x);
    obj.Param().SetY(y);
  }
};

struct objOfFileGan_3 : public RLOpcode<IntConstant_T,
                                        StrConstant_T,
                                        StrConstant_T,
                                        IntConstant_T,
                                        IntConstant_T,
                                        IntConstant_T,
                                        IntConstant_T> {
  void operator()(RLMachine& machine,
                  int buf,
                  string imgFilename,
                  string ganFilename,
                  int visible,
                  int x,
                  int y,
                  int pattern) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    SetObjectDataToGan(machine, obj, imgFilename, ganFilename);
    obj.Param().SetVisible(visible);
    obj.Param().SetX(x);
    obj.Param().SetY(y);
    obj.Param().SetPattNo(pattern);
  }
};

void SetObjectDataToRect(RLMachine& machine,
                         RLOperation* op,
                         int buf,
                         const Rect& r) {
  GraphicsObject& obj = GetGraphicsObject(machine, op, buf);
  obj.SetObjectData(
      new ColourFilterObjectData(machine.GetSystem().graphics(), r));
}

struct objOfArea_0 : public RLOpcode<IntConstant_T> {
  void operator()(RLMachine& machine, int buf) {
    Rect rect(0, 0, GetScreenSize(machine.GetSystem().gameexe()));
    SetObjectDataToRect(machine, this, buf, rect);
  }
};

struct objOfArea_1 : public RLOpcode<IntConstant_T, Rect_T<rect_impl::GRP>> {
  void operator()(RLMachine& machine, int buf, Rect rect) {
    SetObjectDataToRect(machine, this, buf, rect);
  }
};

struct objOfArea_2
    : public RLOpcode<IntConstant_T, Rect_T<rect_impl::GRP>, IntConstant_T> {
  void operator()(RLMachine& machine, int buf, Rect rect, int visible) {
    SetObjectDataToRect(machine, this, buf, rect);

    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    obj.Param().SetVisible(visible);
  }
};

struct objOfRect_1 : public RLOpcode<IntConstant_T,
                                     IntConstant_T,
                                     IntConstant_T,
                                     DefaultIntValue_T<INT_MIN>,
                                     DefaultIntValue_T<INT_MIN>> {
  void operator()(RLMachine& machine,
                  int buf,
                  int x,
                  int y,
                  int width,
                  int height) {
    // Because of the screwed up optionality here, (x and y are optional while
    // width height are not, we hack with the INT_MIN value).
    if (width == INT_MIN) {
      Rect screen(0, 0, GetScreenSize(machine.GetSystem().gameexe()));
      SetObjectDataToRect(machine, this, buf, Size(x, y).CenteredIn(screen));
    } else {
      SetObjectDataToRect(machine, this, buf, Rect(x, y, Size(width, height)));
    }
  }
};

struct objOfRect_2 : public RLOpcode<IntConstant_T,
                                     IntConstant_T,
                                     IntConstant_T,
                                     IntConstant_T,
                                     DefaultIntValue_T<INT_MIN>,
                                     DefaultIntValue_T<INT_MIN>> {
  void operator()(RLMachine& machine,
                  int buf,
                  int x,
                  int y,
                  int width,
                  int height,
                  int visible) {
    Rect data_rect;
    if (height == INT_MIN) {
      Rect screen(0, 0, GetScreenSize(machine.GetSystem().gameexe()));
      data_rect = Size(x, y).CenteredIn(screen);
      // Crazy optionality here.
      visible = width;
    } else {
      data_rect = Rect(x, y, Size(width, height));
    }

    SetObjectDataToRect(machine, this, buf, data_rect);
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    obj.Param().SetVisible(visible);
  }
};

// reallive.kfn and the rldev docs disagree about whether there's an
// objOfRect_4. Blank until I see it in the wild.

struct objOfChild_0 : public RLOpcode<IntConstant_T,
                                      IntConstant_T,
                                      StrConstant_T,
                                      StrConstant_T> {
  void operator()(RLMachine& machine,
                  int buf,
                  int count,
                  string imgFilename,
                  string ganFilename) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    obj.SetObjectData(new ParentGraphicsObjectData(count));
    obj.Param().SetVisible(true);
  }
};

struct objOfChild_1 : public RLOpcode<IntConstant_T,
                                      IntConstant_T,
                                      StrConstant_T,
                                      StrConstant_T,
                                      IntConstant_T> {
  void operator()(RLMachine& machine,
                  int buf,
                  int count,
                  string imgFilename,
                  string ganFilename,
                  int visible) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    obj.SetObjectData(new ParentGraphicsObjectData(count));
    obj.Param().SetVisible(visible);
  }
};

struct objOfChild_2 : public RLOpcode<IntConstant_T,
                                      IntConstant_T,
                                      StrConstant_T,
                                      StrConstant_T,
                                      IntConstant_T,
                                      IntConstant_T,
                                      IntConstant_T> {
  void operator()(RLMachine& machine,
                  int buf,
                  int count,
                  string imgFilename,
                  string ganFilename,
                  int visible,
                  int x,
                  int y) {
    GraphicsObject& obj = GetGraphicsObject(machine, this, buf);
    obj.SetObjectData(new ParentGraphicsObjectData(count));
    obj.Param().SetVisible(visible);
    obj.Param().SetX(x);
    obj.Param().SetY(y);
  }
};

}  // namespace

// -----------------------------------------------------------------------

void addObjectCreationFunctions(RLModule& m) {
  m.AddOpcode(1000, 0, "objOfFile", new objGeneric_0(objOfFileLoader));
  m.AddOpcode(1000, 1, "objOfFile", new objGeneric_1(objOfFileLoader));
  m.AddOpcode(1000, 2, "objOfFile", new objGeneric_2(objOfFileLoader));
  m.AddOpcode(1000, 3, "objOfFile", new objGeneric_3(objOfFileLoader));
  m.AddOpcode(1000, 4, "objOfFile", new objGeneric_4(objOfFileLoader));

  m.AddOpcode(1001, 0, "objOfFile2", new objGeneric_0(objOfFileLoader));
  m.AddOpcode(1001, 1, "objOfFile2", new objGeneric_2(objOfFileLoader));

  m.AddOpcode(1003, 0, "objOfFileGan", new objOfFileGan_0);
  m.AddOpcode(1003, 1, "objOfFileGan", new objOfFileGan_1);
  m.AddOpcode(1003, 2, "objOfFileGan", new objOfFileGan_2);
  m.AddOpcode(1003, 3, "objOfFileGan", new objOfFileGan_3);
  m.AddUnsupportedOpcode(1003, 4, "objOfFileGan");

  m.AddOpcode(1100, 0, "objOfArea", new objOfArea_0);
  m.AddOpcode(1100, 1, "objOfArea", new objOfArea_1);
  m.AddOpcode(1100, 2, "objOfArea", new objOfArea_2);

  m.AddOpcode(1101, 0, "objOfRect", new objOfArea_0);
  m.AddOpcode(1101, 1, "objOfRect", new objOfRect_1);
  m.AddOpcode(1101, 2, "objOfRect", new objOfRect_2);
  m.AddUnsupportedOpcode(1101, 3, "objOfRect");

  m.AddOpcode(1200, 0, "objOfText", new objGeneric_0(objOfTextBuilder));
  m.AddOpcode(1200, 1, "objOfText", new objGeneric_1(objOfTextBuilder));
  m.AddOpcode(1200, 2, "objOfText", new objGeneric_2(objOfTextBuilder));
  m.AddUnsupportedOpcode(1200, 3, "objOfText");

  m.AddOpcode(1300, 0, "objDriftOfFile", new objGeneric_0(objOfDriftLoader));
  m.AddOpcode(1300, 1, "objDriftOfFile", new objGeneric_1(objOfDriftLoader));
  m.AddOpcode(1300, 2, "objDriftOfFile", new objGeneric_2(objOfDriftLoader));
  m.AddOpcode(1300, 3, "objDriftOfFile", new objGeneric_3(objOfDriftLoader));

  m.AddOpcode(1400, 0, "objOfDigits", new objGeneric_0(objOfDigitsLoader));
  m.AddOpcode(1400, 1, "objOfDigits", new objGeneric_1(objOfDigitsLoader));
  m.AddOpcode(1400, 2, "objOfDigits", new objGeneric_2(objOfDigitsLoader));
  m.AddOpcode(1400, 3, "objOfDigits", new objGeneric_3(objOfDigitsLoader));

  m.AddOpcode(1500, 0, "objOfChild", new objOfChild_0);
  m.AddOpcode(1500, 1, "objOfChild", new objOfChild_1);
  m.AddOpcode(1500, 2, "objOfChild", new objOfChild_2);
}

// -----------------------------------------------------------------------

ObjFgCreationModule::ObjFgCreationModule() : RLModule("ObjFgCreation", 1, 71) {
  addObjectCreationFunctions(*this);
  SetProperty(P_FGBG, OBJ_FG);
}

// -----------------------------------------------------------------------

ObjBgCreationModule::ObjBgCreationModule() : RLModule("ObjBgCreation", 1, 72) {
  addObjectCreationFunctions(*this);
  SetProperty(P_FGBG, OBJ_BG);
}

// -----------------------------------------------------------------------

ChildObjFgCreationModule::ChildObjFgCreationModule()
    : MappedRLModule(ChildObjMappingFun, "ChildObjFgCreation", 2, 71) {
  addObjectCreationFunctions(*this);
  SetProperty(P_FGBG, OBJ_FG);
}

// -----------------------------------------------------------------------

ChildObjBgCreationModule::ChildObjBgCreationModule()
    : MappedRLModule(ChildObjMappingFun, "ChildObjBgCreation", 2, 72) {
  addObjectCreationFunctions(*this);
  SetProperty(P_FGBG, OBJ_BG);
}
