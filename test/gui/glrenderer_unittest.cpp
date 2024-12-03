// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2025 Serina Sakurai
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
// -----------------------------------------------------------------------

#include <gtest/gtest.h>

#include "test_system/sdl_env.hpp"

#include "base/rect.hpp"
#include "systems/gl_frame_buffer.hpp"
#include "systems/gl_utils.hpp"
#include "systems/glrenderer.hpp"
#include "systems/gltexture.hpp"

#include <glm/mat4x4.hpp>

static const Size screen_size = Size(8, 8);

class glRendererTest : public ::testing::Test {
 protected:
  inline static std::shared_ptr<sdlEnv> sdl_handle = nullptr;
  static void SetUpTestSuite() { sdl_handle = SetupSDL(); }

  void SetUp() override {
    renderer.SetUp();
    texture = std::make_shared<glTexture>(screen_size);
    canvas = std::make_shared<glFrameBuffer>(texture);
  }

  glRenderer renderer;
  std::shared_ptr<glTexture> texture;
  std::shared_ptr<glFrameBuffer> canvas;
};

TEST_F(glRendererTest, ClearBuffer) {
  const auto color = RGBAColour(20, 40, 60, 100);

  renderer.SetFrameBuffer(canvas);
  renderer.ClearBuffer(color);
  auto data = texture->Dump(Rect(Point(0, 0), screen_size));
  for (size_t i = 0; i < data.size(); ++i) {
    static std::vector<uint8_t> color_data{color.r(), color.g(), color.b(),
                                           color.a()};
    EXPECT_EQ(data[i], color_data[i % color_data.size()]);
  }
  std::cout << std::endl;
}
