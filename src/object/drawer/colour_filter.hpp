// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2011 Elliot Glaysher
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

#include <memory>

#include "base/rect.hpp"
#include "machine/rlmachine.hpp"
#include "machine/serialization.hpp"
#include "object/objdrawer.hpp"

class GraphicsObject;
class GraphicsSystem;

class ColourFilterObjectData : public GraphicsObjectData {
 public:
  ColourFilterObjectData(GraphicsSystem& system, const Rect& screen_rect);
  virtual ~ColourFilterObjectData();

  // load_construct_data helper. Wish I could make this private.
  explicit ColourFilterObjectData(System& system);

  void set_rect(const Rect& screen_rect) { screen_rect_ = screen_rect; }

  // Overridden from GraphicsObjectData:
  virtual void Render(const GraphicsObject& go,
                      const GraphicsObject* parent) override;
  virtual int PixelWidth(const GraphicsObject& rendering_properties) override;
  virtual int PixelHeight(const GraphicsObject& rendering_properties) override;
  virtual std::unique_ptr<GraphicsObjectData> Clone() const override;
  virtual void Execute(RLMachine& machine) override;

 protected:
  virtual std::shared_ptr<const Surface> CurrentSurface(
      const GraphicsObject& rp) override;

 private:
  GraphicsSystem& graphics_system_;

  Rect screen_rect_;

  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, const unsigned int file_version);
};

// We need help creating ColourFilterObjectData s since they don't have a
// default constructor:
namespace boost {
namespace serialization {
template <class Archive>
inline void load_construct_data(Archive& ar,
                                ColourFilterObjectData* t,
                                const unsigned int file_version) {
  ::new (t)
      ColourFilterObjectData(Serialization::g_current_machine->GetSystem());
}
}  // namespace serialization
}  // namespace boost
