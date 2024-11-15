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

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>

#include "object/drawer/parent.hpp"

#include "systems/base/graphics_object.hpp"
#include "utilities/exception.hpp"

// -----------------------------------------------------------------------
// ParentGraphicsObjectData
// -----------------------------------------------------------------------
ParentGraphicsObjectData::ParentGraphicsObjectData(int size) : objects_(size) {}

ParentGraphicsObjectData::~ParentGraphicsObjectData() {}

GraphicsObject& ParentGraphicsObjectData::GetObject(int obj_number) {
  return objects_[obj_number];
}

void ParentGraphicsObjectData::SetObject(int obj_number,
                                         GraphicsObject&& object) {
  objects_[obj_number] = std::move(object);
}

LazyArray<GraphicsObject>& ParentGraphicsObjectData::objects() {
  return objects_;
}

void ParentGraphicsObjectData::Render(const GraphicsObject& go,
                                      const GraphicsObject* parent) {
  AllocatedLazyArrayIterator<GraphicsObject> it = objects_.begin();
  AllocatedLazyArrayIterator<GraphicsObject> end = objects_.end();
  for (; it != end; ++it) {
    it->Render(it.pos(), &go);
  }
}

int ParentGraphicsObjectData::PixelWidth(const GraphicsObject&) {
  throw rlvm::Exception("There is no sane value for this!");
}

int ParentGraphicsObjectData::PixelHeight(const GraphicsObject&) {
  throw rlvm::Exception("There is no sane value for this!");
}

std::unique_ptr<GraphicsObjectData> ParentGraphicsObjectData::Clone() const {
  int size = objects_.Size();
  auto cloned = std::make_unique<ParentGraphicsObjectData>(size);
  for (int i = 0; i < size; ++i) {
    if (!objects_.Exists(i))
      continue;
    cloned->objects_[i] = objects_.At(i)->Clone();
  }

  return cloned;
}

void ParentGraphicsObjectData::Execute(RLMachine& machine) {
  for (GraphicsObject& obj : objects_)
    obj.Execute(machine);
}

std::shared_ptr<const Surface> ParentGraphicsObjectData::CurrentSurface(
    const GraphicsObject& rp) {
  return std::shared_ptr<const Surface>();
}

ParentGraphicsObjectData::ParentGraphicsObjectData() : objects_(0) {}

template <class Archive>
void ParentGraphicsObjectData::serialize(Archive& ar, unsigned int version) {
  ar& boost::serialization::base_object<GraphicsObjectData>(*this);
  ar & objects_;
}

// -----------------------------------------------------------------------

// Explicit instantiations for text archives (since we hide the
// implementation)

template void ParentGraphicsObjectData::serialize<
    boost::archive::text_iarchive>(boost::archive::text_iarchive& ar,
                                   unsigned int version);
template void ParentGraphicsObjectData::serialize<
    boost::archive::text_oarchive>(boost::archive::text_oarchive& ar,
                                   unsigned int version);

BOOST_CLASS_EXPORT(ParentGraphicsObjectData);
