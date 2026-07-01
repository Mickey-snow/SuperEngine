// -----------------------------------------------------------------------
//
// This file is part of RLVM
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2026 Serina Sakurai
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

#include "systems/scene_renderer.hpp"

#include <tuple>
#include <vector>

class GraphicsObject;
class Stage;
class System;

template <typename T>
class LazyArray;

namespace libsiglus {

class SiglusSceneRendererTest;

class SiglusSceneRenderer final : public ISceneRenderer {
 public:
  SiglusSceneRenderer(::Stage& stage, ::System& system);

  void ExecuteFrame() override;
  void RenderScene() override;

 private:
  friend class SiglusSceneRendererTest;

  using ToRenderVec =
      std::vector<std::tuple<int, int, int, int, int, GraphicsObject*, double>>;

  static void RenderStageObjects(::Stage& stage, ToRenderVec& to_render);
  static void QueueObjects(LazyArray<GraphicsObject>& objects,
                           int source_order,
                           double alpha_multiplier,
                           ToRenderVec& to_render);
  static void RenderQueuedObjects(ToRenderVec& to_render);

  ::Stage& stage_;
  ::System& system_;
  ToRenderVec to_render_;
};

}  // namespace libsiglus
