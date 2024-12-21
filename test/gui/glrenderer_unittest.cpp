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

#include <SDL/SDL.h>
#include <glm/mat4x4.hpp>
#include <thread>

static const Size screen_size = Size(128, 128);

class glRendererTest : public ::testing::Test {
 protected:
  inline static std::shared_ptr<sdlEnv> sdl_handle = nullptr;
  static void SetUpTestSuite() {
    try {
      sdl_handle = SetupSDL(screen_size);
    } catch (std::exception& e) {
      GTEST_SKIP() << "Failed to setup sdl (testing): " << e.what();
    }
  }
  static void TearDownTestSuite() { sdl_handle = nullptr; }

  void SetUp() override {
    renderer.SetUp();
    texture = std::make_shared<glTexture>(screen_size);
    canvas = std::make_shared<glFrameBuffer>(texture);
  }

  glRenderer renderer;
  std::shared_ptr<glTexture> texture;
  std::shared_ptr<glFrameBuffer> canvas;

  // building a grid-like texture from a 2D color array
  std::shared_ptr<glTexture> CreateTestTexture(
      const std::vector<std::vector<RGBAColour>>& color_grid,
      Size cell) {
    const int cell_width = cell.width();
    const int cell_height = cell.height();
    if (color_grid.empty() || color_grid.front().empty()) {
      throw std::invalid_argument(
          "color_grid must be a non-empty 2D array of colors.");
    }

    int rows = static_cast<int>(color_grid.size());
    int cols = static_cast<int>(color_grid.front().size());

    Size texture_size(cols * cell_width, rows * cell_height);
    auto texture = std::make_shared<glTexture>(texture_size);

    for (int row = 0; row < rows; ++row) {
      for (int col = 0; col < cols; ++col) {
        const RGBAColour& c = color_grid[row][col];

        std::vector<uint8_t> cell_data;
        cell_data.reserve(cell_width * cell_height * 4);
        for (int y = 0; y < cell_height; ++y) {
          for (int x = 0; x < cell_width; ++x) {
            cell_data.push_back(c.r());
            cell_data.push_back(c.g());
            cell_data.push_back(c.b());
            cell_data.push_back(c.a());
          }
        }

        Rect cell_rect(Point(col * cell_width, row * cell_height),
                       Size(cell_width, cell_height));
        texture->Write(cell_rect, std::move(cell_data));
      }
    }

    return texture;
  }
};

TEST_F(glRendererTest, DISABLED_ClearBuffer) {
  const auto color = RGBAColour(20, 40, 60, 100);

  renderer.ClearBuffer(canvas, color);
  auto data = texture->Dump();
  for (size_t i = 0; i < data.size(); ++i) {
    EXPECT_EQ(data[i], color);
  }
}

TEST_F(glRendererTest, DISABLED_SubtractiveColorMask) {
  uint8_t data[] = {0,  255, 0,  0,  255, 255, 255, 255,   // NOLINT
                    10, 20,  30, 40, 255, 0,   0,   255};  // NOLINT
  auto masktex = std::make_shared<glTexture>(Size(2, 2), std::views::all(data));
  const auto maskcolor = RGBAColour(90, 60, 30, 120);

  renderer.ClearBuffer(canvas, RGBAColour(20, 40, 60, 100));
  const Rect dstrect(Point(0, 2), Size(2, 2));
  renderer.RenderColormask({masktex, Rect(Point(0, 0), Size(2, 2))},
                           {canvas, dstrect}, maskcolor);

  auto result = texture->Dump(dstrect);
  EXPECT_EQ(result[0], RGBAColour(20, 40, 60, 100));
  EXPECT_EQ(result[1], RGBAColour(17, 34, 52, 91));
  EXPECT_EQ(result[2], RGBAColour(16, 35, 54, 96));
  EXPECT_EQ(result[3], RGBAColour(17, 34, 52, 91));
}

TEST_F(glRendererTest, DISABLED_DrawColor) {
  auto texture_size = Size(12, 12);
  auto tex = CreateTestTexture(
      {{RGBAColour(0, 255, 0, 0), RGBAColour(255, 255, 255, 255)},
       {RGBAColour(10, 20, 30, 40), RGBAColour(255, 0, 0, 255)}},
      texture_size / 2);

  renderer.ClearBuffer(canvas, RGBAColour(0, 20, 80, 255));
  renderer.Render({tex, Rect(Point(0, 0), texture_size)},
                  {canvas, Rect(Point(32, 32), texture_size)});

  Rect topleft(Point(35, 35), Size(1, 1));
  Rect downleft(Point(35, 41), Size(1, 1));
  Rect topright(Point(41, 35), Size(1, 1));
  Rect downright(Point(41, 41), Size(1, 1));
  EXPECT_EQ(texture->Dump(topleft).front(), RGBAColour(0, 20, 80, 255));
  EXPECT_EQ(texture->Dump(topright).front(), RGBAColour(255, 255, 255, 255));
  EXPECT_EQ(texture->Dump(downleft).front(), RGBAColour(2, 20, 72, 221));
  EXPECT_EQ(texture->Dump(downright).front(), RGBAColour(255, 0, 0, 255));
}
