// -----------------------------------------------------------------------
//
// This file is part of RLVM
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
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
// -----------------------------------------------------------------------

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "libsiglus/element.hpp"
#include "libsiglus/element_parser.hpp"
#include "utilities/mpl.hpp"

namespace siglus_test {
using namespace libsiglus::elm;
using namespace libsiglus;

using ::testing::ReturnRef;

class ElementParserTest : public ::testing::Test {
 protected:
  class MockContext : public ElementParser::Context {
   public:
    MOCK_METHOD(const std::vector<Property>&,
                SceneProperties,
                (),
                (const, override));
    MOCK_METHOD(const std::vector<Property>&,
                GlobalProperties,
                (),
                (const, override));
    MOCK_METHOD(const std::vector<Command>&,
                SceneCommands,
                (),
                (const, override));
    MOCK_METHOD(const std::vector<Command>&,
                GlobalCommands,
                (),
                (const, override));
    MOCK_METHOD(const std::vector<Type>&, CurcallArgs, (), (const, override));
    MOCK_METHOD(int, ReadKidoku, (), (override));
    MOCK_METHOD(int, SceneId, (), (const, override));

    void Warn(std::string message) override { ADD_FAILURE() << message; }
  };
  ElementParserTest() {
    std::unique_ptr<MockContext> c = std::make_unique<MockContext>();
    ctx = c.get();
    parser = std::make_unique<ElementParser>(std::move(c));
  }
  MockContext* ctx;
  std::unique_ptr<ElementParser> parser;

  // ==============================================================================
  // Test helpers
  struct ChainCtx {
    AccessChain chain;

    bool operator==(std::string rhs) const {
      return chain.ToDebugString() == rhs;
    }

    friend std::ostream& operator<<(std::ostream& os, const ChainCtx& ctx) {
      return os << ctx.chain.ToDebugString();
    }
  };

  ChainCtx chain(ElementCode elm) {
    return ChainCtx{.chain = parser->Parse(elm)};
  }
  template <typename... Ts>
    requires(std::same_as<Ts, int> && ...)
  ChainCtx chain(Ts&&... elms) {
    ElementCode elmcode{std::forward<Ts>(elms)...};
    return ChainCtx{.chain = parser->Parse(elmcode)};
  }

  template <typename T>
  inline static Value v(T param) {
    if constexpr (std::same_as<T, int>)
      return Value(Integer(param));
    else if constexpr (std::constructible_from<std::string, T>)
      return Value(String(std::move(param)));
    else
      static_assert(always_false<T>);
  }
};

TEST_F(ElementParserTest, MemoryBank) {
  EXPECT_EQ(chain(25, -1, 0), "A[int:0]");
  EXPECT_EQ(chain(26, 3, -1, 1), "B.b1[int:1]");
  EXPECT_EQ(chain(27, 4, -1, 2), "C.b2[int:2]");
  EXPECT_EQ(chain(28, 5, -1, 3), "D.b4[int:3]");
  EXPECT_EQ(chain(29, 7, -1, 4), "E.b8[int:4]");
  EXPECT_EQ(chain(30, 6, -1, 5), "F.b16[int:5]");
  EXPECT_EQ(chain(31, -1, 250), "G[int:250]");
  EXPECT_EQ(chain(32, -1, 251), "Z[int:251]");
}

TEST_F(ElementParserTest, Farcall) {
  {
    ElementCode elm{5};
    elm.ForceBind({0, {v("scnname")}});
    EXPECT_EQ(chain(elm), "farcall@[str:scnname].z[int:0]()()");
  }
  {
    ElementCode elm{5};
    elm.ForceBind({1, {v("name"), v(1), v(2), v("3"), v(4)}});
    EXPECT_EQ(chain(elm), "farcall@[str:name].z[int:1](int:2,int:4)(str:3)");
  }
  {
    // dynamic farcall
    ElementCode elm{5};
    elm.ForceBind({1,
                   {Value(Variable(Type::String, 123)),
                    Value(Variable(Type::Int, 456))}});
    EXPECT_EQ(chain(elm), "farcall@[v123].z[v456]()()");
  }
}

TEST_F(ElementParserTest, TimeWait) {
  {
    ElementCode elm{54};
    elm.ForceBind({0, {v(123)}});
    EXPECT_EQ(chain(elm), "wait(int:123)");
  }
  {
    ElementCode elm{55};
    elm.ForceBind({0, {v(456)}});
    EXPECT_EQ(chain(elm), "wait_key(int:456)");
  }
  {
    ElementCode elm{55};
    elm.ForceBind({0, {Value(Variable(Type::Int, 456))}});
    EXPECT_EQ(chain(elm), "wait_key(v456)");
  }
}

TEST_F(ElementParserTest, Title) {
  {
    ElementCode elm{74};
    elm.ForceBind({0, {v("title")}});
    EXPECT_EQ(chain(elm), ".set_title(str:title)");
  }
  {
    ElementCode elm{75};
    EXPECT_EQ(chain(elm), ".get_title()");
  }
}

TEST_F(ElementParserTest, FrameAction) {
  {
    ElementCode elm{53, -1, 8, 1};
    elm.ForceBind({0, {v(-1), v("$$command_name")}});
    EXPECT_EQ(chain(elm),
              "frame_action_ch[int:8].start(int:-1,str:$$command_name)");
  }
}

TEST_F(ElementParserTest, CurcallArgStr) {
  std::vector<Type> curcall_args{Type::None, Type::String};
  EXPECT_CALL(*ctx, CurcallArgs()).WillOnce(ReturnRef(curcall_args));

  int flag = 0x7d << 24;
  int idx = 1;
  ElementCode elm{83, (flag | idx), 2};
  EXPECT_EQ(chain(elm), "arg_1.left()");
}

TEST_F(ElementParserTest, Movie) {
  {
    ElementCode elm{20, 2};
    elm.ForceBind({0, {v("mov1")}});
    EXPECT_EQ(chain(elm), "mov.play_wait(str:mov1)");
  }
  {
    ElementCode elm{20, 3};
    elm.ForceBind({1, {v("mov2"), v(0), v(0), v(420), v(420)}});
    EXPECT_EQ(chain(elm),
              "mov.play_waitkey(str:mov2,int:0,int:0,int:420,int:420)");
  }
}

TEST_F(ElementParserTest, BgmTable) {
  {
    ElementCode elm{123, 2};
    elm.ForceBind({0, {v("song01"), v(1)}});
    EXPECT_EQ(chain(elm), "bgm_table.set_listen(str:song01,int:1)");
  }
}

TEST_F(ElementParserTest, Bgm) {
  {
    ElementCode elm{42, 0};
    elm.ForceBind({0, {v("song02"), v(1), v(2)}});
    EXPECT_EQ(chain(elm), "bgm.play(str:song02,int:1,int:2)");
  }
  {
    ElementCode elm{42, 4};
    elm.ForceBind({1, {v(4000)}});
    EXPECT_EQ(chain(elm), "bgm.stop(int:4000)");
  }
}

TEST_F(ElementParserTest, Mwnd) {
  {
    ElementCode elm{10};
    EXPECT_EQ(chain(elm), "mwnd.close()");
  }
  {
    ElementCode elm{115};
    EXPECT_EQ(chain(elm), "mwnd.page()");
  }
  {
    ElementCode elm{84};
    EXPECT_EQ(chain(elm), "mwnd.msg_block()");
  }
  {
    ElementCode elm{14};
    EXPECT_EQ(chain(elm), "mwnd.r()");
  }
  {
    ElementCode elm{119};
    EXPECT_EQ(chain(elm), "mwnd.indent()");
  }
}

TEST_F(ElementParserTest, System) {
  {
    ElementCode elm{92, 13};
    EXPECT_EQ(chain(elm), "system.is_debug()");
  }
  {
    ElementCode elm{92, 7};
    elm.ForceBind({1, {v("msg")}});
    EXPECT_EQ(chain(elm), "system.debug_msgbox_ok(str:msg)");
  }
  {
    ElementCode elm{92, 6};
    elm.ForceBind({0, {v("file")}});
    EXPECT_EQ(chain(elm), "system.check_file_exist(str:file)");
  }
  {
    ElementCode elm{92, 2};
    elm.ForceBind({0, {v("dummy"), v(123), v("key")}});
    EXPECT_EQ(chain(elm), "system.check_dummy(str:dummy,int:123,str:key)");
  }
}

TEST_F(ElementParserTest, UsrcmdGlobal) {
  std::vector<Command> globalcmd = {
      Command{.scene_id = 1, .offset = 2, .name = "$$cmd"}};
  EXPECT_CALL(*ctx, GlobalCommands()).WillRepeatedly(ReturnRef(globalcmd));

  ElementCode elm{2113929216};
  elm.ForceBind({0, {v(20)}});
  EXPECT_EQ(chain(elm), "@1.2:$$cmd(int:20)");
}

}  // namespace siglus_test
