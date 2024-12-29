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

#include <vector>

#include "base/tone_curve.hpp"
#include "systems/base/surface.hpp"

class glFrameBuffer;
class ScreenCanvas;
struct SDL_Surface;
class GraphicsSystem;
class SDLGraphicsSystem;
class GraphicsObject;
class glTexture;

// Wrapper around an OpenGL texture; meant to be passed out of the
// graphics system.
//
// Some SDLSurfaces will own their underlying SDL_Surface, for
// example, anything returned from GetSurfaceNamedAndMarkViewed(), while others
// don't own their surfaces (SDLSurfaces returned by GetDC()
class SDLSurface : public Surface {
 public:
  SDLSurface();
  // Surface created with a specified width and height
  SDLSurface(const Size& size);
  // Surface that takes ownership of an externally created surface.
  SDLSurface(SDL_Surface* surf, std::vector<GrpRect> region_table = {});
  ~SDLSurface();

  virtual void EnsureUploaded() const override;

  // Whether we have an underlying allocated surface.
  bool allocated() { return surface_; }

  virtual void SetIsMask(const bool is) override { is_mask_ = is; }

  void buildRegionTable(const Size& size);

  void allocate(const Size& size);
  void deallocate();

  virtual void BlitToSurface(Surface& dest_surface,
                             const Rect& src,
                             const Rect& dst,
                             int alpha = 255,
                             bool use_src_alpha = true) const override;

  void blitFROMSurface(SDL_Surface* src_surface,
                       const Rect& src,
                       const Rect& dst,
                       int alpha = 255,
                       bool use_src_alpha = true);

  virtual void RenderToScreen(const Rect& src,
                              const Rect& dst,
                              int alpha = 255) const override;

  virtual void RenderToScreenAsColorMask(const Rect& src,
                                         const Rect& dst,
                                         const RGBAColour& rgba,
                                         int filter) const override;

  virtual void RenderToScreen(const Rect& src,
                              const Rect& dst,
                              const int opacity[4]) const override;

  // Used internally; not exposed to the general graphics system
  virtual void RenderToScreenAsObject(const GraphicsObject& rp,
                                      const Rect& src,
                                      const Rect& dst,
                                      int alpha) const override;

  virtual int GetNumPatterns() const override;

  // Returns pattern information.
  virtual const GrpRect& GetPattern(int patt_no) const override;

  // -----------------------------------------------------------------------

  virtual Size GetSize() const override;

  virtual void Fill(const RGBAColour& colour) override;
  virtual void Fill(const RGBAColour& colour, const Rect& area) override;
  virtual void ToneCurve(const ToneCurveRGBMap effect,
                         const Rect& area) override;
  virtual void Invert(const Rect& rect) override;
  virtual void Mono(const Rect& area) override;
  virtual void ApplyColour(const RGBColour& colour, const Rect& area) override;

  SDL_Surface* surface() const { return surface_; }
  SDL_Surface* rawSurface() { return surface_; }

  virtual void GetDCPixel(const Point& pos,
                          int& r,
                          int& g,
                          int& b) const override;

  virtual RGBAColour GetPixel(Point pos) const;

  virtual std::shared_ptr<Surface> ClipAsColorMask(const Rect& clip_rect,
                                                   int r,
                                                   int g,
                                                   int b) const override;

  virtual Surface* Clone() const override;

  // Called after each change to surface_. Marks the texture as
  // invalid and notifies SDLGraphicsSystem when appropriate.
  void markWrittenTo(const Rect& written_rect);

  std::vector<char> Dump(Rect region) const;

  // Keeps track of a texture and the information about which region
  // of the current surface this Texture is. We keep track of this
  // information so we can reupload a certain part of the Texture
  // without allocating a new OpenGL texture. glGenTexture()/
  // glTexImage2D() is SLOW and should never be done in a loop.)
  struct TextureRecord {
    TextureRecord(SDLSurface const* surface,
                  int x,
                  int y,
                  int w,
                  int h,
                  unsigned int bytes_per_pixel,
                  int byte_order,
                  int byte_type);

    // Reuploads this current piece of surface from the supplied
    // surface without allocating a new texture.
    void reupload(SDLSurface const* surface, Rect dirty);

    // Clears |texture|. Called before a switch between windowed and
    // fullscreen mode, so that we aren't holding stale references.
    void forceUnload();

    // The actual texture.
    std::shared_ptr<glTexture> gltexture;

    int x_, y_, w_, h_;
    unsigned int bytes_per_pixel_;
    int byte_order_, byte_type_;
  };

  std::vector<TextureRecord> GetTextureArray() const;

 private:
  // Makes sure that texture_ is a valid object and that it's
  // updated. This method should be called before doing anything with
  // texture_.
  void uploadTextureIfNeeded() const;

  static std::vector<int> segmentPicture(int size_remainging);

  // The SDL_Surface that contains the software version of the bitmap.
  SDL_Surface* surface_;

  // The region table
  std::vector<GrpRect> region_table_;

  // The SDLTexture which wraps one or more OpenGL textures
  mutable std::vector<TextureRecord> textures_;

  // Whether texture_ represents the contents of surface_. Blits
  // from surfaces to surfaces invalidate the target surface's
  // texture.
  mutable bool texture_is_valid_;

  // When a chunk of the surface is invalidated, we only want to upload the
  // smallest possible area, but for simplicity, we only keep one dirty area.
  mutable Rect dirty_rectangle_;

  bool is_mask_;

 public:
  static std::shared_ptr<glFrameBuffer> screen_;
};
