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
 * @file   GraphicsSystem.hpp
 * @author Elliot Glaysher
 * @date   Sun Oct 15 15:07:23 2006
 * 
 * @brief  Defines the abstract interface to the Graphics system.
 */

#ifndef __GraphicsSystem_hpp__
#define __GraphicsSystem_hpp__

#include <string>

#include <boost/shared_ptr.hpp>
#include "Systems/Base/GraphicsObject.hpp"

class Surface;
class RLMachine;

/** 
 * Abstract interface to a graphics system. Specialize this class for
 * each system you plan on running RLVM on. For now, there's only one
 * derived class; SDLGraphicsSystem.
 *
 * Two device contexts must be allocated during initialization; DC 0,
 * which should refer to a surface that is (usually) blitted onto the
 * screen immediatly after it is written to, and DC 1, which is simply
 * guarenteed to be allocated, and is guarenteed to not be smaller
 * then the screen. (Many {rec,grp} functions will load data onto DC1
 * and then copy it onto DC0 with some sort of fancy transition
 * effect.)
 */
class GraphicsSystem
{
public:
  /** 
   * The current display context drawing mode. The Reallive system
   * will update the screen after certain events in user code
   * regarding DCs.
   * 
   * Note that these are not the only times when the screen will be
   * updated. Most functions that deal with text windows will trigger
   * screen updates. (Object manipulation functions *don't*.) Having
   * this fine level of control is why DCs are often used for smooth
   * animation...
   */
  enum DCScreenUpdateMode {
    /// The screen will be redrawn after every load or blit to DC 0.
    SCREENUPDATEMODE_AUTOMATIC,   
    /// We currently don't understand how this differs from automatic
    /// mode. We declare it anyway for compatibility and the hope that
    /// someday we will.
    SCREENUPDATEMODE_SEMIAUTOMATIC,
    /// The screen is updated after refresh() is called
    SCREENUPDATEMODE_MANUAL
  };

private:
  /// Default grp name (used in grp* and rec* functions where filename
  /// is '???')
  std::string m_defaultGrpName;

  /// Default bgr name (used in bgr* functions where filename is
  /// '???')
  std::string m_defaultBgrName;

  /// Current screen update mode
  DCScreenUpdateMode m_screenUpdateMode;

public:
  GraphicsSystem();
  virtual ~GraphicsSystem();

  void setDefaultGrpName(const std::string& name) { m_defaultGrpName = name; }
  const std::string& defaultGrpName() const { return m_defaultGrpName; }
  void setDefaultBgrName(const std::string& name) { m_defaultBgrName = name; }
  const std::string& defaultBgrName() const { return m_defaultBgrName; }

  DCScreenUpdateMode screenUpdateMode() const { return m_screenUpdateMode; }
  void setScreenUpdateMode(DCScreenUpdateMode u) { m_screenUpdateMode = u; }

  // Marks the screen as dirty; something is done about this if we are
  // in automatic mode.
  virtual void markScreenAsDirty() { }

  // Marks the screen for refresh; we refresh the screen the next time
  // the graphics system is executed.
  virtual void markScreenForRefresh() { }

  virtual void beginFrame() { }
  virtual void refresh(RLMachine& machine) = 0;
  virtual void endFrame() { }

  virtual boost::shared_ptr<Surface> renderToSurfaceWithBg(
    RLMachine& machine, boost::shared_ptr<Surface> bg) { return boost::shared_ptr<Surface>(); }


  /** 
   * Called from the game loop; Does everything that's needed to keep
   * things up.
   */
  virtual void executeGraphicsSystem(RLMachine& machine) = 0;

  virtual int screenWidth() const = 0;
  virtual int screenHeight() const = 0;

  virtual void allocateDC(int dc, int width, int height) = 0;
  virtual void freeDC(int dc) = 0;

/*
  virtual void getDCPixel(int x, int y, int dc, int& r, int& g, int& b) = 0;
*/

  // ----------------------------------------- [ Surface loading functions ]
  virtual Surface* loadSurfaceFromFile(const std::string& filename) = 0;

  virtual boost::shared_ptr<Surface> getDC(int dc) = 0;


  // ----------------------------------- [ Object getter/factory functions ]
  virtual void promoteObjects() = 0;

  virtual GraphicsObjectData* buildObjOfFile(RLMachine& machine, 
                                             const std::string& filename) = 0;

  /// Object getters
  /// layer == 0 for fg, layer == 1 for bg.
  virtual GraphicsObject& getObject(int layer, int objNumber) = 0;
  virtual void setObject(int layer, int objNumber, GraphicsObject& object) {}
};

const static int OBJ_FG_LAYER = 0;
const static int OBJ_BG_LAYER = 1;

#endif