// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2007 Elliot Glaysher
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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
// -----------------------------------------------------------------------

#ifndef __GraphicsTextObject_hpp__
#define __GraphicsTextObject_hpp__

#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/serialization/split_member.hpp>

#include "Systems/Base/GraphicsObjectData.hpp"

class GraphicsObject;
class Surface;

/**
 * Represents the textual data made with commands such as objOfText,
 * objSetText, objTextOpts, et cetera.
 */
class GraphicsTextObject : public GraphicsObjectData
{
private:
  std::string m_cachedUtf8str;

  boost::shared_ptr<Surface> m_surface;

  /**
   * Redraw m_surface.
   */
  void updateSurface(RLMachine& machine,
					 const GraphicsObject& rp);

  bool needsUpdate(const GraphicsObject& renderingProperties);

public:
  GraphicsTextObject();

  GraphicsTextObject(RLMachine& machine);
  ~GraphicsTextObject();

  // ------------------------------------ [ GraphicsObjectData interface ]
  virtual void render(RLMachine& machine,
                      const GraphicsObject& renderingProperties);

  virtual int pixelWidth(RLMachine& machine,
                         const GraphicsObject& renderingProperties);
  virtual int pixelHeight(RLMachine& machine,
                          const GraphicsObject& renderingProperties);

  virtual GraphicsObjectData* clone() const;

  template<class Archive>
  void save(Archive & ar, const unsigned int file_version) const;

  template<class Archive>
  void load(Archive& ar, const unsigned int file_version);

  BOOST_SERIALIZATION_SPLIT_MEMBER()
};

#endif
