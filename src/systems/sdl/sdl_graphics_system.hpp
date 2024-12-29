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

#pragma once

#include "GL/glew.h"

#include <memory>
#include <set>
#include <string>

#include "base/notification/observer.hpp"
#include "base/notification/registrar.hpp"
#include "systems/base/graphics_system.hpp"

struct SDL_Surface;
class Gameexe;
class GraphicsObject;
class SDLGraphicsSystem;
class SDLSurface;
class System;
class Texture;
class glTexture;
class glCanvas;

// -----------------------------------------------------------------------

// Implements all screen output and screen management functionality.
//
// TODO(erg): This public interface really needs to be rethought out.
class SDLGraphicsSystem : public GraphicsSystem, public NotificationObserver {
 public:
  // SDL should be initialized before you create an SDLGraphicsSystem.
  SDLGraphicsSystem(System& system, Gameexe& gameexe);
  ~SDLGraphicsSystem();

 public:
  std::shared_ptr<SDLSurface> CreateSurface(Size size);
  virtual std::shared_ptr<Surface> BuildSurface(const Size& size) override;
  virtual std::shared_ptr<const Surface> LoadSurfaceFromFile(
      const std::string& short_filename) override;
  std::shared_ptr<Surface> CreateSurface(SDL_Surface* surface);

 public:
  // When the cursor is changed, also make sure that it exists so that we can
  // switch on/off the operating system cursor when the cursor index is invalid.
  virtual void SetCursor(int cursor) override;

  std::shared_ptr<glCanvas> CreateCanvas() const;
  virtual void BeginFrame() override;
  virtual void EndFrame() override;
  virtual std::shared_ptr<Surface> RenderToSurface() override;

  void RedrawLastFrame();
  void DrawCursor();

  virtual void MarkScreenAsDirty(GraphicsUpdateType type) override;

  virtual void ExecuteGraphicsSystem(RLMachine& machine) override;

  virtual void AllocateDC(int dc, Size screen_size) override;
  virtual void SetMinimumSizeForDC(int dc, Size size) override;
  virtual void FreeDC(int dc) override;

  virtual std::shared_ptr<Surface> GetHaikei() override;
  virtual std::shared_ptr<Surface> GetDC(int dc) override;

  virtual ColourFilter* BuildColourFiller() override;

  // -----------------------------------------------------------------------
  virtual void SetWindowTitle(std::string new_caption);
  virtual void SetWindowSubtitle(const std::string& cp932str,
                                 int text_encoding) override;

  virtual void SetScreenMode(const int in) override;

  // Reset the system. Should clear all state for when a user loads a
  // game.
  virtual void Reset() override;

  void SetupVideo(Size window_size);

  void Resize(Size display_size);

  Size GetDisplaySize() const noexcept;

 private:
  // Makes sure that a passed in dc number is valid.
  //
  // @exception Error Throws when dc is greater then the maximum.
  // @exception Error Throws when dc is unallocated.
  void VerifySurfaceExists(int dc, const std::string& caller);

  // NotificationObserver:
  virtual void Observe(NotificationType type,
                       const NotificationSource& source,
                       const NotificationDetails& details) override;

  // ---------------------------------------------------------------------

  SDL_Surface* screen_;

  std::shared_ptr<SDLSurface> haikei_;
  std::shared_ptr<SDLSurface> display_contexts_[16];

  bool redraw_last_frame_;

  // utf8 encoded title string
  std::string caption_title_;
  // utf8 encoded subtitle
  std::string subtitle_;

  // Texture used to store the contents of the screen while in DrawManual()
  // mode. The stored image is then used if we need to redraw in the
  // intervening time (expose events, mouse cursor moves, etc).
  std::shared_ptr<glTexture> screen_contents_texture_;

  // Whether |screen_contents_texture_| is valid to use.
  bool screen_contents_texture_valid_;

  NotificationRegistrar registrar_;

  Size display_size_;
};
