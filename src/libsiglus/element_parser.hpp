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

#pragma once

#include "libsiglus/element.hpp"
#include "libsiglus/property.hpp"
#include "libsiglus/value.hpp"

#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace libsiglus::elm {

class ElementParser {
 public:
  ElementParser(std::span<const Property> scene_properties,
                std::span<const Property> global_properties,
                std::span<const Command> scene_commands,
                std::span<const Command> global_commands,
                const std::vector<Type>& curcall_args,
                int scene_id,
                std::function<int()> read_kidoku_callback,
                std::function<void(std::string)> warn_callback);
  ~ElementParser();

  AccessChain Parse(ElementCode& elm);

 private:
  AccessChain resolve_usrcmd(ElementCode& elm, size_t idx);
  AccessChain resolve_usrprop(ElementCode& elm, size_t idx);
  AccessChain resolve_element(ElementCode& elm);

  AccessChain make_chain(AccessChain result,
                         ElementCode& elm,
                         std::span<const Value> elmcode);
  AccessChain make_chain(Type root_type,
                         Root::var_t root_node,
                         ElementCode& elm,
                         size_t subidx);
  AccessChain make_sym_chain(Type type,
                             std::string_view top_id,
                             ElementCode& elm,
                             size_t subidx);
  AccessChain make_stage_member_chain(std::string_view member,
                                      ElementCode& elm,
                                      size_t subidx);

 private:
  std::span<const Property> scene_properties_;
  std::span<const Property> global_properties_;
  std::span<const Command> scene_commands_;
  std::span<const Command> global_commands_;
  const std::vector<Type>& curcall_args_;
  int scene_id_;
  std::function<int()> read_kidoku_;
  std::function<void(std::string)> warn_;
};

}  // namespace libsiglus::elm
