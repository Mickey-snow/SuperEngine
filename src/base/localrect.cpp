// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
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

#include "base/localrect.hpp"

#include <algorithm>
#include <cmath>

LocalRect::LocalRect(Rect rec)
    : offsetX_(rec.x()),
      offsetY_(rec.y()),
      width_(rec.width()),
      height_(rec.height()) {}

LocalRect::LocalRect(int offsetX, int offsetY, int width, int height)
    : offsetX_(offsetX), offsetY_(offsetY), width_(width), height_(height) {}

bool LocalRect::intersectAndTransform(Rect& src, Rect& dst) const {
  int srcX1 = src.x();
  int srcX2 = src.x2();
  int srcY1 = src.y();
  int srcY2 = src.y2();

  int dstX1 = dst.x();
  int dstX2 = dst.x2();
  int dstY1 = dst.y();
  int dstY2 = dst.y2();

  bool result = intersectAndTransform(srcX1, srcY1, srcX2, srcY2, dstX1, dstY1,
                                      dstX2, dstY2);
  if (result) {
    src = Rect(Point(srcX1, srcY1), Point(srcX2, srcY2));
    dst = Rect(Point(dstX1, dstY1), Point(dstX2, dstY2));
  }
  return result;
}

bool LocalRect::intersectAndTransform(int& srcX1,
                                      int& srcY1,
                                      int& srcX2,
                                      int& srcY2,
                                      int& dstX1,
                                      int& dstY1,
                                      int& dstX2,
                                      int& dstY2) const {
  // Calculate the size of the incoming source rectangle
  int srcWidth = srcX2 - srcX1;
  int srcHeight = srcY2 - srcY1;

  // Check for intersection with local bounding rectangle
  // local bounding rect = [offsetX_, offsetY_, offsetX_+width_,
  // offsetY_+height_]
  if (srcX1 + srcWidth <= offsetX_ || srcX1 >= offsetX_ + width_ ||
      srcY1 + srcHeight <= offsetY_ || srcY1 >= offsetY_ + height_) {
    // No intersection
    return false;
  }

  // Compute intersection in world space
  int intersectX1 = std::max(srcX1, offsetX_);
  int intersectY1 = std::max(srcY1, offsetY_);
  int intersectX2 = std::min(srcX1 + srcWidth, offsetX_ + width_);
  int intersectY2 = std::min(srcY1 + srcHeight, offsetY_ + height_);

  int intersectWidth = intersectX2 - intersectX1;
  int intersectHeight = intersectY2 - intersectY1;

  // Calculate the destination rectangle sizes
  int dstWidth = dstX2 - dstX1;
  int dstHeight = dstY2 - dstY1;

  // Compute how far into the source rect we shifted by intersection
  float offsetFactorX1 = static_cast<float>(intersectX1 - srcX1) / srcWidth;
  float offsetFactorY1 = static_cast<float>(intersectY1 - srcY1) / srcHeight;

  // Compute how much of the source’s width/height we kept
  float keepFactorX = static_cast<float>(intersectWidth) / srcWidth;
  float keepFactorY = static_cast<float>(intersectHeight) / srcHeight;

  // Update destination rect with these offsets
  int newDstX1 = std::lround(dstX1 + dstWidth * offsetFactorX1);
  int newDstY1 = std::lround(dstY1 + dstHeight * offsetFactorY1);
  int newDstX2 = std::lround(newDstX1 + dstWidth * keepFactorX);
  int newDstY2 = std::lround(newDstY1 + dstHeight * keepFactorY);

  // Apply the updated intersection back into the source coordinates,
  // translating into local space
  srcX1 = intersectX1 - offsetX_;
  srcY1 = intersectY1 - offsetY_;
  srcX2 = srcX1 + intersectWidth;
  srcY2 = srcY1 + intersectHeight;

  // Write back the newly computed destination coordinates
  dstX1 = newDstX1;
  dstY1 = newDstY1;
  dstX2 = newDstX2;
  dstY2 = newDstY2;

  return true;
}
