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

#ifndef SRC_OBJECT_PARAMETER_MANAGER_H_
#define SRC_OBJECT_PARAMETER_MANAGER_H_

#include "object/bst.h"
#include "object/properties.h"

class ParameterManager {
 public:
  ParameterManager() = default;
  ~ParameterManager() = default;

  void Set(ObjectProperty property, std::any value) {
    bst_.Set(static_cast<int>(property), std::move(value));
  }

  template <ObjectProperty property>
  auto Get() {
    constexpr int param_id = static_cast<int>(property);
    if constexpr (param_id < 0) {
      static_assert(false, "Invalid parameter ID");
    }

    using result_t = typename GetNthType<static_cast<size_t>(property),
                                         ObjectPropertyType>::type;
    try {
      return std::any_cast<result_t>(bst_.Get(param_id));
    } catch (std::bad_any_cast& e) {
      throw std::logic_error("ParameterManager: Parameter type mismatch (" +
                             std::to_string(param_id) + ")\n" + e.what());
    }
  }

  std::any Get(ObjectProperty property) {
    return bst_.Get(static_cast<int>(property));
  }
 private:
  Scapegoat bst_;
};

#endif
