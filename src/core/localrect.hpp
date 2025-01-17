// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2025 Serina Sakurai
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

#include "core/rect.hpp"

/**
 * @class LocalRect
 * @brief Stores and operates on a "local" coordinate space with an offset and
 * dimensions.
 *
 * The LocalRect class provides methods to intersect and transform rectangles
 * from a world coordinate system into the local coordinate system defined by
 * this class.
 */
class LocalRect {
 public:
  LocalRect(Rect rec);
  LocalRect(int offsetX, int offsetY, int width, int height);

  /**
   * @brief Intersects the given source rectangle with this LocalRect
   *        and transforms both source and destination rectangles accordingly.
   *
   * - If an intersection is found, both \p src and \p dst are modified:
   *   - \p src is updated to coordinates local to this LocalRect.
   *   - \p dst is updated to reflect the intersected portion in the destination
   * space.
   * - If there is no intersection, returns false and no modification is made.
   *
   * @param src [in,out] The source rectangle in world coordinates.
   * @param dst [in,out] The destination rectangle in (usually) world
   * coordinates.
   * @return True if intersection occurred and the rectangles were updated;
   * false otherwise.
   */
  bool intersectAndTransform(Rect& src, Rect& dst) const;

  /**
   * @brief Intersects the given source rectangle with this LocalRect
   *        and transforms both source and destination coordinates accordingly.
   *
   * - If an intersection is found, the parameters \p srcX1, \p srcY1, etc.
   *   are updated:
   *   - \p srcX1, \p srcY1, \p srcX2, \p srcY2 become local to this LocalRect.
   *   - \p dstX1, \p dstY1, \p dstX2, \p dstY2 become the transformed portion
   *     of the destination space.
   * - If there is no intersection, returns false and the parameters remain
   * unchanged.
   *
   * @param srcX1 [in,out] Source top-left x
   * @param srcY1 [in,out] Source top-left y
   * @param srcX2 [in,out] Source bottom-right x
   * @param srcY2 [in,out] Source bottom-right y
   * @param dstX1 [in,out] Destination top-left x
   * @param dstY1 [in,out] Destination top-left y
   * @param dstX2 [in,out] Destination bottom-right x
   * @param dstY2 [in,out] Destination bottom-right y
   * @return True if intersection occurred and the coordinates were updated;
   * false otherwise.
   */
  bool intersectAndTransform(int& srcX1,
                             int& srcY1,
                             int& srcX2,
                             int& srcY2,
                             int& dstX1,
                             int& dstY1,
                             int& dstX2,
                             int& dstY2) const;

 private:
  int offsetX_;  ///< X-offset (left) of the local coordinate space.
  int offsetY_;  ///< Y-offset (top) of the local coordinate space.
  int width_;    ///< Width of the local coordinate space.
  int height_;   ///< Height of the local coordinate space.
};
