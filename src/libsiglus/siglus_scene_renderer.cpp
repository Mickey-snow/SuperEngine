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

#include "libsiglus/siglus_scene_renderer.hpp"

#include "core/object.hpp"
#include "core/stage.hpp"
#include "libsiglus/bindings/wipe.hpp"
#include "systems/graphics_system.hpp"
#include "systems/system.hpp"
#include "systems/text_system.hpp"

#include <algorithm>
#include <cmath>

namespace libsiglus {

namespace {

class ScopedAlpha {
 public:
  ScopedAlpha(GraphicsObject& object, double multiplier)
      : object_(object), original_alpha_(object.Param().raw_alpha()) {
    const int alpha = std::clamp(
        static_cast<int>(std::lround(original_alpha_ * multiplier)), 0, 255);
    object_.Param().SetAlpha(alpha);
  }

  ~ScopedAlpha() { object_.Param().SetAlpha(original_alpha_); }

 private:
  GraphicsObject& object_;
  int original_alpha_;
};

}  // namespace

SiglusSceneRenderer::SiglusSceneRenderer(Stage& stage, System& system)
    : stage_(stage), system_(system) {}

void SiglusSceneRenderer::SetWipe(binding::SiglusWipe* wipe) { wipe_ = wipe; }

void SiglusSceneRenderer::ExecuteFrame() { stage_.Execute(); }

void SiglusSceneRenderer::RenderScene() {
  if (wipe_)
    wipe_->Update();

  if (wipe_ && wipe_->IsActive())
    RenderWipeObjects(stage_, wipe_->Progress(), to_render_);
  else
    RenderForegroundObjects(stage_, to_render_);

  if (!system_.graphics().is_interface_hidden())
    system_.text().Render();
}

void SiglusSceneRenderer::RenderForegroundObjects(Stage& stage,
                                                  ToRenderVec& to_render) {
  to_render.clear();
  QueueObjects(stage.foreground_objects, 0, 1.0, to_render);
  RenderQueuedObjects(to_render);
}

void SiglusSceneRenderer::RenderWipeObjects(Stage& stage,
                                            double progress,
                                            ToRenderVec& to_render) {
  progress = std::clamp(progress, 0.0, 1.0);

  to_render.clear();
  QueueObjects(stage.next_objects, 0, 1.0 - progress, to_render);
  QueueObjects(stage.foreground_objects, 1, progress, to_render);
  RenderQueuedObjects(to_render);
}

void SiglusSceneRenderer::QueueObjects(LazyArray<GraphicsObject>& objects,
                                       int source_order,
                                       double alpha_multiplier,
                                       ToRenderVec& to_render) {
  for (auto it = objects.begin(), end = objects.end(); it != end; ++it) {
    to_render.emplace_back(it->Param().z_order, it->Param().z_layer,
                           it->Param().z_depth, static_cast<int>(it.pos()),
                           source_order, &*it, alpha_multiplier);
  }
}

void SiglusSceneRenderer::RenderQueuedObjects(ToRenderVec& to_render) {
  std::sort(to_render.begin(), to_render.end());

  for (auto& object : to_render) {
    const double alpha_multiplier = std::get<6>(object);
    if (alpha_multiplier <= 0.0)
      continue;

    GraphicsObject& graphics_object = *std::get<5>(object);
    ScopedAlpha alpha(graphics_object, alpha_multiplier);
    graphics_object.Render(std::get<3>(object), nullptr);
  }
}

}  // namespace libsiglus
