// This file is part of RLVM, a RealLive virutal machine clone.
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

/**
 * @file   Module_Grp.cpp
 * @author Elliot Glaysher
 * @date   Wed Oct  4 16:45:44 2006
 * @ingroup ModulesOpcodes
 * @brief  Implements the Graphics module (mod<1:33>).
 */

#include "Modules/Module_Grp.hpp"
#include "MachineBase/RLOperation.hpp"
#include "MachineBase/RLMachine.hpp"

#include "Systems/Base/System.hpp"
#include "Systems/Base/GraphicsSystem.hpp"
#include "Systems/Base/Surface.hpp"

#include "Modules/Effect.hpp"
#include "Modules/EffectFactory.hpp"

#include "libReallive/gameexe.h"

#include <iostream>

#include <boost/scoped_ptr.hpp>

#include "Utilities.h"

using namespace std;
using namespace boost;

const int SEL_SIZE = 16;

/**
 * @defgroup ModuleGrp The Graphics Module (mod<1:33>)
 * @ingroup ModulesOpcodes
 * 
 * Module that describes various graphics commands that operate on the DCs.
 *
 * @{
 */

// -----------------------------------------------------------------------

namespace {

/** 
 * Performs half the grunt work of a recOpen command; Copies DC0 to
 * DC1, loads a graphics file, and then composites that file to DC1.
 * 
 * Note that it works in rec coordinate space; grp commands must
 * convert from grp coordinate space.
 * 
 * @param graphics The graphics system to composite on
 * @param fileName The full filename (as returned by findFile)
 * @param x Source X coordinate
 * @param y Source Y coordinate
 * @param width Width of area to composite
 * @param height Height of area to composite
 * @param dx Destination X coordinate
 * @param dy Destination Y coordinate
 * @param opacity Opacity in range from 0 - 255
 */
void loadImageToDC1(GraphicsSystem& graphics,
                    const std::string& fileName,
                    int x, int y, int width, int height,
                    int dx, int dy, int opacity, bool useAlpha)
{
  shared_ptr<Surface> dc0 = graphics.getDC(0);
  shared_ptr<Surface> dc1 = graphics.getDC(1);

  // Inclusive ranges are a monstrosity to computer people
  width++;
  height++;

  dc0->blitToSurface(*dc1,
                     0, 0, dc0->width(), dc0->height(),
                     0, 0, dc0->width(), dc0->height(),
                     255);

  // Load the section of the image file on top of dc1
  scoped_ptr<Surface> surface(graphics.loadSurfaceFromFile(fileName));
  surface->blitToSurface(*graphics.getDC(1),
                         x, y, width, height,
                         dx, dy, width, height,
                         opacity, useAlpha);
}

// -----------------------------------------------------------------------

void loadDCToDC1(GraphicsSystem& graphics,
                 int srcDc,
                 int x, int y, int width, int height,
                 int dx, int dy, int opacity)
{
  shared_ptr<Surface> dc0 = graphics.getDC(0);
  shared_ptr<Surface> dc1 = graphics.getDC(1);
  shared_ptr<Surface> src = graphics.getDC(srcDc);

  // Inclusive ranges are a monstrosity to computer people
  width++;
  height++;

  dc0->blitToSurface(*dc1,
                     0, 0, dc0->width(), dc0->height(),
                     0, 0, dc0->width(), dc0->height(),
                     255);
  src->blitToSurface(*dc1,
                     x, y, width, height,
                     dx, dy, width, height,
                     opacity, false);
}

}

// -----------------------------------------------------------------------

/** 
 * Abstract interface for a space. Taken as a parameter to most Grp
 * classess.
 */
struct SPACE {
  virtual vector<int> getEffect(RLMachine& machine, int effectNum) = 0;
  virtual void translateToRec(int x1, int y1, int& x2, int& y2) = 0;
  virtual LongOperation* buildEffectFrom(RLMachine& machine, 
                                         boost::shared_ptr<Surface> src,
                                         boost::shared_ptr<Surface> dst,
                                         int effectNum) = 0;
};

// -----------------------------------------------------------------------

/**
 * A large number of the operation structs in the Grp module are
 * written in a generic way so that they can be done in either rec or
 * grp coordinate space. GRP_SPACE or REC_SPACE are passed as
 * parameters to these operations.
 * 
 * @see REC_SPACE
 * @see Grp_open_0
 */
struct GRP_SPACE : public SPACE {
  /** 
   * Gets the effect vector for grp coordinates (#SEL)
   */
  virtual vector<int> getEffect(RLMachine& machine, int effectNum) 
  {
    return machine.system().gameexe()("SEL", effectNum).to_intVector();
  }

  /** 
   * Changes the coordinate types. All operations internally are done in
   * rec coordinates, (x, y, width, height). The GRP functions pass
   * parameters of the format (x1, y1, x2, y2).
   * 
   * @param x1 X coordinate. Not changed by this function
   * @param y1 Y coordinate. Not changed by this function
   * @param x2 X2. In place changed to width.
   * @param y2 Y2. In place changed to height.
   */
  virtual void translateToRec(int x1, int y1, int& x2, int& y2)
  {
    x2 = x2 - x1;
    y2 = y2 - y1;
  }

  virtual LongOperation* buildEffectFrom(RLMachine& machine, 
                                         boost::shared_ptr<Surface> src,
                                         boost::shared_ptr<Surface> dst,
                                         int effectNum) 
  {
    return EffectFactory::buildFromSEL(machine, src, dst, src, effectNum);
  }

  static SPACE& get() {
    static GRP_SPACE space;
    return space;
  }
};

// -----------------------------------------------------------------------

struct REC_SPACE : public SPACE {
  /** 
   * Gets the effect vector for grp coordinates (#SELR)
   */
  virtual vector<int> getEffect(RLMachine& machine, int effectNum) 
  {
    return machine.system().gameexe()("SELR", effectNum).to_intVector();
  }

  /** 
   * Don't do anything; leave the incoming coordinates as they are.
   */
  virtual void translateToRec(int x1, int y1, int& x2, int& y2)
  {
  }

  virtual LongOperation* buildEffectFrom(RLMachine& machine, 
                                         boost::shared_ptr<Surface> src,
                                         boost::shared_ptr<Surface> dst,
                                         int effectNum) 
  {
    return EffectFactory::buildFromSELR(machine, src, dst, src, effectNum);
  }

  static SPACE& get() {
    static REC_SPACE space;
    return space;
  }
};


// -----------------------------------------------------------------------

/** 
 * Implements op<1:Grp:00015, 0>, fun allocDC('DC', 'width', 'height').
 * 
 * Allocates a blank width � height bitmap in dc. Any DC apart from DC
 * 0 may be allocated thus, although DC 1 is never given a size
 * smaller than the screen resolution. Any previous contents of dc are
 * erased.
 */
struct Grp_allocDC : public RLOp_Void_3< IntConstant_T, IntConstant_T,
                                         IntConstant_T > {
  void operator()(RLMachine& machine, int dc, int width, int height) {
    machine.system().graphics().allocateDC(dc, width, height);
  }
};

// -----------------------------------------------------------------------

/** 
 * Implements op<1:Grp:00016, 0>, fun freeDC('DC').
 * 
 * Frees dc, releasing the memory allocated to it. DC may not be 0; if
 * it is 1, DC 1 will be blanked, but not released.
 */
struct Grp_freeDC : public RLOp_Void_1< IntConstant_T > {
  void operator()(RLMachine& machine, int dc) {
    machine.system().graphics().freeDC(dc);
  }
};

// -----------------------------------------------------------------------

/** 
 * Implements op<1:Grp:00031, 0>, fun wipe('DC', 'r', 'g', 'b')
 * 
 * Fills dc with the colour indicated by the given RGB triplet.
 */
struct Grp_wipe : public RLOp_Void_4< IntConstant_T, IntConstant_T,
                                      IntConstant_T, IntConstant_T > {
  void operator()(RLMachine& machine, int dc, int r, int g, int b) {
    machine.system().graphics().getDC(dc)->fill(r, g, b, 255);
  }
};

// -----------------------------------------------------------------------
// {grp,rec}Load Commands
// -----------------------------------------------------------------------

/**
 * Implements the {grp,rec}(Mask)?Load family of functions.
 *
 * Loads filename into dc; note that filename may not be '???'. 
 *
 * @note Since this function deals with the entire screen (and
 * therefore doesn't need to worry about the difference between
 * grp/rec coordinate space), we write one function for both versions.
 */
struct Grp_load_1 : public RLOp_Void_3< StrConstant_T, IntConstant_T, 
                                        DefaultIntValue_T<255> > {
  bool m_useAlpha;
  Grp_load_1(bool in) : m_useAlpha(in) {}

  void operator()(RLMachine& machine, string filename, int dc, int opacity) {
    filename = findFile(machine, filename);
    GraphicsSystem& graphics = machine.system().graphics();
    scoped_ptr<Surface> surface(graphics.loadSurfaceFromFile(filename));
    graphics.allocateDC(dc, graphics.screenWidth(), graphics.screenHeight());
    surface->blitToSurface(*graphics.getDC(dc),
                           0, 0, surface->width(), surface->height(),
                           0, 0, surface->width(), surface->height(),
                           opacity, m_useAlpha);
  }
};

// -----------------------------------------------------------------------

/**
 * Implements op<1:Grp:00050, 3>, fun grpLoad(strC 'filename', 'DC',
 * 'x1', 'y1', 'x2', 'y2', 'dx', 'dy', 'alpha').
 *
 * Loads filename into dc; note that filename may not be '???'. Using
 * this form, the given area of the bitmap is loaded at the given
 * location.
 */
struct Grp_load_3 : public RLOp_Void_9< 
  StrConstant_T, IntConstant_T, IntConstant_T, IntConstant_T, 
  IntConstant_T, IntConstant_T, IntConstant_T, IntConstant_T,
  DefaultIntValue_T<255> > 
{
  bool m_useAlpha;
  SPACE& m_space;
  Grp_load_3(bool in, SPACE& space) : m_useAlpha(in), m_space(space) {}

  void operator()(RLMachine& machine, string filename, int dc,
                  int x1, int y1, int x2, int y2, int dx, int dy, int opacity) {
    filename = findFile(machine, filename);
    GraphicsSystem& graphics = machine.system().graphics();
    scoped_ptr<Surface> surface(graphics.loadSurfaceFromFile(filename));
    m_space.translateToRec(x1, y1, x2, y2);
//    graphics.getDC(dc).allocate(x2, y2);
    surface->blitToSurface(*graphics.getDC(dc),
                           x1, y1, x2, y2, dx, dy, x2, y2, opacity, m_useAlpha);    
  }
};

// -----------------------------------------------------------------------
// {grp,rec}Display
// -----------------------------------------------------------------------

struct Grp_display_1
  : public RLOp_Void_3< IntConstant_T, IntConstant_T, IntConstant_T > 
{
  SPACE& m_space;
  Grp_display_1(SPACE& space) : m_space(space) {}

  void operator()(RLMachine& machine, int dc, int effectNum, int opacity) 
  {
    vector<int> selEffect = m_space.getEffect(machine, effectNum);
    m_space.translateToRec(selEffect[0], selEffect[1], 
                        selEffect[2], selEffect[3]);

    GraphicsSystem& graphics = machine.system().graphics();
    loadDCToDC1(graphics, dc,
                selEffect[0], selEffect[1], selEffect[2], selEffect[3],
                selEffect[4], selEffect[5], opacity);

    // Set the long operation for the correct transition long operation
    shared_ptr<Surface> dc0 = graphics.getDC(0);
    shared_ptr<Surface> dc1 = graphics.getDC(1);
    machine.setLongOperation(m_space.buildEffectFrom(machine, dc1, dc0, effectNum));
  }
};

// -----------------------------------------------------------------------

struct Grp_display_0 : public RLOp_Void_2< IntConstant_T, IntConstant_T > {
  SPACE& m_space;
  Grp_display_1 m_delegate;
  Grp_display_0(SPACE& space) : m_space(space), m_delegate(space) {}

  void operator()(RLMachine& machine, int dc, int effectNum) {
    vector<int> selEffect = m_space.getEffect(machine, effectNum);
    m_delegate(machine, dc, effectNum, selEffect.at(14));
  }
};

// -----------------------------------------------------------------------

struct Grp_display_3 : public RLOp_Void_9< 
  IntConstant_T, IntConstant_T, IntConstant_T, IntConstant_T, 
  IntConstant_T, IntConstant_T, IntConstant_T, IntConstant_T,
  IntConstant_T>
{
  SPACE& m_space;
  Grp_display_3(SPACE& space) : m_space(space) {}

  void operator()(RLMachine& machine, int dc, int effectNum, 
                  int x1, int y1, int x2, int y2, int dx, int dy, int opacity)
  {
    GraphicsSystem& graphics = machine.system().graphics();
    m_space.translateToRec(x1, y1, x2, y2);
    loadDCToDC1(graphics, dc, x1, y1, x2, y2, dx, dy, opacity);

    // Set the long operation for the correct transition long operation
    shared_ptr<Surface> dc0 = graphics.getDC(0);
    shared_ptr<Surface> dc1 = graphics.getDC(1);
    machine.setLongOperation(m_space.buildEffectFrom(machine, dc1, dc0, effectNum));
  }
};

// -----------------------------------------------------------------------

/** 
 * @todo Finish documentation
 */
struct Grp_display_2 : public RLOp_Void_8< 
  IntConstant_T, IntConstant_T, IntConstant_T, IntConstant_T, 
  IntConstant_T, IntConstant_T, IntConstant_T, IntConstant_T>
{
  SPACE& m_space;
  Grp_display_3 delegate;
  Grp_display_2(SPACE& space) : m_space(space), delegate(space) {}

  void operator()(RLMachine& machine, int dc, int effectNum, 
                  int x1, int y1, int x2, int y2, int dx, int dy)
  {
    int opacity = m_space.getEffect(machine, effectNum).at(14);
    delegate(machine, dc, effectNum, x1, y1, x2, y2, dx, dy, 
             opacity);
  }
};


// -----------------------------------------------------------------------
// {grp,rec}Open
// -----------------------------------------------------------------------

/** 
 * Implements op<1:Grp:00076, 1>, fun grpOpen(strC 'filename', '\#SEL', 'opacity').
 * 
 * Load and display a bitmap. @em filename is loaded into DC1 with
 * opacity @em opacity, and then is passed off to whatever transition
 * effect, which will perform some intermediary steps and then render
 * DC1 to DC0.
 *
 * @todo factor out the common code between grpOpens!
 */
struct Grp_open_1 : public RLOp_Void_3< StrConstant_T, IntConstant_T, 
                                        IntConstant_T > {
  SPACE& m_space;
  bool m_useAlpha;
  Grp_open_1(bool in, SPACE& space) : m_space(space), m_useAlpha(in) {}

  void operator()(RLMachine& machine, string filename, int effectNum, 
                  int opacity)
  {
    vector<int> selEffect = m_space.getEffect(machine, effectNum);

    GraphicsSystem& graphics = machine.system().graphics();
    if(filename == "???") filename = graphics.defaultGrpName();
    filename = findFile(machine, filename);

    m_space.translateToRec(selEffect[0], selEffect[1], 
                        selEffect[2], selEffect[3]);
    loadImageToDC1(graphics, filename,
                   selEffect[0], selEffect[1], selEffect[2], selEffect[3],
                   selEffect[4], selEffect[5], opacity, m_useAlpha);

    // Set the long operation for the correct transition long operation
    shared_ptr<Surface> dc0 = graphics.getDC(0);
    shared_ptr<Surface> dc1 = graphics.getDC(1);
    machine.setLongOperation(m_space.buildEffectFrom(machine, dc1, dc0, effectNum));
  }
};

// -----------------------------------------------------------------------

/** 
 * Implements op<1:Grp:00076, 0>, fun grpOpen(strC 'filename', '\#SEL').
 * 
 * Load and display a bitmap. @em filename is loaded into DC1, and
 * then is passed off to whatever transition effect, which will
 * perform some intermediary steps and then render DC1 to DC0.
 */
struct Grp_open_0 : public RLOp_Void_2< StrConstant_T, IntConstant_T > {
  SPACE& m_space;
  Grp_open_1 m_delegate;
  Grp_open_0(bool in, SPACE& space) 
    : m_space(space), m_delegate(in, space) {}

  void operator()(RLMachine& machine, string filename, int effectNum) {
    vector<int> selEffect = m_space.getEffect(machine, effectNum);
    m_delegate(machine, filename, effectNum, selEffect[14]);
  }
};

// -----------------------------------------------------------------------

struct Grp_open_3 : public RLOp_Void_9< 
  StrConstant_T, IntConstant_T, IntConstant_T, IntConstant_T, 
  IntConstant_T, IntConstant_T, IntConstant_T, IntConstant_T,
  IntConstant_T>
{
  SPACE& m_space;
  bool m_useAlpha;
  Grp_open_3(bool in, SPACE& space) : m_space(space), m_useAlpha(in) {}

  void operator()(RLMachine& machine, string filename, int effectNum, 
                  int x1, int y1, int x2, int y2, int dx, int dy, int opacity)
  {
    GraphicsSystem& graphics = machine.system().graphics();
    if(filename == "???") filename = graphics.defaultGrpName();
    filename = findFile(machine, filename);

    m_space.translateToRec(x1, y1, x2, y2);
    loadImageToDC1(graphics, filename, x1, y1, x2, y2, dx, dy, opacity, m_useAlpha);

    // Set the long operation for the correct transition long operation
    shared_ptr<Surface> dc0 = graphics.getDC(0);
    shared_ptr<Surface> dc1 = graphics.getDC(1);
    machine.setLongOperation(m_space.buildEffectFrom(machine, dc1, dc0, effectNum));
  }
};

// -----------------------------------------------------------------------

/** 
 * Implements op<1:Grp:00076, 1>, fun grpOpen(strC 'filename', '\#SEL', 'opacity').
 * 
 * Load and display a bitmap. @em filename is loaded into DC1 with
 * opacity @em opacity, and then is passed off to whatever transition
 * effect, which will perform some intermediary steps and then render
 * DC1 to DC0.
 *
 * @todo Finish documentation
 */
struct Grp_open_2 : public RLOp_Void_8< 
  StrConstant_T, IntConstant_T, IntConstant_T, IntConstant_T, 
  IntConstant_T, IntConstant_T, IntConstant_T, IntConstant_T>
{
  SPACE& m_space;
  Grp_open_3 m_delegate;
  Grp_open_2(bool in, SPACE& space) : m_space(space), m_delegate(in, space) {}

  void operator()(RLMachine& machine, string filename, int effectNum, 
                  int x1, int y1, int x2, int y2, int dx, int dy)
  {
    int opacity = m_space.getEffect(machine, effectNum).at(14);
    m_delegate(machine, filename, effectNum, x1, y1, x2, y2, 
               dx, dy, opacity);
  }
};

// -----------------------------------------------------------------------

//(strC 'filename', 'x1', 'y1', 'x2', 'y2', 
// 'dx', 'dy', 'steps', 'effect', 'direction',
// 'interpolation', 'density', 'speed', '?', '?',
// 'alpha', '?')
struct Grp_open_4 : public RLOp_Void_17<
  StrConstant_T, IntConstant_T, IntConstant_T, IntConstant_T, IntConstant_T,
  IntConstant_T, IntConstant_T, IntConstant_T, IntConstant_T, IntConstant_T,
  IntConstant_T, IntConstant_T, IntConstant_T, IntConstant_T, IntConstant_T,
  IntConstant_T, IntConstant_T>
{
  SPACE& m_space;
  bool m_useAlpha;
  Grp_open_4(bool in, SPACE& space) : m_space(space), m_useAlpha(in) {}

  void operator()(RLMachine& machine, string filename,
                  int x1, int y1, int x2, int y2, int dx, int dy,
                  int time, int style, int direction, int interpolation,
                  int xsize, int ysize, int a, int b, int opacity, int c)
  {
    GraphicsSystem& graphics = machine.system().graphics();
    m_space.translateToRec(x1, y1, x2, y2);

    if(filename != "?")
    {
      if(filename == "???") filename = graphics.defaultGrpName();
      filename = findFile(machine, filename);

      loadImageToDC1(graphics, filename, x1, y1, x2, y2, dx, dy, 
                     opacity, m_useAlpha);
    }

    // Set the long operation for the correct transition long operation
    shared_ptr<Surface> dc0 = graphics.getDC(0);
    shared_ptr<Surface> dc1 = graphics.getDC(1);
    machine.setLongOperation(
      EffectFactory::build(machine, dc1, dc0, dc1, time, style, direction, 
                           interpolation, xsize, ysize, a, b, c));
  }
};


//(strC 'filename', 'x1', 'y1', 'x2', 'y2', 
// 'dx', 'dy', 'steps', 'effect', 'direction',
// 'interpolation', 'density', 'speed', '?', '?',
// 'alpha', '?')
struct Grp_openBg_4 : public RLOp_Void_17<
  StrConstant_T, IntConstant_T, IntConstant_T, IntConstant_T, IntConstant_T,
  IntConstant_T, IntConstant_T, IntConstant_T, IntConstant_T, IntConstant_T,
  IntConstant_T, IntConstant_T, IntConstant_T, IntConstant_T, IntConstant_T,
  IntConstant_T, IntConstant_T>
{
  SPACE& m_space;
  bool m_useAlpha;
  Grp_openBg_4(bool in, SPACE& space) : m_space(space), m_useAlpha(in) {}

  void operator()(RLMachine& machine, string filename,
                  int x1, int y1, int x2, int y2, int dx, int dy,
                  int time, int style, int direction, int interpolation,
                  int xsize, int ysize, int a, int b, int opacity, int c)
  {
    GraphicsSystem& graphics = machine.system().graphics();
    m_space.translateToRec(x1, y1, x2, y2);

    // Set the long operation for the correct transition long operation
    shared_ptr<Surface> dc0 = graphics.getDC(0);

    if(filename != "?")
    {
      if(filename == "???") filename = graphics.defaultGrpName();
      filename = findFile(machine, filename);

      loadImageToDC1(graphics, filename, x1, y1, x2, y2, dx, dy, 
                     opacity, m_useAlpha);
    }
    else 
    {
      // The current DC0 is really 
      dc0 = graphics.renderToSurfaceWithBg(machine, dc0);
    }

    // Promote the objects 
    graphics.promoteObjects();

    // Render the screen to a temporary 
    shared_ptr<Surface> dc1 = graphics.getDC(1);    
    shared_ptr<Surface> tmp = graphics.renderToSurfaceWithBg(machine, dc1);

    Effect* effect = EffectFactory::build(machine, tmp, dc0, dc1, time, 
                                          style, direction, interpolation,
                                          xsize, ysize, a, b, c);
    effect->setPerformFinalBlit(false);
    machine.setLongOperation(effect);
  }
};

// -----------------------------------------------------------------------
// {grp,rec}Copy
// -----------------------------------------------------------------------

struct Grp_copy_3 : public RLOp_Void_9<
  IntConstant_T, IntConstant_T, IntConstant_T, IntConstant_T,
  IntConstant_T, IntConstant_T, IntConstant_T, IntConstant_T,
  DefaultIntValue_T<255> > 
{
  SPACE& m_space;
  bool m_useAlpha;
  Grp_copy_3(bool in, SPACE& space) : m_space(space), m_useAlpha(in) {}

  void operator()(RLMachine& machine, int x1, int y1, int x2, int y2,
                  int src, int dx, int dy, int dst, int opacity) {
    // Copying to self is a noop
    if(src == dst)
      return;

    GraphicsSystem& graphics = machine.system().graphics();
    shared_ptr<Surface> sourceSurface = graphics.getDC(src);

    // Reallocate the destination so that it's the same size as the first.
//    graphics.allocateDC(dst, sourceSurface.width(), sourceSurface.height());

    m_space.translateToRec(x1, y1, x2, y2);

    sourceSurface->blitToSurface(
      *graphics.getDC(dst),
      x1, y1, x2, y2, dx, dy, x2, y2, opacity, m_useAlpha);
  }
};

// -----------------------------------------------------------------------

struct Grp_copy_1 : public RLOp_Void_3<IntConstant_T, IntConstant_T, 
                                       DefaultIntValue_T<255> > 
{
  bool m_useAlpha;
  Grp_copy_1(bool in) : m_useAlpha(in) {}

  void operator()(RLMachine& machine, int src, int dst, int opacity) {
    // Copying to self is a noop
    if(src == dst)
      return;

    GraphicsSystem& graphics = machine.system().graphics();
    shared_ptr<Surface> sourceSurface = graphics.getDC(src);

    // Reallocate the destination so that it's the same size as the first.
    graphics.allocateDC(dst, sourceSurface->width(), sourceSurface->height());

    sourceSurface->blitToSurface(
      *graphics.getDC(dst),
      0, 0, sourceSurface->width(), sourceSurface->height(),
      0, 0, sourceSurface->width(), sourceSurface->height(),
      opacity, m_useAlpha);
  }
};

// -----------------------------------------------------------------------
// {grp,rec}Fill
// -----------------------------------------------------------------------

struct Grp_fill_1 : public RLOp_Void_5<
  IntConstant_T, IntConstant_T, IntConstant_T, IntConstant_T, 
  DefaultIntValue_T<255> > {
  void operator()(RLMachine& machine, int dc, int r, int g, int b, int alpha) {
    machine.system().graphics().getDC(dc)->fill(r, g, b, alpha);
  }
};

// -----------------------------------------------------------------------

struct Grp_fill_3 : public RLOp_Void_9<
  IntConstant_T, IntConstant_T, IntConstant_T, IntConstant_T, 
  IntConstant_T, IntConstant_T, IntConstant_T, IntConstant_T, 
  DefaultIntValue_T<255> > 
{
  SPACE& m_space;
  Grp_fill_3(SPACE& space) : m_space(space) {}

  void operator()(RLMachine& machine, int x1, int y1, int x2, int y2,
                  int dc, int r, int g, int b, int alpha) {
    m_space.translateToRec(x1, y1, x2, y2);
    machine.system().graphics().getDC(dc)->fill(r, g, b, alpha, x1, y1, x2, y2);
  }
};

// -----------------------------------------------------------------------
// {grp,rec}Multi
// -----------------------------------------------------------------------

/**
 * Defines the fairly complex parameter definition for the list of
 * functions to call in a {grp,rec}Multi command.
 */
typedef Argc_T< 
  Special_T<
    StrConstant_T, // 0:copy
    Complex2_T<StrConstant_T, IntConstant_T>, // 1:copy
    Complex3_T<StrConstant_T, IntConstant_T, IntConstant_T>, // 2:copy
    // 3:area
    Complex7_T<StrConstant_T, IntConstant_T, IntConstant_T,
               IntConstant_T, IntConstant_T, IntConstant_T, 
               IntConstant_T>,
    // 4:area
    Complex8_T<StrConstant_T, IntConstant_T, IntConstant_T,
               IntConstant_T, IntConstant_T, IntConstant_T, 
               IntConstant_T, IntConstant_T>
    > >  MultiCommand;

// -----------------------------------------------------------------------

/**
 * Defines the weird multi commands. I will be the first to admit that
 * the following is fairly difficult to read; it comes from the
 * quagmire of composing Special_T and ComplexX_T templates.
 *
 * In the end, this operation struct simply dispatches the
 * Special/Complex commands to functions and other operation structs
 * that are clearer in purpose.
 * 
 * @todo Finish this operation; it's in an incomplete, but compiling
 *       state.
 * @see MultiCommand
 */
//template<typename SPACE>
/*
struct Grp_multi_1 : public RLOp_Void_4<StrConstant_T, IntConstant_T, 
                                        IntConstant_T, MultiCommand>
{
  void operator()(RLMachine& machine, string filename, int effect, int alpha,
                  MultiCommand::type commands)
  {
//    Grp_load_0(false)(machine, filename, effect);

    for(MultiCommand::type::iterator it = commands.begin(); it != commands.end();
        it++) 
    {
      switch(it->type)
      {
      case 0:
//        Grp_load_0(true)(machine, it->first.get<0>(), it->first.get<1>(),
//                         it->first.get<2>(), it->first.get<3>());
        break;
      case 1:
//        Grp_load_1(true)(machine, it->second.get<0>(), it->second.get<1>(),
//                         it->second.get<2>(), it->second.get<3>());
        break;
      case 2:

        break;
      case 3:
        Grp_load_3<SPACE>(true)(machine, it->fourth.get<0>(), 
                                it->fourth.get<1>(),
                                it->fourth.get<2>(),
                                it->fourth.get<3>(),
                                it->fourth.get<4>(),
                                it->fourth.get<5>(),
                                it->fourth.get<6>(),
                                255);
        break;
      case 4:
        Grp_load_3<SPACE>(true)(machine, it->fifth.get<0>(), 
                                it->fifth.get<1>(),
                                it->fifth.get<2>(),
                                it->fifth.get<3>(),
                                it->fifth.get<4>(),
                                it->fifth.get<5>(),
                                it->fifth.get<6>(),
                                it->fifth.get<7>());
        break;
      }
    }

    // Does this work?
    Grp_display_0<SPACE>()(1, effect);
  }
};
*/

// -----------------------------------------------------------------------
/*
template<typename SPACE>
struct Grp_multi_0 : public RLOp_Void_3<StrConstant_T, IntConstant_T, MultiCommand>
{
  void operator()(RLMachine& machine, string dc, int effect, 
                  MultiCommand::type commands)
  {
    Grp_multi_1<SPACE>()(machine, dc, effect, 255, commands);
  }     
};
*/     
                                                                 
// -----------------------------------------------------------------------

/**
 *
 * At minimum, we need to get these functions working for Kanon:
 *
 * [elliot@Kotori:~/Projects/Kanon] $  grep grp *.ke -h | cut -f 1 -d " " | sort | uniq  
 * grpBuffer
 * grpCopy
 * grpFill
 * grpMaskCopy
 * grpMaskOpen
 * grpMulti
 * grpOpenBg
 * [elliot@Kotori:~/Projects/Kanon] $  grep rec *.ke -h | cut -f 1 -d " " | sort | uniq
 * recCopy
 * recFill
 * recOpen
 * recOpenBg
 */
GrpModule::GrpModule()
  : RLModule("Grp", 1, 33)
{
  SPACE& GRP = GRP_SPACE::get();
  SPACE& REC = REC_SPACE::get();

  addOpcode(15, 0, "allocDC", new Grp_allocDC);
  addOpcode(16, 0, "freeDC", new Grp_freeDC);

  // addOpcode(20, 0, new Grp_grpLoadMask);
  // addOpcode(30, 0, new Grp_grpTextout);

  addOpcode(31, 0, "wipe", new Grp_wipe);
  // addOpcode(32, 0, new Grp_shake);

  addOpcode(50, 0, "grpLoad", new Grp_load_1(false));
  addOpcode(50, 1, "grpLoad", new Grp_load_1(false));
  addOpcode(50, 2, "grpLoad", new Grp_load_3(false, GRP));
  addOpcode(50, 3, "grpLoad", new Grp_load_3(false, GRP));
  addOpcode(51, 0, "grpMaskLoad", new Grp_load_1(true));
  addOpcode(51, 1, "grpMaskLoad", new Grp_load_1(true));
  addOpcode(51, 2, "grpMaskLoad", new Grp_load_3(true, GRP));
  addOpcode(51, 3, "grpMaskLoad", new Grp_load_3(true, GRP));

  // These are grpBuffer, which is very similar to grpLoad and Haeleth
  // doesn't know how they differ. For now, we just assume they're
  // equivalent.
  addOpcode(70, 0, "grpBuffer", new Grp_load_1(false));
  addOpcode(70, 1, "grpBuffer", new Grp_load_1(false));
  addOpcode(70, 2, "grpBuffer", new Grp_load_3(false, GRP));
  addOpcode(70, 3, "grpBuffer", new Grp_load_3(false, GRP));
  addOpcode(71, 0, "grpMaskBuffer", new Grp_load_1(true));
  addOpcode(71, 1, "grpMaskBuffer", new Grp_load_1(true));
  addOpcode(71, 2, "grpMaskBuffer", new Grp_load_3(true, GRP));
  addOpcode(71, 3, "grpMaskBuffer", new Grp_load_3(true, GRP));

  addOpcode(72, 0, "grpDisplay", new Grp_display_0(GRP));
  addOpcode(72, 1, "grpDisplay", new Grp_display_1(GRP));
  addOpcode(72, 2, "grpDisplay", new Grp_display_2(GRP));
  addOpcode(72, 3, "grpDisplay", new Grp_display_3(GRP));

  // These are supposed to be grpOpenBg, but until I have the object
  // layer working, this simply does the same thing.
  addOpcode(73, 0, "grpOpenBg", new Grp_open_0(false, GRP));
  addOpcode(73, 1, "grpOpenBg", new Grp_open_1(false, GRP));
  addOpcode(73, 2, "grpOpenBg", new Grp_open_2(false, GRP));
  addOpcode(73, 3, "grpOpenBg", new Grp_open_3(false, GRP));
  addOpcode(73, 4, "grpOpenBg", new Grp_openBg_4(false, GRP));

  addOpcode(74, 0, "grpMaskOpen", new Grp_open_0(true, GRP));
  addOpcode(74, 1, "grpMaskOpen", new Grp_open_1(true, GRP));
  addOpcode(74, 2, "grpMaskOpen", new Grp_open_2(true, GRP));
  addOpcode(74, 3, "grpMaskOpen", new Grp_open_3(true, GRP));
  addOpcode(74, 4, "grpMaskOpen", new Grp_open_4(true, GRP));

  addOpcode(76, 0, "grpOpen", new Grp_open_0(false, GRP));
  addOpcode(76, 1, "grpOpen", new Grp_open_1(false, GRP));
  addOpcode(76, 2, "grpOpen", new Grp_open_2(false, GRP));
  addOpcode(76, 3, "grpOpen", new Grp_open_3(false, GRP));
  addOpcode(76, 4, "grpOpen", new Grp_open_4(false, GRP));

  addOpcode(100, 0, "grpCopy", new Grp_copy_1(false));
  addOpcode(100, 1, "grpCopy", new Grp_copy_1(false));
  addOpcode(100, 2, "grpCopy", new Grp_copy_3(false, GRP));
  addOpcode(100, 3, "grpCopy", new Grp_copy_3(false, GRP));
  addOpcode(101, 0, "grpMaskCopy", new Grp_copy_1(true));
  addOpcode(101, 1, "grpMaskCopy", new Grp_copy_1(true));
  addOpcode(101, 2, "grpMaskCopy", new Grp_copy_3(true, GRP));
  addOpcode(101, 3, "grpMaskCopy", new Grp_copy_3(true, GRP));

  addOpcode(201, 0, "grpFill", new Grp_fill_1);
  addOpcode(201, 1, "grpFill", new Grp_fill_1);
  addOpcode(201, 2, "grpFill", new Grp_fill_3(GRP));
  addOpcode(201, 3, "grpFill", new Grp_fill_3(GRP));

  // -----------------------------------------------------------------------
  
  addOpcode(1050, 0, "recLoad", new Grp_load_1(false));
  addOpcode(1050, 1, "recLoad", new Grp_load_1(false));
  addOpcode(1050, 2, "recLoad", new Grp_load_3(false, REC));
  addOpcode(1050, 3, "recLoad", new Grp_load_3(false, REC));

  addOpcode(1052, 0, "grpDisplay", new Grp_display_0(REC));
  addOpcode(1052, 1, "grpDisplay", new Grp_display_1(REC));
  addOpcode(1052, 2, "grpDisplay", new Grp_display_2(REC));
  addOpcode(1052, 3, "grpDisplay", new Grp_display_3(REC));

  // These are supposed to be recOpenBg, but until I have the object
  // layer working, this simply does the same thing.
  addOpcode(1053, 0, "recOpenBg", new Grp_open_0(false, REC));
  addOpcode(1053, 1, "recOpenBg", new Grp_open_1(false, REC));
  addOpcode(1053, 2, "recOpenBg", new Grp_open_2(false, REC));
  addOpcode(1053, 3, "recOpenBg", new Grp_open_3(false, REC));
  addOpcode(1053, 4, "recOpenBg", new Grp_openBg_4(false, REC));

  addOpcode(1054, 0, "recMaskOpen", new Grp_open_0(true, REC));
  addOpcode(1054, 1, "recMaskOpen", new Grp_open_1(true, REC));
  addOpcode(1054, 2, "recMaskOpen", new Grp_open_2(true, REC));
  addOpcode(1054, 3, "recMaskOpen", new Grp_open_3(true, REC));
  addOpcode(1054, 4, "recMaskOpen", new Grp_open_4(true, REC));

  addOpcode(1056, 0, "recOpen", new Grp_open_0(false, REC));
  addOpcode(1056, 1, "recOpen", new Grp_open_1(false, REC));
  addOpcode(1056, 2, "recOpen", new Grp_open_2(false, REC));
  addOpcode(1056, 3, "recOpen", new Grp_open_3(false, REC));
  addOpcode(1056, 4, "recOpen", new Grp_open_4(false, REC));

  addOpcode(1100, 0, "recCopy", new Grp_copy_1(false));
  addOpcode(1100, 1, "recCopy", new Grp_copy_1(false));
  addOpcode(1100, 2, "recCopy", new Grp_copy_3(false, REC));
  addOpcode(1100, 3, "recCopy", new Grp_copy_3(false, REC));
  addOpcode(1101, 0, "recMaskCopy", new Grp_copy_1(true));
  addOpcode(1101, 1, "recMaskCopy", new Grp_copy_1(true));
  addOpcode(1101, 2, "recMaskCopy", new Grp_copy_3(true, REC));
  addOpcode(1101, 3, "recMaskCopy", new Grp_copy_3(true, REC));

  addOpcode(1201, 0, "recFill", new Grp_fill_1);
  addOpcode(1201, 1, "recFill", new Grp_fill_1);
  addOpcode(1201, 2, "recFill", new Grp_fill_3(REC));
  addOpcode(1201, 3, "recFill", new Grp_fill_3(REC));
}

// @}