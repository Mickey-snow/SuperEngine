// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2006, 2007 Elliot Glaysher
// Copyright (C) 2025 Serina Sakurai
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

#include "base/colour.hpp"
#include "base/grprect.hpp"
#include "base/rect.hpp"
#include "base/tone_curve.hpp"

#include <functional>
#include <memory>
#include <vector>

class glFrameBuffer;
class ScreenCanvas;
struct SDL_Surface;
class GraphicsSystem;
class SDLGraphicsSystem;
class GraphicsObject;
class glTexture;

class SDLSurface;
using Surface = SDLSurface;

class SDLSurface {
 public:
  SDLSurface();
  // Surface created with a specified width and height
  SDLSurface(const Size& size);
  // Surface that takes ownership of an externally created surface.
  SDLSurface(SDL_Surface* surf, std::vector<GrpRect> region_table = {});
  ~SDLSurface();

  // Whether we have an underlying allocated surface.
  bool allocated() { return surface_; }

  void SetIsMask(const bool is) { is_mask_ = is; }

  void buildRegionTable(const Size& size);

  void allocate(const Size& size);
  void deallocate();

  Rect GetRect() const;

  void BlitToSurface(Surface& dest_surface,
                     const Rect& src,
                     const Rect& dst,
                     int alpha = 255,
                     bool use_src_alpha = true) const;

  void blitFROMSurface(SDL_Surface* src_surface,
                       const Rect& src,
                       const Rect& dst,
                       int alpha = 255,
                       bool use_src_alpha = true);

  void RenderToScreen(const Rect& src, const Rect& dst, int alpha = 255) const;

  void RenderToScreenAsColorMask(const Rect& src,
                                 const Rect& dst,
                                 const RGBAColour& rgba,
                                 int filter) const;

  void RenderToScreen(const Rect& src,
                      const Rect& dst,
                      const int opacity[4]) const;

  // Used internally; not exposed to the general graphics system
  void RenderToScreenAsObject(const GraphicsObject& rp,
                              const Rect& src,
                              const Rect& dst,
                              int alpha) const;

  int GetNumPatterns() const;

  // Returns pattern information.
  const GrpRect& GetPattern(int patt_no) const;

  // -----------------------------------------------------------------------

  Size GetSize() const;

  void Fill(const RGBAColour& colour);
  void Fill(const RGBAColour& colour, const Rect& area);
  void ToneCurve(const ToneCurveRGBMap effect, const Rect& area);
  void Invert(const Rect& rect);
  void Mono(const Rect& area);
  void ApplyColour(const RGBColour& colour, const Rect& area);

  SDL_Surface* surface() const { return surface_; }
  SDL_Surface* rawSurface() { return surface_; }

  void GetDCPixel(const Point& pos, int& r, int& g, int& b) const;

  RGBAColour GetPixel(Point pos) const;

  std::shared_ptr<Surface> ClipAsColorMask(const Rect& clip_rect,
                                           int r,
                                           int g,
                                           int b) const;

  Surface* Clone() const;

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
