// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2024 Serina Sakurai
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
//
// -----------------------------------------------------------------------

#include "object/parameter_manager.h"

const Rect EMPTY_RECT = Rect(Point(0, 0), Size(-1, -1));

namespace details {

template <size_t I>
void Set_one_impl(Scapegoat& bst) {
  using T = typename GetNthType<I, ObjectPropertyType>::type;
  bst.Set(I, T{});
}

template <size_t... I>
void Set_all_impl(Scapegoat& bst, std::index_sequence<I...> /*unused*/) {
  (Set_one_impl<I>(bst), ...);
}

static const Scapegoat init_param = []() -> Scapegoat {
  Scapegoat result;
  Set_all_impl(result, std::make_index_sequence<static_cast<size_t>(
                           ObjectProperty::TOTAL_COUNT)>{});

  result.Set(static_cast<int>(ObjectProperty::AlphaSource), 255);
  result.Set(static_cast<int>(ObjectProperty::HeightPercent), 100);
  result.Set(static_cast<int>(ObjectProperty::WidthPercent), 100);
  result.Set(static_cast<int>(ObjectProperty::HighQualityWidthPercent), 1000);
  result.Set(static_cast<int>(ObjectProperty::HighQualityHeightPercent), 1000);
  result.Set(static_cast<int>(ObjectProperty::ClippingRegion), EMPTY_RECT);
  result.Set(static_cast<int>(ObjectProperty::OwnSpaceClippingRegion),
             EMPTY_RECT);
  result.Set(static_cast<int>(ObjectProperty::AdjustmentAlphas),
             (std::array<int, 8>{255, 255, 255, 255, 255, 255, 255, 255}));

  return result;
}();
}  // namespace details

ParameterManager::ParameterManager() : bst_(details::init_param) {}
