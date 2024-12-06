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
  static void SetUpTestSuite() { sdl_handle = SetupSDL(screen_size); }
  static void TearDownTestSuite() { sdl_handle = nullptr; }

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
  auto data = texture->Dump();
  for (size_t i = 0; i < data.size(); ++i) {
    EXPECT_EQ(data[i], color);
  }
}

TEST_F(glRendererTest, SubtractiveColorMask) {
  GTEST_SKIP();
  uint8_t data[] = {0,  255, 0,  0,  255, 255, 255, 255,   // NOLINT
                    10, 20,  30, 40, 255, 0,   0,   255};  // NOLINT
  auto masktex = std::make_shared<glTexture>(Size(2, 2), data);
  const auto maskcolor = RGBAColour(90, 60, 30, 120);

  renderer.SetFrameBuffer(canvas);
  renderer.ClearBuffer(RGBAColour(20, 40, 60, 100));
  const Rect dstrect(Point(0, 2), Size(4, 4));
  renderer.RenderColormask({masktex, Rect(Point(0, 0), Size(2, 2))}, dstrect,
                           maskcolor);

  // auto result = texture->Dump();
  // for (auto it : result) {
  //   std::cout << it << ' ';
  // }
  // std::cout << std::endl;
}

struct ScreenCanvas : public glFrameBuffer {
 public:
  ScreenCanvas(Size size) : size_(size) {}

  virtual unsigned int GetID() const override { return 0; }
  virtual Size GetSize() const override { return size_; }

  Size size_;
};

TEST_F(glRendererTest, DrawColor) {
  uint8_t col_data[] = {0,  255, 0,  0,  255, 255, 255, 255,   // NOLINT
                        10, 20,  30, 40, 255, 0,   0,   255};  // NOLINT

  auto texture_size = Size(12, 12);
  auto tex = std::make_shared<glTexture>(texture_size);

  std::vector<uint8_t> data;
  for (int i = 0; i < texture_size.width() * texture_size.height() / 4; ++i)
    for (int j = 0; j < 4; ++j)
      data.push_back(col_data[j]);
  tex->Write(Rect(Point(0, 0), texture_size / 2), std::views::all(data));
  data.clear();
  for (int i = 0; i < texture_size.width() * texture_size.height() / 4; ++i)
    for (int j = 4; j < 8; ++j)
      data.push_back(col_data[j]);
  tex->Write(Rect(Point(texture_size.width() / 2, 0), texture_size / 2),
             std::views::all(data));
  data.clear();
  for (int i = 0; i < texture_size.width() * texture_size.height() / 4; ++i)
    for (int j = 8; j < 12; ++j)
      data.push_back(col_data[j]);
  tex->Write(Rect(Point(0, texture_size.height() / 2), texture_size / 2),
             std::views::all(data));
  data.clear();
  for (int i = 0; i < texture_size.width() * texture_size.height() / 4; ++i)
    for (int j = 12; j < 16; ++j)
      data.push_back(col_data[j]);
  tex->Write(Rect(Point(texture_size.width() / 2, texture_size.height() / 2),
                  texture_size / 2),
             std::views::all(data));

  renderer.SetFrameBuffer(canvas);
  renderer.ClearBuffer(RGBAColour(0, 20, 80, 255));
  RenderingConfig config;
  config.dst = Rect(Point(32, 32), texture_size);
  renderer.Render({tex, Rect(Point(0, 0), texture_size)}, config);

  Rect topleft(Point(35, 35), Size(1, 1));
  Rect downleft(Point(35, 41), Size(1, 1));
  Rect topright(Point(41, 35), Size(1, 1));
  Rect downright(Point(41, 41), Size(1, 1));
  EXPECT_EQ(texture->Dump(topleft).front(), RGBAColour(0, 20, 80, 255));
  EXPECT_EQ(texture->Dump(topright).front(), RGBAColour(255, 255, 255, 255));
  EXPECT_EQ(texture->Dump(downleft).front(), RGBAColour(2, 20, 72, 221));
  EXPECT_EQ(texture->Dump(downright).front(), RGBAColour(255, 0, 0, 255));
}
