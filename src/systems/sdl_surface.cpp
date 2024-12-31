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

#include "systems/sdl_surface.hpp"

#include <SDL/SDL.h>

#include "base/colour.hpp"
#include "base/localrect.hpp"
#include "pygame/alphablit.h"
#include "systems/base/graphics_object.hpp"
#include "systems/gl_frame_buffer.hpp"
#include "systems/glrenderer.hpp"
#include "systems/gltexture.hpp"
#include "systems/screen_canvas.hpp"
#include "systems/sdl/sdl_graphics_system.hpp"
#include "systems/sdl/sdl_utils.hpp"
#include "utilities/graphics.hpp"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/matrix.hpp"

#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>
#include <vector>

std::shared_ptr<glFrameBuffer> SDLSurface::screen_ = nullptr;

namespace {

// An interface to TransformSurface that maps one color to another.
class ColourTransformer {
 public:
  virtual ~ColourTransformer() {}
  virtual SDL_Color operator()(const SDL_Color& colour) const = 0;
};

class ToneCurveColourTransformer : public ColourTransformer {
 public:
  explicit ToneCurveColourTransformer(const ToneCurveRGBMap m) : colormap(m) {}
  virtual SDL_Color operator()(const SDL_Color& colour) const {
    SDL_Color out = {colormap[0][colour.r], colormap[1][colour.g],
                     colormap[2][colour.b], 0};
    return out;
  }

 private:
  ToneCurveRGBMap colormap;
};

class InvertColourTransformer : public ColourTransformer {
 public:
  virtual SDL_Color operator()(const SDL_Color& colour) const {
    SDL_Color out = {255 - colour.r, 255 - colour.g, 255 - colour.b, 0};
    return out;
  }
};

class MonoColourTransformer : public ColourTransformer {
 public:
  virtual SDL_Color operator()(const SDL_Color& colour) const {
    float grayscale = 0.3 * colour.r + 0.59 * colour.g + 0.11 * colour.b;

    static const auto Clamp = [](float& var, float min, float max) {
      if (var < min)
        var = min;
      else if (var > max)
        var = max;
    };
    Clamp(grayscale, 0, 255);

    SDL_Color out = {grayscale, grayscale, grayscale, 0};
    return out;
  }
};

class ApplyColourTransformer : public ColourTransformer {
 public:
  explicit ApplyColourTransformer(const RGBColour& colour) : colour_(colour) {}

  int compose(int in_colour, int surface_colour) const {
    if (in_colour > 0) {
      return 255 -
             ((static_cast<float>((255 - in_colour) * (255 - surface_colour)) /
               (255 * 255)) *
              255);
    } else if (in_colour < 0) {
      return (static_cast<float>(abs(in_colour) * surface_colour) /
              (255 * 255)) *
             255;
    } else {
      return surface_colour;
    }
  }

  virtual SDL_Color operator()(const SDL_Color& colour) const {
    SDL_Color out = {compose(colour_.r(), colour.r),
                     compose(colour_.g(), colour.g),
                     compose(colour_.b(), colour.b), 0};
    return out;
  }

 private:
  RGBColour colour_;
};

// Applies a |transformer| to every pixel in |area| in the surface |surface|.
void TransformSurface(SDLSurface* our_surface,
                      const Rect& area,
                      const ColourTransformer& transformer) {
  SDL_Surface* surface = our_surface->rawSurface();
  SDL_Color colour;
  Uint32 col = 0;

  // determine position
  char* p_position = (char*)surface->pixels;

  // offset by y
  p_position += (surface->pitch * area.y());

  SDL_LockSurface(surface);
  {
    for (int y = 0; y < area.height(); ++y) {
      // advance forward x
      p_position += (surface->format->BytesPerPixel * area.x());

      for (int x = 0; x < area.width(); ++x) {
        // copy pixel data
        memcpy(&col, p_position, surface->format->BytesPerPixel);

        // Before someone tries to simplify the following four lines,
        // remember that sizeof(int) != sizeof(Uint8).
        Uint8 alpha;
        SDL_GetRGBA(col, surface->format, &colour.r, &colour.g, &colour.b,
                    &alpha);
        SDL_Color out = transformer(colour);
        Uint32 out_colour =
            SDL_MapRGBA(surface->format, out.r, out.g, out.b, alpha);

        memcpy(p_position, &out_colour, surface->format->BytesPerPixel);

        p_position += surface->format->BytesPerPixel;
      }

      // advance forward image_width - area.width() - x
      int advance = surface->w - area.x() - area.width();
      p_position += (surface->format->BytesPerPixel * advance);
    }
  }
  SDL_UnlockSurface(surface);

  // If we are the main screen, then we want to update the screen
  our_surface->markWrittenTo(our_surface->GetRect());
}

}  // namespace

// -----------------------------------------------------------------------

// Note to self: These describe the byte order IN THE RAW G00 DATA!
// These should NOT be switched to native byte order.
#define DefaultRmask 0xff0000
#define DefaultGmask 0xff00
#define DefaultBmask 0xff
#define DefaultAmask 0xff000000
#define DefaultBpp 32

SDL_Surface* buildNewSurface(const Size& size) {
  // Create an empty surface
  SDL_Surface* tmp = SDL_CreateRGBSurface(
      SDL_SWSURFACE | SDL_SRCALPHA, size.width(), size.height(), DefaultBpp,
      DefaultRmask, DefaultGmask, DefaultBmask, DefaultAmask);

  if (tmp == NULL) {
    std::ostringstream ss;
    ss << "Couldn't allocate surface in build_new_surface"
       << ": " << SDL_GetError();
    throw std::runtime_error(ss.str());
  }

  return tmp;
}

// -----------------------------------------------------------------------
// SDLSurface::TextureRecord
// -----------------------------------------------------------------------
SDLSurface::TextureRecord::TextureRecord(SDLSurface const* me,
                                         int x,
                                         int y,
                                         int w,
                                         int h,
                                         unsigned int bytes_per_pixel,
                                         int byte_order,
                                         int byte_type)
    : x_(x),
      y_(y),
      w_(w),
      h_(h),
      bytes_per_pixel_(bytes_per_pixel),
      byte_order_(byte_order),
      byte_type_(byte_type) {
  reupload(me, Rect::REC(x, y, w, h));
}

// -----------------------------------------------------------------------

void SDLSurface::TextureRecord::reupload(SDLSurface const* me, Rect dirty) {
  if (gltexture) {
    Rect intersect = Rect::REC(x_, y_, w_, h_).Intersection(dirty);
    auto data = me->Dump(intersect);
    gltexture->Write(
        Rect(Point(intersect.x() - x_, intersect.y() - y_), intersect.size()),
        byte_order_, byte_type_, data.data());
  } else {
    auto data = me->Dump(Rect::REC(x_, y_, w_, h_));
    gltexture = std::make_shared<glTexture>(Size(w_, h_));
    gltexture->Write(Rect(Point(0, 0), Size(w_, h_)), byte_order_, byte_type_,
                     data.data());
  }
}

// -----------------------------------------------------------------------

void SDLSurface::TextureRecord::forceUnload() { gltexture.reset(); }

// -----------------------------------------------------------------------
// SDLSurface
// -----------------------------------------------------------------------

SDLSurface::SDLSurface()
    : surface_(NULL), texture_is_valid_(false), is_mask_(false) {}

// -----------------------------------------------------------------------

// Surface that takes ownership of an externally created surface.
SDLSurface::SDLSurface(SDL_Surface* surf, std::vector<GrpRect> region_table)
    : surface_(surf),
      region_table_(std::move(region_table)),
      texture_is_valid_(false),
      is_mask_(false) {
  if (region_table_.empty())
    buildRegionTable(Size(surf->w, surf->h));
}

// -----------------------------------------------------------------------

SDLSurface::SDLSurface(const Size& size)
    : surface_(NULL), texture_is_valid_(false), is_mask_(false) {
  allocate(size);
  buildRegionTable(size);
}

// -----------------------------------------------------------------------

// Constructor helper function
void SDLSurface::buildRegionTable(const Size& size) {
  // Build a region table with one entry the size of the surface (This
  // should never need to be used with objects created with this
  // constructor, but let's make sure everything is initialized since
  // it'll happen somehow.)
  GrpRect rect;
  rect.rect = Rect(Point(0, 0), size);
  rect.originX = 0;
  rect.originY = 0;
  region_table_.push_back(rect);
}

// -----------------------------------------------------------------------

SDLSurface::~SDLSurface() { deallocate(); }

// -----------------------------------------------------------------------

Size SDLSurface::GetSize() const { return Size(surface_->w, surface_->h); }

Rect SDLSurface::GetRect() const { return Rect(Point(0, 0), GetSize()); }

// -----------------------------------------------------------------------

void SDLSurface::allocate(const Size& size) {
  deallocate();

  surface_ = buildNewSurface(size);

  Fill(RGBAColour::Black());
}

// -----------------------------------------------------------------------

void SDLSurface::deallocate() {
  textures_.clear();
  if (surface_) {
    SDL_FreeSurface(surface_);
    surface_ = NULL;
  }
}

// TODO(erg): This function doesn't ignore alpha blending when use_src_alpha is
// false; thus, grp_open and grp_mask_open are really grp_mask_open.
void SDLSurface::BlitToSurface(Surface& dest_surface,
                               const Rect& src,
                               const Rect& dst,
                               int alpha,
                               bool use_src_alpha) const {
  SDLSurface& sdl_dest_surface = dynamic_cast<SDLSurface&>(dest_surface);

  SDL_Rect src_rect, dest_rect;
  RectToSDLRect(src, &src_rect);
  RectToSDLRect(dst, &dest_rect);

  if (src.size() != dst.size()) {
    // Blit the source rectangle into its own image.
    SDL_Surface* src_image = buildNewSurface(src.size());
    if (pygame_AlphaBlit(surface_, &src_rect, src_image, NULL))
      reportSDLError("SDL_BlitSurface", "SDLGraphicsSystem::blitSurfaceToDC()");

    SDL_Surface* tmp = buildNewSurface(dst.size());
    pygame_stretch(src_image, tmp);

    if (use_src_alpha) {
      if (SDL_SetAlpha(tmp, SDL_SRCALPHA, alpha))
        reportSDLError("SDL_SetAlpha", "SDLGraphicsSystem::blitSurfaceToDC()");
    } else {
      if (SDL_SetAlpha(tmp, 0, 0))
        reportSDLError("SDL_SetAlpha", "SDLGraphicsSystem::blitSurfaceToDC()");
    }

    if (SDL_BlitSurface(tmp, NULL, sdl_dest_surface.surface(), &dest_rect))
      reportSDLError("SDL_BlitSurface", "SDLGraphicsSystem::blitSurfaceToDC()");

    SDL_FreeSurface(tmp);
    SDL_FreeSurface(src_image);
  } else {
    if (use_src_alpha) {
      if (SDL_SetAlpha(surface_, SDL_SRCALPHA, alpha))
        reportSDLError("SDL_SetAlpha", "SDLGraphicsSystem::blitSurfaceToDC()");
    } else {
      if (SDL_SetAlpha(surface_, 0, 0))
        reportSDLError("SDL_SetAlpha", "SDLGraphicsSystem::blitSurfaceToDC()");
    }

    if (SDL_BlitSurface(surface_, &src_rect, sdl_dest_surface.surface(),
                        &dest_rect))
      reportSDLError("SDL_BlitSurface", "SDLGraphicsSystem::blitSurfaceToDC()");
  }
  sdl_dest_surface.markWrittenTo(dst);
}

// -----------------------------------------------------------------------

// Allows for tight coupling with SDL_ttf. Rethink the existence of
// this function later.
void SDLSurface::blitFROMSurface(SDL_Surface* src_surface,
                                 const Rect& src,
                                 const Rect& dst,
                                 int alpha,
                                 bool use_src_alpha) {
  SDL_Rect src_rect, dest_rect;
  RectToSDLRect(src, &src_rect);
  RectToSDLRect(dst, &dest_rect);

  if (use_src_alpha) {
    if (pygame_AlphaBlit(src_surface, &src_rect, surface_, &dest_rect))
      reportSDLError("pygame_AlphaBlit",
                     "SDLGraphicsSystem::blitSurfaceToDC()");
  } else {
    if (SDL_BlitSurface(src_surface, &src_rect, surface_, &dest_rect))
      reportSDLError("SDL_BlitSurface", "SDLGraphicsSystem::blitSurfaceToDC()");
  }

  markWrittenTo(dst);
}

// -----------------------------------------------------------------------

static void determineProperties(SDL_Surface* surface,
                                bool is_mask,
                                GLenum& bytes_per_pixel,
                                GLint& byte_order,
                                GLint& byte_type) {
  SDL_LockSurface(surface);
  {
    bytes_per_pixel = surface->format->BytesPerPixel;
    byte_order = GL_RGBA;
    byte_type = GL_UNSIGNED_BYTE;

    // Determine the byte order of the surface
    SDL_PixelFormat* format = surface->format;
    if (bytes_per_pixel == 4) {
      // If the order is RGBA...
      if (format->Rmask == 0xFF000000 && format->Amask == 0xFF)
        byte_order = GL_RGBA;
      // OSX's crazy ARGB pixel format
      else if ((format->Amask == 0x0 || format->Amask == 0xFF000000) &&
               format->Rmask == 0xFF0000 && format->Gmask == 0xFF00 &&
               format->Bmask == 0xFF) {
        // This is an insane hack to get around OSX's crazy byte order
        // for alpha on PowerPC. Since there isn't a GL_ARGB type, we
        // need to specify BGRA and then tell the byte type to be
        // reversed order.
        //
        // 20070303: Whoah! Is this the internal format on all
        // platforms!?
        byte_order = GL_BGRA;
        byte_type = GL_UNSIGNED_INT_8_8_8_8_REV;
      } else {
        std::ios_base::fmtflags f =
            std::cerr.flags(std::ios::hex | std::ios::uppercase);
        std::cerr << "Unknown mask: (" << format->Rmask << ", " << format->Gmask
                  << ", " << format->Bmask << ", " << format->Amask << ")"
                  << std::endl;
        std::cerr.flags(f);
      }
    } else if (bytes_per_pixel == 3) {
      // For now, just assume RGB.
      byte_order = GL_RGB;
      std::cerr << "Warning: Am I really an RGB Surface? Check"
                << " Texture::Texture()!" << std::endl;
    } else {
      std::ostringstream oss;
      oss << "Error loading texture: bytes_per_pixel == "
          << int(bytes_per_pixel) << " and we only handle 3 or 4.";
      throw std::runtime_error(oss.str());
    }
  }
  SDL_UnlockSurface(surface);

  if (is_mask) {
    // Compile shader for use:
    bytes_per_pixel = GL_ALPHA;
  }
}

// -----------------------------------------------------------------------

void SDLSurface::uploadTextureIfNeeded() const {
  if (!texture_is_valid_) {
    if (textures_.size() == 0) {
      GLenum bytes_per_pixel;
      GLint byte_order, byte_type;
      determineProperties(surface_, is_mask_, bytes_per_pixel, byte_order,
                          byte_type);

      // ---------------------------------------------------------------------

      // Figure out the optimal way of splitting up the image.
      std::vector<int> x_pieces, y_pieces;
      x_pieces = segmentPicture(surface_->w);
      y_pieces = segmentPicture(surface_->h);

      int x_offset = 0;
      for (std::vector<int>::const_iterator it = x_pieces.begin();
           it != x_pieces.end(); ++it) {
        int y_offset = 0;
        for (std::vector<int>::const_iterator jt = y_pieces.begin();
             jt != y_pieces.end(); ++jt) {
          textures_.emplace_back(this, x_offset, y_offset, *it, *jt,
                                 bytes_per_pixel, byte_order, byte_type);

          y_offset += *jt;
        }

        x_offset += *it;
      }
    } else {
      // Reupload the textures without reallocating them.
      for (auto& it : textures_)
        it.reupload(this, dirty_rectangle_);
    }

    dirty_rectangle_ = Rect();
    texture_is_valid_ = true;
  }
}

// -----------------------------------------------------------------------

void SDLSurface::RenderToScreen(const Rect& src_rect,
                                const Rect& dst_rect,
                                int alpha) const {
  int op[] = {alpha, alpha, alpha, alpha};
  this->RenderToScreen(src_rect, dst_rect, op);
}

// -----------------------------------------------------------------------

void SDLSurface::RenderToScreenAsColorMask(const Rect& src,
                                           const Rect& dst,
                                           const RGBAColour& rgba,
                                           int filter) const {
  uploadTextureIfNeeded();

  for (const auto& it : textures_) {
    auto src_rect = src, dst_rect = dst;
    LocalRect coordinate_system(it.x_, it.y_, it.w_, it.h_);
    if (!coordinate_system.intersectAndTransform(src_rect, dst_rect))
      continue;

    if (filter == 0) {  // subtractive color mask
      glRenderer().RenderColormask({it.gltexture, src_rect},
                                   {screen_, dst_rect}, rgba);
    } else {
      RenderingConfig config;
      config.mask_color = rgba;
      glRenderer().Render({it.gltexture, src_rect}, config,
                          {screen_, dst_rect});
    }
  }
}

// -----------------------------------------------------------------------

void SDLSurface::RenderToScreen(const Rect& src_rect,
                                const Rect& dst_rect,
                                const int opacity[4]) const {
  uploadTextureIfNeeded();

  for (const auto& it : textures_) {
    auto src = src_rect, dst = dst_rect;
    LocalRect coordinate_system(it.x_, it.y_, it.w_, it.h_);
    if (coordinate_system.intersectAndTransform(src, dst)) {
      RenderingConfig config;
      config.vertex_alpha =
          std::array<float, 4>{opacity[0] / 255.0f, opacity[1] / 255.0f,
                               opacity[2] / 255.0f, opacity[3] / 255.0f};
      glRenderer().Render({it.gltexture, src}, config, {screen_, dst});
    }
  }
}

// -----------------------------------------------------------------------

void SDLSurface::RenderToScreenAsObject(const GraphicsObject& rp,
                                        const Rect& src,
                                        const Rect& dst,
                                        int alpha) const {
  uploadTextureIfNeeded();

  for (const auto& it : textures_) {
    auto src_rect = src, dst_rect = dst;
    LocalRect coordinate_system(it.x_, it.y_, it.w_, it.h_);
    if (!coordinate_system.intersectAndTransform(src_rect, dst_rect))
      continue;

    RenderingConfig config;
    config.alpha = static_cast<float>(alpha) / 255.0f;

    auto model =
        glm::translate(glm::mat4(1.0f), glm::vec3(dst.x(), dst.y(), 0));
    auto& param = rp.Param();
    float x_rep = dst.width() / 2.0f + param.rep_origin_x();
    float y_rep = dst.height() / 2.0f + param.rep_origin_y();
    model = glm::translate(model, glm::vec3(x_rep, y_rep, 0.0f));
    model = glm::rotate(model, glm::radians(param.rotation() / 10.0f),
                        glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::translate(model, glm::vec3(-x_rep, -y_rep, 0.0f));
    config.model = model;
    config.blend_type = param.composite_mode();
    config.color = param.colour();
    config.tint = param.tint();
    config.mono = param.mono();
    config.invert = param.invert();
    config.light = param.light();

    glRenderer().Render({it.gltexture, src_rect}, std::move(config),
                        {screen_, dst_rect});
  }
}

// -----------------------------------------------------------------------

void SDLSurface::Fill(const RGBAColour& colour) {
  // Fill the entire surface with the incoming colour
  Uint32 sdl_colour = MapRGBA(surface_->format, colour);

  if (SDL_FillRect(surface_, NULL, sdl_colour))
    reportSDLError("SDL_FillRect", "SDLGraphicsSystem::wipe()");

  // If we are the main screen, then we want to update the screen
  markWrittenTo(GetRect());
}

// -----------------------------------------------------------------------

void SDLSurface::Fill(const RGBAColour& colour, const Rect& area) {
  // Fill the entire surface with the incoming colour
  Uint32 sdl_colour = MapRGBA(surface_->format, colour);

  SDL_Rect rect;
  RectToSDLRect(area, &rect);

  if (SDL_FillRect(surface_, &rect, sdl_colour))
    reportSDLError("SDL_FillRect", "SDLGraphicsSystem::wipe()");

  // If we are the main screen, then we want to update the screen
  markWrittenTo(area);
}

// -----------------------------------------------------------------------

void SDLSurface::Invert(const Rect& rect) {
  InvertColourTransformer inverter;
  TransformSurface(this, rect, inverter);
}

// -----------------------------------------------------------------------

void SDLSurface::Mono(const Rect& rect) {
  MonoColourTransformer mono;
  TransformSurface(this, rect, mono);
}

// -----------------------------------------------------------------------

void SDLSurface::ToneCurve(const ToneCurveRGBMap effect, const Rect& area) {
  ToneCurveColourTransformer tc(effect);
  TransformSurface(this, area, tc);
}

// -----------------------------------------------------------------------

void SDLSurface::ApplyColour(const RGBColour& colour, const Rect& area) {
  ApplyColourTransformer apply(colour);
  TransformSurface(this, area, apply);
}

// -----------------------------------------------------------------------

int SDLSurface::GetNumPatterns() const { return region_table_.size(); }

// -----------------------------------------------------------------------

const GrpRect& SDLSurface::GetPattern(int patt_no) const {
  if (patt_no < region_table_.size())
    return region_table_[patt_no];
  else
    return region_table_[0];
}

// -----------------------------------------------------------------------

Surface* SDLSurface::Clone() const {
  SDL_Surface* tmp_surface = SDL_CreateRGBSurface(
      surface_->flags, surface_->w, surface_->h, surface_->format->BitsPerPixel,
      surface_->format->Rmask, surface_->format->Gmask, surface_->format->Bmask,
      surface_->format->Amask);

  // Disable alpha blending because we're copying onto a blank (and
  // blank alpha!) surface
  if (SDL_SetAlpha(surface_, 0, 0))
    reportSDLError("SDL_SetAlpha", "SDLGraphicsSystem::blitSurfaceToDC()");

  if (SDL_BlitSurface(surface_, NULL, tmp_surface, NULL))
    reportSDLError("SDL_BlitSurface", "SDLSurface::clone()");

  return new SDLSurface(tmp_surface, region_table_);
}

// -----------------------------------------------------------------------

std::vector<int> SDLSurface::segmentPicture(int size_remainging) {
  int max_texture_size = GetMaxTextureSize();

  std::vector<int> output;
  while (size_remainging > max_texture_size) {
    output.push_back(max_texture_size);
    size_remainging -= max_texture_size;
  }

  if (IsNPOTSafe()) {
    output.push_back(size_remainging);
  } else {
    while (size_remainging) {
      int ss = SafeSize(size_remainging);
      if (ss > 512) {
        output.push_back(512);
        size_remainging -= 512;
      } else {
        output.push_back(size_remainging);
        size_remainging = 0;
      }
    }
  }

  return output;
}

// -----------------------------------------------------------------------

void SDLSurface::GetDCPixel(const Point& pos, int& r, int& g, int& b) const {
  SDL_Color colour;
  Uint32 col = 0;

  // determine position
  char* p_position = (char*)surface_->pixels;

  // offset by y
  p_position += (surface_->pitch * pos.y());

  // offset by x
  p_position += (surface_->format->BytesPerPixel * pos.x());

  // copy pixel data
  memcpy(&col, p_position, surface_->format->BytesPerPixel);

  // Before someone tries to simplify the following four lines,
  // remember that sizeof(int) != sizeof(Uint8).
  SDL_GetRGB(col, surface_->format, &colour.r, &colour.g, &colour.b);
  r = colour.r;
  g = colour.g;
  b = colour.b;
}

// -----------------------------------------------------------------------

RGBAColour SDLSurface::GetPixel(Point pos) const {
  SDL_Color colour;
  Uint32 col = 0;

  char* p_position = (char*)surface_->pixels;
  p_position += (surface_->pitch * pos.y());
  p_position += (surface_->format->BytesPerPixel * pos.x());

  // Copy pixel data
  memcpy(&col, p_position, surface_->format->BytesPerPixel);

  // Use SDL_GetRGBA to extract RGBA components
  SDL_GetRGBA(col, surface_->format, &colour.r, &colour.g, &colour.b,
              &colour.unused);
  return RGBAColour(colour.r, colour.g, colour.b, colour.unused);
}

// Note: the dumped pixels are upside down
std::vector<char> SDLSurface::Dump(Rect region) const {
  SDL_Surface* surface = this->surface();
  auto x = region.x(), y = region.y();
  auto w = region.width(), h = region.height();

  std::vector<char> buf(surface->format->BytesPerPixel * w * h);
  char* dst = buf.data();
  if (SDL_MUSTLOCK(surface)) {
    if (SDL_LockSurface(surface) != 0) {
      throw std::runtime_error("Failed to lock the SDL_Surface: " +
                               std::string(SDL_GetError()));
    }
  }
  char* src = static_cast<char*>(surface->pixels);
  src += surface->pitch * (y + h - 1);
  int col_offset = surface->format->BytesPerPixel * x;
  int col_size = surface->format->BytesPerPixel * w;
  for (int row = 0; row < h; ++row) {
    std::memcpy(dst, src + col_offset, col_size);
    dst += col_size;
    src -= surface->pitch;
  }
  if (SDL_MUSTLOCK(surface)) {
    SDL_UnlockSurface(surface);
  }
  return buf;
}

// -----------------------------------------------------------------------

std::shared_ptr<Surface> SDLSurface::ClipAsColorMask(const Rect& clip_rect,
                                                     int r,
                                                     int g,
                                                     int b) const {
  const char* function_name = "SDLGraphicsSystem::ClipAsColorMask()";

  // TODO(erg): This needs to be made exception safe and so does the rest
  // of this file.
  SDL_Surface* tmp_surface = SDL_CreateRGBSurface(
      0, surface_->w, surface_->h, 24, 0xFF0000, 0xFF00, 0xFF, 0);

  if (!tmp_surface)
    reportSDLError("SDL_CreateRGBSurface", function_name);

  if (SDL_BlitSurface(surface_, NULL, tmp_surface, NULL))
    reportSDLError("SDL_BlitSurface", function_name);

  Uint32 colour = SDL_MapRGB(tmp_surface->format, r, g, b);
  if (SDL_SetColorKey(tmp_surface, SDL_SRCCOLORKEY, colour))
    reportSDLError("SDL_SetAlpha", function_name);

  // The OpenGL pieces don't know what to do an image formatted to
  // (FF0000, FF00, FF, 0), so convert it to a standard RGBA image
  // (and clip to the desired rectangle)
  SDL_Surface* surface = buildNewSurface(clip_rect.size());
  SDL_Rect srcrect;
  RectToSDLRect(clip_rect, &srcrect);
  if (SDL_BlitSurface(tmp_surface, &srcrect, surface, NULL))
    reportSDLError("SDL_BlitSurface", function_name);

  SDL_FreeSurface(tmp_surface);

  return std::make_shared<SDLSurface>(surface);
}

// -----------------------------------------------------------------------

void SDLSurface::markWrittenTo(const Rect& written_rect) {
  // Mark that the texture needs reuploading
  dirty_rectangle_ = dirty_rectangle_.Union(written_rect);
  texture_is_valid_ = false;
}

std::vector<SDLSurface::TextureRecord> SDLSurface::GetTextureArray() const {
  return textures_;
}
