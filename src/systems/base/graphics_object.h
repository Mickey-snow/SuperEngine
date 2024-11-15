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

#include <boost/serialization/access.hpp>
#include <boost/serialization/version.hpp>

#include <iostream>
#include <string>
#include <vector>

#include "base/rect.h"
#include "object/parameter_manager.h"
#include "systems/base/colour.h"

class RLMachine;
class GraphicsObjectSlot;
class GraphicsObjectData;
class ObjectMutator;

// Describes an graphical object on the screen.
class GraphicsObject {
 public:
  GraphicsObject();
  ~GraphicsObject();

  // Use this->Clone() instead.
  GraphicsObject(const GraphicsObject& obj) = delete;
  GraphicsObject& operator=(const GraphicsObject& obj) = delete;

  GraphicsObject(GraphicsObject&& rhs);
  GraphicsObject& operator=(GraphicsObject&& rhs);

  GraphicsObject Clone() const;

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

 private:
  // Class to manage the actual implementation data
  ParameterManager param_;

  // The actual data used to render the object
  std::unique_ptr<GraphicsObjectData> object_data_;

  // Tasks that run every tick. Used to mutate object parameters over time (and
  // how we check from a blocking LongOperation if the mutation is ongoing).
  //
  // I think R23 mentioned that these were called "Parameter Events" in the
  // RLMAX SDK.
  std::vector<std::unique_ptr<ObjectMutator>> object_mutators_;

  // boost::serialization support
  friend class boost::serialization::access;

  template <class Archive>
  void serialize(Archive& ar, unsigned int version) {
    if (version < 1)
      throw std::runtime_error(
          "GraphicsObject: serialization for version<1 is unsupported.");

    std::cerr << "Currently under construction." << std::endl;
  }
};

BOOST_CLASS_VERSION(GraphicsObject, 1)

enum { OBJ_FG = 0, OBJ_BG = 1 };
