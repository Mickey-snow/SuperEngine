// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2009 Elliot Glaysher
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

#include "modules/module_bgr.hpp"

#include <boost/algorithm/string.hpp>
#include <filesystem>
#include <iostream>
#include <string>

#include "base/colour.hpp"
#include "effects/effect.hpp"
#include "effects/effect_factory.hpp"
#include "machine/general_operations.hpp"
#include "machine/rlmachine.hpp"
#include "machine/rloperation.hpp"
#include "machine/rloperation/argc_t.hpp"
#include "machine/rloperation/complex_t.hpp"
#include "machine/rloperation/special_t.hpp"
#include "modules/module_grp.hpp"
#include "systems/base/graphics_system.hpp"
#include "systems/base/hik_renderer.hpp"
#include "systems/base/hik_script.hpp"
#include "systems/sdl_surface.hpp"
#include "systems/base/system.hpp"
#include "utilities/graphics.hpp"

namespace fs = std::filesystem;
using boost::iends_with;

static const std::set<std::string> HIK_FILETYPES = {"hik", "g00", "pdt"};

// Working theory of how this module works: The haikei module is one backing
// surface and (optionally) a HIK script. Games like AIR and the Maiden Halo
// demo use just the surface with a combination of bgrMulti and
// bgrLoadHaikei. OTOH, ALMA and planetarian use HIK scripts and the whole
// point of HIK scripts is to manipulate the backing surface on a timer that's
// divorced from the main interpreter loop.

namespace {

struct bgrLoadHaikei_blank : public RLOpcode<IntConstant_T> {
  void operator()(RLMachine& machine, int sel) {
    GraphicsSystem& graphics = machine.GetSystem().graphics();
    graphics.set_default_bgr_name("");
    graphics.SetHikRenderer(NULL);
    graphics.set_graphics_background(BACKGROUND_HIK);

    std::shared_ptr<Surface> before = graphics.RenderToSurface();
    graphics.GetHaikei()->Fill(RGBAColour::Clear());

    if (!machine.replaying_graphics_stack())
      graphics.ClearAndPromoteObjects();

    std::shared_ptr<Surface> after = graphics.RenderToSurface();

    LongOperation* effect =
        EffectFactory::BuildFromSEL(machine, after, before, sel);
    machine.PushLongOperation(effect);
  }
};

struct bgrLoadHaikei_main : RLOpcode<StrConstant_T, IntConstant_T> {
  void operator()(RLMachine& machine, std::string filename, int sel) {
    System& system = machine.GetSystem();
    GraphicsSystem& graphics = system.graphics();
    graphics.set_default_bgr_name(filename);
    graphics.set_graphics_background(BACKGROUND_HIK);

    // bgrLoadHaikei clears the stack.
    graphics.ClearStack();

    fs::path path = system.GetAssetScanner()->FindFile(filename, HIK_FILETYPES);
    if (iends_with(path.string(), "hik")) {
      if (!machine.replaying_graphics_stack())
        graphics.ClearAndPromoteObjects();

      graphics.SetHikRenderer(new HIKRenderer(
          system, graphics.GetHIKScript(system, filename, path)));
    } else {
      std::shared_ptr<Surface> before = graphics.RenderToSurface();

      if (!path.empty()) {
        std::shared_ptr<const Surface> source(
            graphics.GetSurfaceNamedAndMarkViewed(machine, filename));
        std::shared_ptr<Surface> haikei = graphics.GetHaikei();
        source->BlitToSurface(*haikei, source->GetRect(), source->GetRect(),
                              255, true);
      }

      // Promote the objects if we're in normal mode. If we're restoring the
      // graphics stack, we already have our layers promoted.
      if (!machine.replaying_graphics_stack())
        graphics.ClearAndPromoteObjects();

      std::shared_ptr<Surface> after = graphics.RenderToSurface();

      LongOperation* effect =
          EffectFactory::BuildFromSEL(machine, after, before, sel);
      machine.PushLongOperation(effect);
    }
  }
};

struct bgrLoadHaikei_wtf
    : RLOpcode<StrConstant_T, IntConstant_T, IntConstant_T, IntConstant_T> {
  void operator()(RLMachine& machine,
                  std::string filename,
                  int sel,
                  int a,
                  int b) {
    bgrLoadHaikei_main()(machine, filename, sel);
  }
};

struct bgrLoadHaikei_wtf2 : RLOpcode<StrConstant_T,
                                     IntConstant_T,
                                     IntConstant_T,
                                     IntConstant_T,
                                     IntConstant_T,
                                     IntConstant_T> {
  void operator()(RLMachine& machine,
                  string filename,
                  int sel,
                  int a,
                  int b,
                  int c,
                  int d) {
    // cerr << "Filename: " << filename
    //      << "(a: " << a << ", b: " << b << ", c: " << c << ", d: " << d <<
    // ")"
    //      << endl;
    bgrLoadHaikei_main()(machine, filename, sel);
  }
};

// -----------------------------------------------------------------------

typedef Argc_T<
    Special_T<DefaultSpecialMapper,
              // 0:copy(strC 'filename')
              StrConstant_T,
              // 1:DUMMY. Unknown.
              Complex_T<StrConstant_T, IntConstant_T>,
              // 2:copy(strC 'filename', '?')
              Complex_T<StrConstant_T, IntConstant_T>,
              // 3:DUMMY. Unknown.
              Complex_T<StrConstant_T, IntConstant_T>,
              // 4:copy(strC, '?', '?')
              Complex_T<StrConstant_T, IntConstant_T, IntConstant_T>>>
    BgrMultiCommand;

struct bgrMulti_1
    : public RLOpcode<StrConstant_T, IntConstant_T, BgrMultiCommand> {
 public:
  void operator()(RLMachine& machine,
                  string filename,
                  int effectNum,
                  BgrMultiCommand::type commands) {
    GraphicsSystem& graphics = machine.GetSystem().graphics();

    // Get the state of the world before we do any processing.
    std::shared_ptr<Surface> before = graphics.RenderToSurface();

    graphics.set_graphics_background(BACKGROUND_HIK);

    // May need to use current background.
    if (filename == "???")
      filename = graphics.default_bgr_name();

    // Load "filename" as the background.
    std::shared_ptr<const Surface> surface(
        graphics.GetSurfaceNamedAndMarkViewed(machine, filename));
    surface->BlitToSurface(*graphics.GetHaikei(), surface->GetRect(),
                           surface->GetRect(), 255, true);

    // TODO(erg): Unsure about the alpha in these implementation.
    for (BgrMultiCommand::type::const_iterator it = commands.begin();
         it != commands.end(); it++) {
      switch (it->type) {
        case 0: {
          // 0:copy(strC 'filename')
          surface = graphics.GetSurfaceNamedAndMarkViewed(machine, it->first);
          surface->BlitToSurface(*graphics.GetHaikei(), surface->GetRect(),
                                 surface->GetRect(), 255, true);
          break;
        }
        case 2: {
          // 2:copy(strC 'filename', '?')
          Rect srcRect;
          Point dest;
          GetSELPointAndRect(machine, std::get<1>(it->third), srcRect, dest);

          surface = graphics.GetSurfaceNamedAndMarkViewed(
              machine, std::get<0>(it->third));
          Rect destRect = Rect(dest, srcRect.size());
          surface->BlitToSurface(*graphics.GetHaikei(), srcRect, destRect, 255,
                                 true);
          break;
        }
        default: {
          std::cerr << "Don't know what to do with a type " << it->type
                    << " in bgrMulti_1" << std::endl;
          break;
        }
      }
    }

    // Promote the objects if we're in normal mode. If we're restoring the
    // graphics stack, we already have our layers promoted.
    if (!machine.replaying_graphics_stack())
      graphics.ClearAndPromoteObjects();

    std::shared_ptr<Surface> after = graphics.RenderToSurface();
    LongOperation* effect =
        EffectFactory::BuildFromSEL(machine, after, before, effectNum);
    machine.PushLongOperation(effect);
  }
};

struct bgrNext : public RLOpcode<> {
  void operator()(RLMachine& machine) {
    HIKRenderer* renderer = machine.GetSystem().graphics().hik_renderer();
    if (renderer) {
      renderer->NextAnimationFrame();
    }
  }
};

struct bgrSetXOffset : public RLOpcode<IntConstant_T> {
  void operator()(RLMachine& machine, int offset) {
    HIKRenderer* renderer = machine.GetSystem().graphics().hik_renderer();
    if (renderer) {
      renderer->set_x_offset(offset);
    }
  }
};

struct bgrSetYOffset : public RLOpcode<IntConstant_T> {
  void operator()(RLMachine& machine, int offset) {
    HIKRenderer* renderer = machine.GetSystem().graphics().hik_renderer();
    if (renderer) {
      renderer->set_y_offset(offset);
    }
  }
};

struct bgrPreloadScript : public RLOpcode<IntConstant_T, StrConstant_T> {
  void operator()(RLMachine& machine, int slot, string name) {
    System& system = machine.GetSystem();
    fs::path path = system.GetAssetScanner()->FindFile(name, HIK_FILETYPES);
    if (iends_with(path.string(), "hik")) {
      system.graphics().PreloadHIKScript(system, slot, name, path);
    }
  }
};

}  // namespace

// -----------------------------------------------------------------------

BgrModule::BgrModule() : MappedRLModule(GraphicsStackMappingFun, "Bgr", 1, 40) {
  AddOpcode(10, 0, "bgrLoadHaikei", new bgrLoadHaikei_blank);
  AddOpcode(10, 1, "bgrLoadHaikei", new bgrLoadHaikei_main);
  AddOpcode(10, 2, "bgrLoadHaikei", new bgrLoadHaikei_wtf);
  AddOpcode(10, 3, "bgrLoadHaikei", new bgrLoadHaikei_wtf2);

  AddUnsupportedOpcode(100, 0, "bgrMulti");
  AddOpcode(100, 1, "bgrMulti", new bgrMulti_1);

  AddOpcode(1000, 0, "bgrNext", new bgrNext);

  AddOpcode(1104, 0, "bgrSetXOffset", new bgrSetXOffset);
  AddOpcode(1105, 0, "bgrSetYOffset", new bgrSetYOffset);

  AddOpcode(2000, 0, "bgrPreloadScript", new bgrPreloadScript);
  AddOpcode(2001, 0, "bgrClearPreloadedScript",
            CallFunction(&GraphicsSystem::ClearPreloadedHIKScript));
  AddOpcode(2002, 0, "bgrClearAllPreloadedScripts",
            CallFunction(&GraphicsSystem::ClearAllPreloadedHIKScripts));
}
