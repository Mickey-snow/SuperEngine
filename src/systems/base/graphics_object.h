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

#ifndef SRC_SYSTEMS_BASE_GRAPHICS_OBJECT_H_
#define SRC_SYSTEMS_BASE_GRAPHICS_OBJECT_H_

#include <boost/scoped_ptr.hpp>
#include <boost/serialization/access.hpp>
#include <boost/serialization/version.hpp>
#include <boost/shared_ptr.hpp>

#include <string>
#include <vector>

#include "base/rect.h"
#include "object/parameter_manager.h"
#include "systems/base/colour.h"

class RLMachine;
class GraphicsObject;
class GraphicsObjectSlot;
class GraphicsObjectData;
class ObjectMutator;

// Describes an independent, movable graphical object on the
// screen. GraphicsObject, internally, references a copy-on-write
// datastructure, which in turn has optional components to save
// memory.
class GraphicsObject {
 public:
  GraphicsObject();
  GraphicsObject(const GraphicsObject& obj);
  ~GraphicsObject();
  GraphicsObject& operator=(const GraphicsObject& obj);

  ParameterManager& Param() { return param_; }
  ParameterManager const& Param() const { return param_; }

  int PixelWidth() const;
  int PixelHeight() const;

  bool has_object_data() const { return object_data_.get(); }

  GraphicsObjectData& GetObjectData();
  void SetObjectData(GraphicsObjectData* obj);

  // Render!
  void Render(int objNum, const GraphicsObject* parent);

  // Frees the object data. Corresponds to objFree, but is also invoked by
  // other commands.
  void FreeObjectData();

  // Resets/reinitializes all the object parameters without deleting the loaded
  // graphics object data.
  void InitializeParams();

  // Both frees the object data and initializes parameters.
  void FreeDataAndInitializeParams();

  // Called each pass through the gameloop to see if this object needs
  // to force a redraw, or something.
  void Execute(RLMachine& machine);

  // Adds a mutator to the list of active mutators. GraphicsSystem takes
  // ownership of the passed in object.
  void AddObjectMutator(std::unique_ptr<ObjectMutator> mutator);

  // Returns true if a mutator matching the following parameters is currently
  // running.
  bool IsMutatorRunningMatching(int repno, const std::string& name);

  // Ends all mutators that match the given parameters.
  void EndObjectMutatorMatching(RLMachine& machine,
                                int repno,
                                const std::string& name,
                                int speedup);

  // Returns a string for each mutator.
  std::vector<std::string> GetMutatorNames() const;

  // Returns the number of GraphicsObject instances sharing the
  // internal copy-on-write object. Only used in unit testing.
  int32_t reference_count() const { return impl_.use_count(); }

  // Whether we have the default shared data. Only used in unit testing.
  bool is_cleared() const { return impl_ == s_empty_impl; }

 private:
  // Makes the internal copy for our copy-on-write semantics. This function
  // checks to see if our Impl object has only one reference to it. If it
  // doesn't, a local copy is made.
  void MakeImplUnique();

  // Immediately delete all mutators; doesn't run their SetToEnd() method.
  void DeleteObjectMutators();

  // Implementation data structure. GraphicsObject::Impl is the internal data
  // store for GraphicsObjects' copy-on-write semantics.
  struct Impl {
    // Visibility. Different from whether an object is in the bg or fg layer
    bool visible_;

    // The positional coordinates of the object
    int x_, y_;

    // Eight additional parameters that are added to x and y during
    // rendering.
    std::array<int, 8> adjust_x_, adjust_y_;

    // Whatever obj_adjust_vert operates on; what's this used for?
    int whatever_adjust_vert_operates_on_;

    // The origin
    int origin_x_, origin_y_;

    // "Rep" origin. This second origin is added to the normal origin
    // only in cases of rotating and scaling.
    int rep_origin_x_, rep_origin_y_;

    // The size of the object, given in integer percentages of [0,
    // 100]. Used for scaling.
    int width_, height_;

    // A second scaling factor, given between [0, 1000].
    int hq_width_, hq_height_;

    // The rotation degree / 10
    int rotation_;

    // Object attributes.

    // The region ("pattern") in g00 bitmaps
    int patt_no_;

    // The source alpha for this image
    int alpha_;

    // Eight additional alphas that are averaged during rendering.
    std::array<int, 8> adjust_alpha_;

    // The clipping region for this image
    Rect clip_;

    // A second clipping region in the object's own space.
    Rect own_clip_;

    // The monochrome transformation
    int mono_;

    // The invert transformation
    int invert_;

    int light_;

    RGBColour tint_;

    // Applies a colour to the object by blending it directly at the
    // alpha components opacity.
    RGBAColour colour_;

    int composite_mode_;

    int scroll_rate_x_, scroll_rate_y_;

    // Three deep zordering.
    int z_order_, z_layer_, z_depth_;

    TextProperties text_properties_;

    DriftProperties drift_properties_;

    DigitProperties digit_properties_;

    ButtonProperties button_properties_;

    // The wipe_copy bit
    int wipe_copy_;

    friend class boost::serialization::access;
    // boost::serialization support
    template <class Archive>
    void serialize(Archive& ar, unsigned int version);
  };
  // Default empty GraphicsObject::Impl. This variable is allocated
  // once, and then is used as the initial value of impl_, where it
  // is cloned on write.
  static const boost::shared_ptr<GraphicsObject::Impl> s_empty_impl;
  // Our actual implementation data
  boost::shared_ptr<GraphicsObject::Impl> impl_;

  // Class to manage the actual implementation data
  ParameterManager param_;

  // The actual data used to render the object
  boost::scoped_ptr<GraphicsObjectData> object_data_;

  // Tasks that run every tick. Used to mutate object parameters over time (and
  // how we check from a blocking LongOperation if the mutation is ongoing).
  //
  // I think R23 mentioned that these were called "Parameter Events" in the
  // RLMAX SDK.
  std::vector<std::unique_ptr<ObjectMutator>> object_mutators_;

  friend class boost::serialization::access;

  // boost::serialization support
  template <class Archive>
  void serialize(Archive& ar, unsigned int version);
};

BOOST_CLASS_VERSION(GraphicsObject::Impl, 7)

enum { OBJ_FG = 0, OBJ_BG = 1 };

#endif
