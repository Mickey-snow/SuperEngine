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

#include <gtest/gtest.h>

#include "libsiglus/element_builder.hpp"
#include "utilities/mpl.hpp"

namespace siglus_test {
using namespace libsiglus::elm;
using namespace libsiglus;

class AccessChainBuilderTest : public ::testing::Test {
 protected:
  struct ChainCtx {
    AccessChain chain;

    bool operator==(std::string rhs) const {
      return chain.ToDebugString() == rhs;
    }

    friend std::ostream& operator<<(std::ostream& os, const ChainCtx& ctx) {
      return os << ctx.chain.ToDebugString();
    }
  };

  template <typename... Ts>
    requires(std::same_as<Ts, int> && ...)
  ChainCtx chain(Ts&&... elms) {
    ElementCode elmcode{std::forward<Ts>(elms)...};
    return ChainCtx{.chain = elm::MakeChain(elmcode)};
  }

  template <typename... Ts>
    requires(std::same_as<Ts, int> && ...)
  ChainCtx chain(BindCtx bind, Ts&&... elms) {
    ElementCode elmcode{std::forward<Ts>(elms)...};
    return ChainCtx{.chain = elm::MakeChain(elmcode, std::move(bind))};
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

TEST_F(AccessChainBuilderTest, MemoryBank) {
  EXPECT_EQ(chain(25, -1, 0), "A[int:0]");
  EXPECT_EQ(chain(26, 3, -1, 1), "B.b1[int:1]");
  EXPECT_EQ(chain(27, 4, -1, 2), "C.b2[int:2]");
  EXPECT_EQ(chain(28, 5, -1, 3), "D.b4[int:3]");
  EXPECT_EQ(chain(29, 7, -1, 4), "E.b8[int:4]");
  EXPECT_EQ(chain(30, 6, -1, 5), "F.b16[int:5]");
  EXPECT_EQ(chain(31, -1, 250), "G[int:250]");
  EXPECT_EQ(chain(32, -1, 251), "Z[int:251]");
}

TEST_F(AccessChainBuilderTest, Farcall) {
  EXPECT_EQ(chain(BindCtx(0, {v("scnname")}), 5), "farcall@scnname.z0()()");
  EXPECT_EQ(chain(BindCtx(1, {v("name"), v(1), v(2), v("3"), v(4)}), 5),
            "farcall@name.z1(int:2,int:4)(str:3)");
}

TEST_F(AccessChainBuilderTest, TimeWait) {
  EXPECT_EQ(chain(BindCtx(0, {v(123)}), 54), "wait(123)");
  EXPECT_EQ(chain(BindCtx(0, {v(456)}), 55), "wait_key(456)");
}

}  // namespace siglus_test
