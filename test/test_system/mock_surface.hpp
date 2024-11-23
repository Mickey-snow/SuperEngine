// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
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
// -----------------------------------------------------------------------

#include <gmock/gmock.h>

#include "systems/base/surface.hpp"

class MockedSurface : public Surface {
 public:
  MockedSurface() = default;
  ~MockedSurface() override = default;

  MOCK_METHOD(void, EnsureUploaded, (), (const, override));
  MOCK_METHOD(void, Fill, (const RGBAColour& colour), (override));
  MOCK_METHOD(void,
              Fill,
              (const RGBAColour& colour, const Rect& area),
              (override));
  MOCK_METHOD(void,
              ToneCurve,
              (const ToneCurveRGBMap effect, const Rect& area),
              (override));
  MOCK_METHOD(void, Invert, (const Rect& area), (override));
  MOCK_METHOD(void, Mono, (const Rect& area), (override));
  MOCK_METHOD(void,
              ApplyColour,
              (const RGBColour& colour, const Rect& area),
              (override));
  MOCK_METHOD(void, SetIsMask, (bool is), (override));
  MOCK_METHOD(Size, GetSize, (), (const, override));
  MOCK_METHOD(void, Dump, (), (override));
  MOCK_METHOD(void,
              BlitToSurface,
              (Surface & dest_surface,
               const Rect& src,
               const Rect& dst,
               int alpha,
               bool use_src_alpha),
              (const, override));
  MOCK_METHOD(void,
              RenderToScreen,
              (const Rect& src, const Rect& dst, int alpha),
              (const, override));
  MOCK_METHOD(void,
              RenderToScreen,
              (const Rect& src, const Rect& dst, const int opacity[4]),
              (const, override));
  MOCK_METHOD(
      void,
      RenderToScreenAsColorMask,
      (const Rect& src, const Rect& dst, const RGBAColour& colour, int filter),
      (const, override));
  MOCK_METHOD(
      void,
      RenderToScreenAsObject,
      (const GraphicsObject& rp, const Rect& src, const Rect& dst, int alpha),
      (const, override));
  MOCK_METHOD(int, GetNumPatterns, (), (const, override));
  MOCK_METHOD(const GrpRect&, GetPattern, (int patt_no), (const, override));
  MOCK_METHOD(void,
              GetDCPixel,
              (const Point& pos, int& r, int& g, int& b),
              (const, override));
  MOCK_METHOD(std::shared_ptr<Surface>,
              ClipAsColorMask,
              (const Rect& clip_rect, int r, int g, int b),
              (const, override));
  MOCK_METHOD(Surface*, Clone, (), (const, override));
};
