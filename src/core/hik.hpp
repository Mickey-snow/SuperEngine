// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2009 Elliot Glaysher
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

#include <memory>
#include <string>
#include <vector>

#include "core/rect.hpp"
#include "utilities/clock.hpp"

class SDLSurface;

// Class that parses and executes HIK files.
class HIKScript {
 public:
  // The contents of the 40000 keys which define an individual frame.
  struct Frame {
    int opacity;
    std::string image;
    std::shared_ptr<const SDLSurface> surface;

    int grp_pattern;
    int frame_length_ms;
  };

  // The contents of the 30000 keys. I used to call this structure Unknowns;
  // "Animation" is a tentative name as it contains individual Frames that are
  // played in sequence.
  struct Animation {
    int use_multiframe_animation;

    // The number of frames as reported by the HIK file. Used for error
    // checking.
    int number_of_frames;

    // All frames to display.
    std::vector<Frame> frames;

    // IDEA: This is the animation number in the layer to move to next when
    // all frames in this animation are played out.
    int i_30101;
    // Unknown
    int i_30102;

    // The sum of all |frame_length_ms| in frames.
    int total_time;
  };

  // The contents of the 20000 keys.
  struct Layer {
    Point top_offset;

    bool use_scrolling;
    Point start_point;
    Point end_point;
    int x_scroll_time_ms;
    int y_scroll_time_ms;

    bool use_clip_area;
    Rect clip_area;

    // Number of unknowns as reported by the HIK file on disk.
    int number_of_animations;

    std::vector<Animation> animations;
  };

  HIKScript(std::vector<Layer> layers, int number_of_layers, Size size_of_hik);
  ~HIKScript();

  // Returns the HIK layer data.
  const std::vector<Layer>& layers() const { return layers_; }
  const Size& size() const { return size_of_hik_; }

 private:
  // Each graphics component in the HIK script.
  std::vector<Layer> layers_;

  // The number of layers as reported by the HIK file. Used for error checking.
  int number_of_layers_;

  // Size of the hik graphic as reported by the hik.
  Size size_of_hik_;
};

// Displays a HIKScript at a certain time to the screen.
class HIKRenderer {
 public:
  HIKRenderer(std::shared_ptr<Clock> clock,
              const std::shared_ptr<const HIKScript>& script);
  ~HIKRenderer();

  void Render();

  // Advances to the next layer.
  void NextAnimationFrame();

  // RL bytecode controlled offsets from the top left corner of the source
  // image.
  inline void set_x_offset(int offset) { x_offset_ = offset; }
  inline void set_y_offset(int offset) { y_offset_ = offset; }

 private:
  std::shared_ptr<Clock> clock_;

  // The script data.
  std::shared_ptr<const HIKScript> script_;

  // Time when this HIK renderer was loaded. Used for animation timing.
  Clock::timepoint_t creation_time_;

  // Bytecode controllable offset.
  int x_offset_;
  int y_offset_;

  struct LayerData {
    explicit LayerData(Clock::timepoint_t time);
    int animation_num_;
    Clock::timepoint_t animation_start_time_;
  };

  // Which animation frame to use per layer. Defaults to zero.
  std::vector<LayerData> layer_to_animation_num_;
};
