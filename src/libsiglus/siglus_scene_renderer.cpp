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

#include "core/stage.hpp"
#include "libsiglus/bindings/wipe.hpp"
#include "systems/graphics_system.hpp"
#include "systems/system.hpp"
#include "systems/text_system.hpp"

namespace libsiglus {

SiglusSceneRenderer::SiglusSceneRenderer(Stage& stage, System& system)
    : stage_(stage), system_(system) {}

void SiglusSceneRenderer::SetWipe(binding::SiglusWipe* wipe) { wipe_ = wipe; }

void SiglusSceneRenderer::ExecuteFrame() { stage_.Execute(); }

void SiglusSceneRenderer::RenderScene() {
  if (wipe_ && wipe_->UpdateAndRender())
    return;

  stage_.RenderObjects(nullptr);
  if (!system_.graphics().is_interface_hidden())
    system_.text().Render();
}

}  // namespace libsiglus
