// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2006 El Riot
//  
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
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

#ifndef __SDLTextWindow_hpp__
#define __SDLTextWindow_hpp__

#include "Systems/Base/TextWindow.hpp"

#include <string>
#include <boost/shared_ptr.hpp>
#include <SDL/SDL_ttf.h>

class SDLSurface;

class SDLTextWindow : public TextWindow
{
private:
  /// 
  std::string m_currentValue;

  /// Converted surface for uploading.
  boost::shared_ptr<SDLSurface> m_surface;

  /// Font being used.
  TTF_Font* m_font;

public:
  SDLTextWindow(RLMachine& machine, int window);
  ~SDLTextWindow();

  void setCurrentText(RLMachine& machine, const std::string& tex);

  virtual void render(RLMachine& machine);
  virtual void displayText(RLMachine& machine, const std::string& text);
};


#endif
