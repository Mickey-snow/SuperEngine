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

#pragma once

#include <boost/serialization/access.hpp>

#include <iosfwd>

#include "object/objdrawer.hpp"
#include "utilities/lazy_array.hpp"

class GraphicsObject;

/**
 * @brief Parent Parameter Influence on Children
 *
 * The following parent parameters always affect children:
 * - **Visibility:** `IsVisible`
 * - **Opacity:** `AlphaSource`, `AdjustmentAlphas`
 * - **Coordinates and Positioning:**
 *   - `PositionX`, `PositionY`
 *   - `AdjustmentOffsetsX`, `AdjustmentOffsetsY`
 *   - `AdjustmentVertical`
 * - **Clipping Regions:**
 *   - `ClippingRegion`
 *   - `OwnSpaceClippingRegion`
 *
 * @note
 * If a child's parameter is neutral or not set, it inherits the following
 * parent parameters:
 * - **Composite Mode:** `CompositeMode` (Normal, Additive, Subtractive)
 * - **Monochrome Transform:** `MonochromeTransform`
 * - **Invert Transform:** `InvertTransform`
 * - **Tint and Blend Colors:**
 *   - `TintColour`
 *   - `BlendColour`
 * - **Brightness:** `LightLevel`
 */

/**
 * @brief Parameters That Do Not Affect Children
 *
 * The following parameters do not affect children:
 * - **Pattern Number:** `PatternNumber`
 * - **Transformations:**
 *   - **Origin Points:** `OriginX`, `OriginY`
 *   - **Repetition Origins:** `RepetitionOriginX`, `RepetitionOriginY`
 *   - **Scaling:** `WidthPercent`, `HeightPercent`
 *   - **Rotation:** `RotationDiv10`
 * - **Others:** Display order (`ZOrder`, `ZLayer`, `ZDepth`), etc.
 */

/**
 * @brief Capabilities Children Do Not Have
 *
 * - **Gameexe.ini Object Settings:** Children cannot have their own object
 * settings (e.g., level, object ON/OFF, time control mode). They inherit from
 * the parent.
 *
 * - **Display Order Control Between Objects:** The parent controls display
 * order with other objects, while children follow the parent's display order.
 *   - Children have `ZOrder` and `ZLayer` properties to control display order
 * among themselves.
 *
 * - **Automatic Wipe Copying:** Children cannot have their own wipe copying
 * behavior (`WipeCopy`). Setting it on the parent applies to all children.
 *
 * - **Wipe Disappearance:** Children cannot disappear independently in the next
 * wipe. Setting it on the parent affects all children.
 *
 * - **Object Copy Between Different Objects:** Children cannot use commands
 * like `OBJCOPY` or `OBJFRONTCOPYFRONT`. However, copying between children of
 * the same object is possible using `OBJFRONTCHILDCOPY`.
 *
 * - **Unsupported Object Types:** Environment objects, Bust shots created using
 * `BustShotEditor.exe`, and old animations are unsupported. Children also
 * cannot create their own children.
 */

// A GraphicsObjectData implementation which owns a full set of graphics
// objects which inherit some of its parent properties.
class ParentGraphicsObjectData : public GraphicsObjectData {
 public:
  explicit ParentGraphicsObjectData(int size);
  virtual ~ParentGraphicsObjectData();

  GraphicsObject& GetObject(int obj_number);
  void SetObject(int obj_number, GraphicsObject&& object);

  LazyArray<GraphicsObject>& objects();

  virtual void Render(const GraphicsObject& go,
                      const GraphicsObject* parent) override;
  virtual int PixelWidth(const GraphicsObject& rendering_properties) override;
  virtual int PixelHeight(const GraphicsObject& rendering_properties) override;
  virtual std::unique_ptr<GraphicsObjectData> Clone() const override;
  virtual void Execute(RLMachine& machine) override;

  virtual bool IsParentLayer() const override { return true; }

 protected:
  virtual std::shared_ptr<const Surface> CurrentSurface(
      const GraphicsObject& rp) override;

 private:
  ParentGraphicsObjectData();

  LazyArray<GraphicsObject> objects_;

  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, const unsigned int file_version);
};
