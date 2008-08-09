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

/**
 * @file   GraphicsSystem.hpp
 * @author Elliot Glaysher
 * @date   Sun Oct 15 15:07:23 2006
 *
 * @brief  Defines the abstract interface to the Graphics system.
 */

#ifndef __GraphicsSystem_hpp__
#define __GraphicsSystem_hpp__

#include "Systems/Base/EventHandler.hpp"
#include "Systems/Base/Rect.hpp"

#include <boost/filesystem/path.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/shared_ptr.hpp>
#include <map>
#include <string>
#include <vector>

// -----------------------------------------------------------------------

class CGMTable;
class Gameexe;
class GraphicsObject;
class GraphicsObjectData;
class GraphicsStackFrame;
class MouseCursor;
class RLMachine;
class Size;
class Surface;
class System;
struct ObjectSettings;

template<typename T> class LazyArray;

// -----------------------------------------------------------------------

/**
 * Variables and configuration data that are global across all save
 * game files in a game.
 */
struct GraphicsSystemGlobals
{
  GraphicsSystemGlobals();
  GraphicsSystemGlobals(Gameexe& gameexe);

  /// ShowObject flags
  int showObject1, showObject2;

  int showWeather;

  /// boost::serialization support
  template<class Archive>
  void serialize(Archive& ar, const unsigned int version)
  {
    ar & showObject1 & showObject2 & showWeather;
  }
};

// -----------------------------------------------------------------------

/**
 * When marking the screen as dirty, we need to know what kind of
 * operation was done
 */
enum GraphicsUpdateType {
  GUT_DRAW_DC0,
  GUT_DISPLAY_OBJ,
  GUT_TEXTSYS,
  GUT_MOUSE_MOTION
};

// -----------------------------------------------------------------------

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

class GraphicsSystem : public MouseListener
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

public:
  GraphicsSystem(System& system, Gameexe& gameexe);
  virtual ~GraphicsSystem();

  bool isResponsibleForUpdate() const { return m_isResponsibleForUpdate; }
  void setIsResponsibleForUpdate(bool in);

  void setDefaultGrpName(const std::string& name) { m_defaultGrpName = name; }
  const std::string& defaultGrpName() const { return m_defaultGrpName; }
  void setDefaultBgrName(const std::string& name) { m_defaultBgrName = name; }
  const std::string& defaultBgrName() const { return m_defaultBgrName; }

  DCScreenUpdateMode screenUpdateMode() const { return m_screenUpdateMode; }
  virtual void setScreenUpdateMode(DCScreenUpdateMode u);

  System& system() { return m_system; }

  /**
   * @name Mouse Cursor Management
   *
   * @{
   */

  /**
   * Whether we are using a custom cursor. Verifies that there was a
   * \#MOUSE_CURSOR entry in the Gameexe.ini file, and that the currently
   * selected cursor exists.
   */
  int useCustomCursor();

  /// Sets the cursor to the incoming cursor index.
  virtual void setCursor(RLMachine& machine, int cursor);

  /// Returns the current index.
  int cursor() const { return m_cursor; }

  /// Whether we display a cursor at all
  void setShowCursor(const int in) { m_showCurosr = in; }
  /// @}

  /**
   * @name Graphics Stack
   *
   * @{
   */
  GraphicsStackFrame& addGraphicsStackFrame(const std::string& name);

  std::vector<GraphicsStackFrame>& graphicsStack();

  int stackSize() const;
  void clearStack();

  void stackPop(int numItems);

  /// Replays the graphics stack. This is called after we've reloaded
  /// a saved game.
  void replayGraphicsStack(RLMachine& machine);
  /// @}


  /**
   * @name Title management
   *
   * We put this here since there's no good place to put it in
   * general. (And it was here before I figured out what I was doing)
   *
   * @{
   */

  /**
   * Sets the current value of the subtitle, as set with title(). This
   * is virtual so that UTF8 or other charset systems can convert for
   * their own internal copy.
   *
   * @param cp932str The subtitle (encoded in cp932 or similar)
   * @param textEncoding The encoding type (as passed in to cp932toUTF8)
   */
  virtual void setWindowSubtitle(const std::string& cp932str, int textEncoding);

  /**
   * Returns the current window subtitle.
   *
   * @return The current window subtitle in cp932 encoding.
   */
  const std::string& windowSubtitle() const;

  bool displaySubtitle() const { return m_displaySubtitle; }
  /// @}

  /**
   * @name Saving Global Values
   *
   * @{
   */
  GraphicsSystemGlobals& globals() { return m_globals; }
  /// @}

  /**
   * @name Show Object flags
   *
   * The `show object' flags are used to provide a way of enabling or
   * disabling interface elements from the menu. If an object's
   * `ObjectOnOff' property is set to 1 or 2, it will be shown or
   * hidden depending on the corresponding `show object' flag. This is
   * one of the properties controlled by the \#OBJECT variables in
   * gameexe.ini.
   *
   * In Clannad, ShowObject1 is used to control display of the date
   * marker at the top left of the screen (object 84).
   *
   * @note { @b A value of 0 from these functions means that data is
   *       visible by default and a value of 1 means thaey are invisible. }
   *
   * @{
   */
  void setShowObject1(const int in);
  int showObject1() const { return m_globals.showObject1; }

  void setShowObject2(const int in);
  int showObject2() const { return m_globals.showObject1; }
  /// @}

  /**
   * @name Other object display settings
   *
   * @{
   */
  void setShowWeather(const int in);
  int showWeather() const { return m_globals.showWeather; }

  /**
   * Toggles whether the interface is shown. Called by
   * PauseLongOperation and related functors.
   */
  void toggleInterfaceHidden();
  bool interfaceHidden();

  /**
   * Returns the ObjectSettings from the Gameexe for objNum. The data
   * from this method should be used by all subclasses of
   * GraphicsSystem when deciding whether to render an object or not.
   */
  ObjectSettings getObjectSettings(const int objNum);
  /// @}

  /**
   * @name Screen refreshing
   *
   * @{
   */

  /**
   * Should be called by any of the drawing functions the screen is
   * invalidated.
   *
   * For more information, please see section 5.10.4 of the RLDev
   * manual, which deals with the behaviour of screen updates, and the
   * various modes.
   */
  virtual void markScreenAsDirty(GraphicsUpdateType type);

  /**
   * Forces a refresh of the screen the next time the graphics system
   * executes.
   */
  virtual void forceRefresh();

  bool screenNeedsRefresh() const { return m_screenNeedsRefresh; }

  void screenRefreshed() { m_screenNeedsRefresh = false; }
  /// @}

  virtual void beginFrame();
  virtual void refresh(RLMachine& machine) = 0;
  virtual void endFrame(RLMachine& machine);

  virtual boost::shared_ptr<Surface> renderToSurfaceWithBg(
    RLMachine& machine, boost::shared_ptr<Surface> bg);


  /**
   * Called from the game loop; Does everything that's needed to keep
   * things up.
   */
  virtual void executeGraphicsSystem(RLMachine& machine) = 0;

  /**
   * Returns the size of the window in pixels.
   */
  virtual Size screenSize() const = 0;

  /**
   * Returns a rectangle with an origin of (0,0) and a size returned by
   * screenSize().
   */
  Rect screenRect() const;

  virtual void allocateDC(int dc, Size size) = 0;
  virtual void freeDC(int dc) = 0;

  // ----------------------------------------- [ Surface loading functions ]
  virtual boost::shared_ptr<Surface> loadSurfaceFromFile(
	  const boost::filesystem::path& filename) = 0;

  virtual boost::shared_ptr<Surface> getDC(int dc) = 0;

  virtual boost::shared_ptr<Surface> buildSurface(const Size& size) = 0;

  // ----------------------------------- [ Object getter/factory functions ]
  /**
   * @name Graphics Object operations
   *
   * @{
   */

  /**
   * Takes the current background object in each slot and puts it in
   * the foreground slot.
   *
   * @todo Rewrite this to not needlessly copy the data.
   */
  void promoteObjects();

  /**
   * Clears and promotes objects.
   */
  void clearAndPromoteObjects();

  /**
   * Calls render() on all foreground objects that need to be
   * rendered.
   *
   * @param machine RLMachine context.
   */
  void renderObjects(RLMachine& machine);

  /**
   * Creates rendering data for a graphics object from a G00, PDT or ANM file.
   *
   * Does not deal with GAN files. Those are built with a separate function.
   */
  GraphicsObjectData* buildObjOfFile(RLMachine& machine,
                                     const std::string& filename);

  /// Object getters
  /// layer == 0 for fg, layer == 1 for bg.
  GraphicsObject& getObject(int layer, int objNumber);
  void setObject(int layer, int objNumber, GraphicsObject& object);

  /**
   * Deallocates all graphics objects.
   */
  void clearAllObjects();

  LazyArray<GraphicsObject>& backgroundObjects();
  LazyArray<GraphicsObject>& foregroundObjects();

  /**
   * Takes a snapshot of the current object state. This snapshot is saved
   * instead of the current state of the graphics, since RealLive is a savepoint
   * based system.
   *
   * (This operation isn't exceptionally expensive; internally GraphicsObject
   * has multiple copy-on-write data structs to make this and object promotion a
   * relativly cheap operation.)
   */
  void takeSavepointSnapshot();
  /// @}

  virtual void clearAllDCs() { }


  /**
   * @name Overridden from MouseMotionListener:
   *
   * @{
   */
  /// Listen to the mouse's location so we can possibly draw a cursor there.
  virtual void mouseMotion(const Point& newLocation);
  /// @}

  /**
   * Reset the system. Should clear all state for when a user loads a
   * game.
   */
  virtual void reset();

  /// Access to the cgtable for the cg* functions.
  CGMTable& cgTable() { return *cg_table_; }

  // boost::serialization forward declaration
  template<class Archive>
  void save(Archive & ar, const unsigned int file_version) const;

  // boost::serialization forward declaration
  template<class Archive>
  void load(Archive& ar, const unsigned int file_version);

  BOOST_SERIALIZATION_SPLIT_MEMBER()

protected:
  const Point& cursorPos() const { return m_cursorPos; }

  boost::shared_ptr<MouseCursor> currentCursor(RLMachine& machine);

private:
  /// Default grp name (used in grp* and rec* functions where filename
  /// is '???')
  std::string m_defaultGrpName;

  /// Default bgr name (used in bgr* functions where filename is
  /// '???')
  std::string m_defaultBgrName;

  /// Current screen update mode
  DCScreenUpdateMode m_screenUpdateMode;

  /// Flag set to redraw the screen NOW
  bool m_screenNeedsRefresh;

  /// Whether it is the Graphics system's responsibility to redraw the
  /// screen. Some LongOperations temporarily take this responsibility
  /// to implement pretty fades and wipes
  bool m_isResponsibleForUpdate;

  /// Whether we should try to append m_subtitle in the window
  /// titlebar
  bool m_displaySubtitle;

  /// cp932 encoded subtitle string
  std::string m_subtitle;

  /// Controls whether we render the interface (this can be
  /// temporarily toggled by the user at runtime)
  bool m_hideInterface;

  /// Mutable global data to be saved in the globals file
  GraphicsSystemGlobals m_globals;

  /// Immutable
  struct GraphicsObjectSettings;
  /// Immutable global data that's constructed from the Gameexe.ini file.
  boost::scoped_ptr<GraphicsObjectSettings> m_graphicsObjectSettings;

  struct GraphicsObjectImpl;
  boost::scoped_ptr<GraphicsObjectImpl> m_graphicsObjectImpl;

  /** Whether we should use a custom mouse cursor. Set while parsing
   * the Gameexe file, and then left unchanged. We only use a custom
   * mouse cursor if \#MOUSE_CURSOR is set in the Gameexe
   */
  bool m_useCustomMouseCursor;

  /// Whether we should render any cursor. Controller by the bytecode.
  bool m_showCurosr;

  /** Current cursor id. Initially set to \#MOUSE_CURSOR if the key
   * exists.
   */
  int m_cursor;

  /// Location of the cursor's hotspot
  Point m_cursorPos;

  /// Current mouse cursor
  boost::shared_ptr<MouseCursor> m_mouseCursor;

  /// MouseCursor construction is nontrivial so cache everything we
  /// build:
  typedef std::map<int, boost::shared_ptr<MouseCursor> > MouseCursorCache;
  MouseCursorCache m_cursorCache;

  /// CG Table
  boost::scoped_ptr<CGMTable> cg_table_;

  /// Our parent system object.
  System& m_system;
};

const int OBJECTS_IN_A_LAYER = 256;

#endif

