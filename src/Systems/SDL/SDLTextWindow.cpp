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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
// -----------------------------------------------------------------------

#include "Precompiled.hpp"

// -----------------------------------------------------------------------

/**
 * @file   SDLTextWindow.cpp
 * @author Elliot Glaysher
 * @date   Wed Mar  7 22:11:17 2007
 *
 * @brief
 */

#include "Systems/Base/System.hpp"
#include "Systems/Base/SystemError.hpp"
#include "Systems/Base/GraphicsSystem.hpp"
#include "Systems/SDL/SDLTextSystem.hpp"
#include "Systems/Base/TextWindowButton.hpp"
#include "Systems/Base/SelectionElement.hpp"
#include "Systems/SDL/SDLTextWindow.hpp"
#include "Systems/SDL/SDLSurface.hpp"
#include "Systems/SDL/SDLUtils.hpp"

#include "MachineBase/RLMachine.hpp"
#include "libReallive/gameexe.h"

#include <SDL/SDL_opengl.h>
#include "SDL_ttf.h"

#include <boost/function.hpp>
#include <boost/bind.hpp>
#include "Utilities.h"

#include "utf8.h"
#include "Modules/cp932toUnicode.hpp"
#include "Modules/TextoutLongOperation.hpp"

#include <iostream>
#include <vector>

using namespace std;
using namespace boost;

// -----------------------------------------------------------------------

SDLTextWindow::SDLTextWindow(RLMachine& machine, int windowNum)
  : TextWindow(machine, windowNum), m_rubyBeginPoint(-1)
{
  Gameexe& gexe = machine.system().gameexe();
  GameexeInterpretObject window(gexe("WINDOW", windowNum));
  setWindowWaku(machine, gexe, window("WAKU_SETNO"));

  SDLTextSystem& text = dynamic_cast<SDLTextSystem&>(machine.system().text());
  m_font = text.getFontOfSize(machine, fontSizeInPixels());
  m_rubyFont = text.getFontOfSize(machine, rubyTextSize());

  clearWin();
}

// -----------------------------------------------------------------------

SDLTextWindow::~SDLTextWindow()
{
}

// -----------------------------------------------------------------------

void SDLTextWindow::setMousePosition(RLMachine& machine, const Point& pos)
{
  if(inSelectionMode())
  {
    for_each(m_selections.begin(), m_selections.end(),
             bind(&SelectionElement::setMousePosition, _1,
                  ref(machine), pos));
  }

  TextWindow::setMousePosition(machine, pos);
}

// -----------------------------------------------------------------------

bool SDLTextWindow::handleMouseClick(RLMachine& machine, const Point& pos,
                                     bool pressed)
{
  if(inSelectionMode())
  {
    bool found =
      find_if(m_selections.begin(), m_selections.end(),
              bind(&SelectionElement::handleMouseClick, _1,
                   ref(machine), pos, pressed))
      != m_selections.end();

    if(found)
      return true;
  }

  return TextWindow::handleMouseClick(machine, pos, pressed);
}

// -----------------------------------------------------------------------

void SDLTextWindow::clearWin()
{
  m_insertionPointX = 0;
  m_insertionPointY = rubyTextSize();
  m_currentIndentationInPixels = 0;
  m_currentLineNumber = 0;

  m_rubyBeginPoint = -1;

  // Reset the color
  // COLOUR
  m_fontColour = m_defaultColor;

  // Allocate the text window surface
  m_surface.reset(new SDLSurface(textWindowSize()));
  m_surface->fill(RGBAColour::Clear());
}

// -----------------------------------------------------------------------

bool SDLTextWindow::displayChar(RLMachine& machine,
                                const std::string& current,
                                const std::string& next)
{
  // If this text page is already full, save some time and reject
  // early.
  if(isFull())
    return false;

  setVisible(true);

  if(current != "")
  {
    SDL_Color color;
    RGBColourToSDLColor(m_fontColour, &color);
    int curCodepoint = codepoint(current);
    int nextCodepoint = codepoint(next);

    // U+3010 (LEFT BLACK LENTICULAR BRACKET) and U+3011 (RIGHT BLACK
    // LENTICULAR BRACKET) should be handled before this
    // function. Otherwise, it's an error.
    if(curCodepoint == 0x3010 || curCodepoint == 0x3011)
    {
      throw SystemError(
        "Bug in parser; \\{name} construct should be handled before displayChar");
    }

    SDL_Surface* tmp =
      TTF_RenderUTF8_Blended(m_font.get(), current.c_str(), color);

    // If the width of this glyph plus the spacing will put us over the
    // edge of the window, then line increment.
    //
    // If the current character will fit on this line, and it is NOT
    // in this set, then we should additionally check the next
    // character.  If that IS in this set and will not fit on the
    // current line, then we break the line before the current
    // character instead, to prevent the next character being stranded
    // at the start of a line.
    //
    bool charWillFitOnLine = m_insertionPointX + tmp->w + m_xSpacing <=
      textWindowSize().width();
    bool nextCharWillFitOnLine = m_insertionPointX + 2*(tmp->w + m_xSpacing) <=
      textWindowSize().width();
    if(!charWillFitOnLine ||
       (charWillFitOnLine && !isKinsoku(curCodepoint) &&
        !nextCharWillFitOnLine && isKinsoku(nextCodepoint)))
    {
      hardBrake();

      if(isFull())
        return false;
    }

    // Render glyph to surface
    Size s(tmp->w,tmp->h);
    m_surface->blitFROMSurface(
      tmp,
      Rect(Point(0, 0), s),
      Rect(Point(m_insertionPointX, m_insertionPointY), s),
      255);

    // Move the insertion point forward one character
    m_insertionPointX += m_fontSizeInPixels + m_xSpacing;

    SDL_FreeSurface(tmp);
  }

  // When we aren't rendering a piece of text with a ruby gloss, mark
  // the screen as dirty so that this character renders.
  if(m_rubyBeginPoint == -1)
  {
    machine.system().graphics().markScreenAsDirty(GUT_TEXTSYS);
  }

  return true;
}

// -----------------------------------------------------------------------

bool SDLTextWindow::isFull() const
{
  return m_currentLineNumber >= m_yWindowSizeInChars;
}

// -----------------------------------------------------------------------

void SDLTextWindow::setIndentation()
{
  m_currentIndentationInPixels = m_insertionPointX;
}

// -----------------------------------------------------------------------

void SDLTextWindow::setName(RLMachine& machine, const std::string& utf8name,
                            const std::string& nextChar)
{
  if(m_nameMod == 0)
  {
    // Display the name in one pass
    printTextToFunction(bind(&SDLTextWindow::displayChar, ref(*this),
                             ref(machine), _1, _2),
                        utf8name, nextChar);
    setIndentation();

    setIndentationIfNextCharIsOpeningQuoteMark(nextChar);
  }
  else if(m_nameMod == 1)
  {
    throw SystemError("NAME_MOD=1 is unsupported.");
  }
  else if(m_nameMod == 2)
  {
    // This doesn't actually fix the problem in Planetarian because
    // the call to set the name and the actual quotetext are in two
    // different strings. This logic will need to be moved.
//    setIndentationIfNextCharIsOpeningQuoteMark(nextChar);
  }
  else
  {
    throw SystemError("Invalid");
  }
}

// -----------------------------------------------------------------------

void SDLTextWindow::setIndentationIfNextCharIsOpeningQuoteMark(
  const std::string& nextChar)
{
  // Check to see if we set the indentation after the
  string::const_iterator it = nextChar.begin();
  int nextCodepoint = utf8::next(it, nextChar.end());
  if(nextCodepoint == 0x300C || nextCodepoint == 0x300E ||
     nextCodepoint == 0xFF08)
  {
    m_currentIndentationInPixels = m_insertionPointX + m_fontSizeInPixels +
      m_xSpacing;
  }
}

// -----------------------------------------------------------------------

void SDLTextWindow::hardBrake()
{
  m_insertionPointX = m_currentIndentationInPixels;
  m_insertionPointY += (m_fontSizeInPixels + m_ySpacing + m_rubySize);
  m_currentLineNumber++;
}

// -----------------------------------------------------------------------

void SDLTextWindow::resetIndentation()
{
  m_currentIndentationInPixels = 0;
}

// -----------------------------------------------------------------------

/**
 * @todo Make this pass the \#WINDOW_ATTR color off wile rendering the
 *       wakuBacking.
 */
void SDLTextWindow::render(RLMachine& machine)
{
  if(m_surface && isVisible())
  {
    Size surfaceSize = m_surface->size();

    // POINT
    int boxX = boxX1();
    int boxY = boxY1();

    if(m_wakuBacking)
    {
      Size backingSize = m_wakuBacking->size();
      // COLOUR
      m_wakuBacking->renderToScreenAsColorMask(
        Rect(Point(0, 0), backingSize),
        Rect(Point(boxX, boxY), backingSize),
        m_colour, m_filter);
    }

    if(m_wakuMain)
    {
      Size mainSize = m_wakuMain->size();
      m_wakuMain->renderToScreen(
        Rect(Point(0, 0), mainSize), Rect(Point(boxX, boxY), mainSize), 255);
    }

    if(m_wakuButton)
      renderButtons(machine);

    int x = textX1();
    int y = textY1();

    if(inSelectionMode())
    {
      for_each(m_selections.begin(), m_selections.end(),
               bind(&SelectionElement::render, _1));
    }
    else
    {
      m_surface->renderToScreen(
        Rect(Point(0, 0), surfaceSize),
        Rect(Point(x, y), surfaceSize),
        255);
    }
  }
}

// -----------------------------------------------------------------------

/**
 * @todo Move the offset magic numbers into constants, and make that a
 *       member of TextWindowButton; this function because a trivial
 *       iteration then.
 * @todo Push this logic up to TextWindow; This is logic, not an
 *       implementation detail.
 */
void SDLTextWindow::renderButtons(RLMachine& machine)
{
  m_buttonMap["CLEAR_BOX"].render(machine, *this, m_wakuButton, 8);

  m_buttonMap["MSGBKLEFT_BOX"].render(machine, *this, m_wakuButton, 24);
  m_buttonMap["MSGBKRIGHT_BOX"].render(machine, *this, m_wakuButton, 32);

  m_buttonMap["EXBTN_000_BOX"].render(machine, *this, m_wakuButton, 40);
  m_buttonMap["EXBTN_001_BOX"].render(machine, *this, m_wakuButton, 48);
  m_buttonMap["EXBTN_002_BOX"].render(machine, *this, m_wakuButton, 56);

  m_buttonMap["READJUMP_BOX"].render(machine, *this, m_wakuButton, 104);
  m_buttonMap["AUTOMODE_BOX"].render(machine, *this, m_wakuButton, 112);
}

// -----------------------------------------------------------------------

void SDLTextWindow::setWakuMain(RLMachine& machine, const std::string& name)
{
  if(name != "")
  {
    GraphicsSystem& gs = machine.system().graphics();
    m_wakuMain =
      dynamic_pointer_cast<SDLSurface>(
        gs.loadSurfaceFromFile(findFile(machine, name)));
  }
  else
    m_wakuMain.reset();
}

// -----------------------------------------------------------------------

void SDLTextWindow::setWakuBacking(RLMachine& machine, const std::string& name)
{
  if(name != "")
  {
    GraphicsSystem& gs = machine.system().graphics();
    shared_ptr<SDLSurface> s = dynamic_pointer_cast<SDLSurface>(
      gs.loadSurfaceFromFile(findFile(machine, name)));
    s->setIsMask(true);
    m_wakuBacking = s;
  }
  else
    m_wakuBacking.reset();
}

// -----------------------------------------------------------------------

void SDLTextWindow::setWakuButton(RLMachine& machine, const std::string& name)
{
  if(name != "")
  {
    GraphicsSystem& gs = machine.system().graphics();
    m_wakuButton = dynamic_pointer_cast<SDLSurface>(
      gs.loadSurfaceFromFile(findFile(machine, name)));
  }
  else
    m_wakuButton.reset();
}

// -----------------------------------------------------------------------

void SDLTextWindow::markRubyBegin()
{
  m_rubyBeginPoint = m_insertionPointX;
}

// -----------------------------------------------------------------------

void SDLTextWindow::displayRubyText(RLMachine& machine,
                                    const std::string& utf8str)
{
  if(m_rubyBeginPoint != -1)
  {
    int endPoint = m_insertionPointX - m_xSpacing;

    if(m_rubyBeginPoint > endPoint)
    {
      m_rubyBeginPoint = -1;
      throw rlvm::Exception("We don't handle ruby across line breaks yet!");
    }

    SDL_Color color;
    RGBColourToSDLColor(m_fontColour, &color);
    SDL_Surface* tmp =
      TTF_RenderUTF8_Blended(m_rubyFont.get(), utf8str.c_str(), color);

    // Render glyph to surface
    int w = tmp->w;
    int h = tmp->h;
    int heightLocation = m_insertionPointY - rubyTextSize();
    int widthStart =
      int(m_rubyBeginPoint + ((endPoint - m_rubyBeginPoint) * 0.5f) -
          (w * 0.5f));
    m_surface->blitFROMSurface(
      tmp,
      Rect(Point(0, 0), Size(w, h)),
      Rect(Point(widthStart, heightLocation), Size(w, h)),
      255);
    SDL_FreeSurface(tmp);

    machine.system().graphics().markScreenAsDirty(GUT_TEXTSYS);

    m_rubyBeginPoint = -1;
  }
}

// -----------------------------------------------------------------------

void SDLTextWindow::addSelectionItem(const std::string& utf8str)
{
  // Render the incoming string for both selected and not-selected.
  SDL_Color color;
  RGBColourToSDLColor(m_fontColour, &color);
  SDL_Surface* normal =
    TTF_RenderUTF8_Blended(m_font.get(), utf8str.c_str(), color);

  // Copy and invert the surface for whatever.
  SDL_Surface* inverted = AlphaInvert(normal);

  // Figure out xpos and ypos
  // POINT
  SelectionElement* element = new SelectionElement(
    shared_ptr<Surface>(new SDLSurface(normal)),
    shared_ptr<Surface>(new SDLSurface(inverted)),
    selectionCallback(), getNextSelectionID(),
    Point(textX1() + m_insertionPointX, textY1() + m_insertionPointY));

  m_insertionPointY += (m_fontSizeInPixels + m_ySpacing + m_rubySize);
  m_selections.push_back(element);
}

// -----------------------------------------------------------------------

void SDLTextWindow::endSelectionMode()
{
  m_selections.clear();
  TextWindow::endSelectionMode();

  clearWin();
}
