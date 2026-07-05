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

#include <gtest/gtest.h>

#include "core/object_internal/drawer/file.hpp"

#include <stdexcept>
#include <vector>

TEST(SiglusCompositeObjectTest, DetectsCompositeNames) {
  EXPECT_TRUE(IsCompositeObjectName("base | face"));
  EXPECT_TRUE(IsCompositeObjectName("||"));
  EXPECT_FALSE(IsCompositeObjectName("base"));
}

TEST(SiglusCompositeObjectTest, ParsesSimpleCompositeAndRemovesSpaces) {
  std::vector<CompositeObjectPart> parts =
      ParseCompositeObjectName("bs2_jd01_base01 | bs2_jd_f01_03");

  ASSERT_EQ(parts.size(), 2);
  EXPECT_EQ(parts[0].file_name, "bs2_jd01_base01");
  EXPECT_EQ(parts[0].x, 0);
  EXPECT_EQ(parts[0].y, 0);
  EXPECT_EQ(parts[0].cut_no, 0);
  EXPECT_EQ(parts[0].blend_type, 0);
  EXPECT_EQ(parts[1].file_name, "bs2_jd_f01_03");
  EXPECT_EQ(parts[1].x, 0);
  EXPECT_EQ(parts[1].y, 0);
  EXPECT_EQ(parts[1].cut_no, 0);
  EXPECT_EQ(parts[1].blend_type, 0);
}

TEST(SiglusCompositeObjectTest, ParsesOffsetsCutAndBlendZero) {
  std::vector<CompositeObjectPart> parts =
      ParseCompositeObjectName("base(10,-2)|face(3,4,1)|mask(0,0,blend=0)");

  ASSERT_EQ(parts.size(), 3);
  EXPECT_EQ(parts[0].file_name, "base");
  EXPECT_EQ(parts[0].x, 10);
  EXPECT_EQ(parts[0].y, -2);
  EXPECT_EQ(parts[0].cut_no, 0);
  EXPECT_EQ(parts[1].file_name, "face");
  EXPECT_EQ(parts[1].x, 3);
  EXPECT_EQ(parts[1].y, 4);
  EXPECT_EQ(parts[1].cut_no, 1);
  EXPECT_EQ(parts[2].file_name, "mask");
  EXPECT_EQ(parts[2].blend_type, 0);
}

TEST(SiglusCompositeObjectTest, RejectsMalformedLayers) {
  EXPECT_THROW(ParseCompositeObjectName("base|"), std::runtime_error);
  EXPECT_THROW(ParseCompositeObjectName("base||face"), std::runtime_error);
  EXPECT_THROW(ParseCompositeObjectName("base(1)|face"), std::runtime_error);
  EXPECT_THROW(ParseCompositeObjectName("base(1,2"), std::runtime_error);
}

TEST(SiglusCompositeObjectTest, RejectsUnsupportedNonzeroBlend) {
  EXPECT_THROW(ParseCompositeObjectName("base|face(0,0,blend=1)"),
               std::runtime_error);
}
