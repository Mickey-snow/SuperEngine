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
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
// -----------------------------------------------------------------------

#include <gtest/gtest.h>

#include "core/input.hpp"

#include <memory>

namespace {

template <typename T>
void Dispatch(InputListener& listener, T event) {
  listener.OnEvent(std::make_shared<Event>(event));
}

void ExpectNoEdges(const InputListener::State& state) {
  EXPECT_FALSE(state.on_down);
  EXPECT_FALSE(state.on_up);
  EXPECT_FALSE(state.down_up);
}

}  // namespace

TEST(InputListenerTest, DecideKeysStayDownAcrossFrames) {
  for (const auto code : {KeyCode::RETURN, KeyCode::SPACE, KeyCode::KP_ENTER}) {
    InputListener listener;

    Dispatch(listener, KeyDown{code});
    EXPECT_TRUE(listener.decide.down);
    EXPECT_TRUE(listener.decide.on_down);
    EXPECT_FALSE(listener.decide.on_up);
    EXPECT_FALSE(listener.decide.down_up);

    listener.ResetState();
    EXPECT_TRUE(listener.decide.down);
    ExpectNoEdges(listener.decide);

    Dispatch(listener, KeyUp{code});
    EXPECT_FALSE(listener.decide.down);
    EXPECT_FALSE(listener.decide.on_down);
    EXPECT_TRUE(listener.decide.on_up);
    EXPECT_TRUE(listener.decide.down_up);
  }
}

TEST(InputListenerTest, EscapeControlsCancelState) {
  InputListener listener;

  Dispatch(listener, KeyDown{KeyCode::ESCAPE});
  EXPECT_TRUE(listener.cancel.down);
  EXPECT_TRUE(listener.cancel.on_down);
  EXPECT_FALSE(listener.decide.down);

  listener.ResetState();
  Dispatch(listener, KeyUp{KeyCode::ESCAPE});
  EXPECT_FALSE(listener.cancel.down);
  EXPECT_TRUE(listener.cancel.on_up);
  EXPECT_TRUE(listener.cancel.down_up);
}

TEST(InputListenerTest, SameFrameCycleAccumulatesEdges) {
  InputListener listener;

  Dispatch(listener, KeyDown{KeyCode::RETURN});
  Dispatch(listener, KeyUp{KeyCode::RETURN});

  EXPECT_FALSE(listener.decide.down);
  EXPECT_TRUE(listener.decide.on_down);
  EXPECT_TRUE(listener.decide.on_up);
  EXPECT_TRUE(listener.decide.down_up);
}

TEST(InputListenerTest, RepeatedEventsDoNotCreateTransitions) {
  InputListener listener;

  Dispatch(listener, KeyUp{KeyCode::RETURN});
  ExpectNoEdges(listener.decide);

  Dispatch(listener, KeyDown{KeyCode::RETURN});
  listener.ResetState();
  Dispatch(listener, KeyDown{KeyCode::RETURN});
  EXPECT_TRUE(listener.decide.down);
  ExpectNoEdges(listener.decide);

  Dispatch(listener, KeyUp{KeyCode::RETURN});
  listener.ResetState();
  Dispatch(listener, KeyUp{KeyCode::RETURN});
  EXPECT_FALSE(listener.decide.down);
  ExpectNoEdges(listener.decide);
}

TEST(InputListenerTest, LogicalInputAggregatesKeyboardAndMouseSources) {
  InputListener listener;

  Dispatch(listener, KeyDown{KeyCode::RETURN});
  listener.ResetState();
  Dispatch(listener, MouseDown{MouseButton::LEFT});
  ExpectNoEdges(listener.decide);

  Dispatch(listener, KeyUp{KeyCode::RETURN});
  EXPECT_TRUE(listener.decide.down);
  ExpectNoEdges(listener.decide);

  Dispatch(listener, MouseUp{MouseButton::LEFT});
  EXPECT_FALSE(listener.decide.down);
  EXPECT_TRUE(listener.decide.on_up);
  EXPECT_TRUE(listener.decide.down_up);
}

TEST(InputListenerTest, MouseEventsUpdatePhysicalAndLogicalState) {
  InputListener listener;

  Dispatch(listener, MouseMotion{Point(23, 42)});
  EXPECT_EQ(listener.mouse_pos, Point(23, 42));

  Dispatch(listener, MouseDown{MouseButton::LEFT});
  EXPECT_TRUE(listener.left_mouse_down);
  EXPECT_TRUE(listener.decide.down);
  EXPECT_TRUE(listener.decide.on_down);

  Dispatch(listener, MouseDown{MouseButton::RIGHT});
  EXPECT_TRUE(listener.right_mouse_down);
  EXPECT_TRUE(listener.cancel.down);
  EXPECT_TRUE(listener.cancel.on_down);

  listener.ResetState();
  Dispatch(listener, MouseUp{MouseButton::LEFT});
  Dispatch(listener, MouseUp{MouseButton::RIGHT});
  EXPECT_FALSE(listener.left_mouse_down);
  EXPECT_FALSE(listener.right_mouse_down);
  EXPECT_TRUE(listener.decide.on_up);
  EXPECT_TRUE(listener.decide.down_up);
  EXPECT_TRUE(listener.cancel.on_up);
  EXPECT_TRUE(listener.cancel.down_up);
}

TEST(InputListenerTest, UnrelatedAndNullEventsAreIgnored) {
  InputListener listener;

  Dispatch(listener, KeyDown{KeyCode::a});
  Dispatch(listener, MouseDown{MouseButton::MIDDLE});
  listener.OnEvent(nullptr);

  EXPECT_FALSE(listener.decide.down);
  EXPECT_FALSE(listener.cancel.down);
  EXPECT_FALSE(listener.left_mouse_down);
  EXPECT_FALSE(listener.right_mouse_down);
  ExpectNoEdges(listener.decide);
  ExpectNoEdges(listener.cancel);
}
