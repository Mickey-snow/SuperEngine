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

#ifndef __ZoomLongOperation_hpp__
#define __ZoomLongOperation_hpp__

#include <boost/shared_ptr.hpp>

#include "MachineBase/LongOperation.hpp"
#include "Systems/Base/Rect.hpp"

class RLMachine;
class Surface;

class ZoomLongOperation : public LongOperation
{
private:
  RLMachine& m_machine;

  boost::shared_ptr<Surface> m_origSurface;
  boost::shared_ptr<Surface> m_srcSurface;

  const Rect m_frect;
  const Rect m_trect;
  const Rect m_drect;
  const unsigned int m_duration;

  unsigned int m_startTime;

public:
  ZoomLongOperation(
    RLMachine& machine,
    const boost::shared_ptr<Surface>& m_origSurface,
    const boost::shared_ptr<Surface>& m_srcSurface,
    const Rect& m_frect, const Rect& m_trect, const Rect& m_drect,
    const int time);
  ~ZoomLongOperation();

  virtual bool operator()(RLMachine& machine);
};

#endif
