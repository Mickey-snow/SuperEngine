// -----------------------------------------------------------------------
//
// This file is part of RLVM
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
// -----------------------------------------------------------------------

#pragma once

#include "libsiglus/property.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace libsiglus {

struct Scene_hdr {
  int32_t header_size;

  int32_t scene_offset;
  int32_t scene_size;

  int32_t str_idxlist_offset;
  int32_t str_idxlist_size;
  int32_t str_list_offset;
  int32_t str_list_size;

  int32_t label_list_offset;
  int32_t label_cnt;
  int32_t zlabel_list_offset;
  int32_t zlabel_cnt;
  int32_t cmdlabel_list_offset;
  int32_t cmdlabel_cnt;

  int32_t prop_offset;
  int32_t prop_cnt;
  int32_t prop_nameidx_offset;
  int32_t prop_nameidx_cnt;
  int32_t prop_name_offset;
  int32_t prop_name_cnt;

  int32_t cmdlist_offset;
  int32_t cmd_cnt;
  int32_t cmd_nameidx_offset;
  int32_t cmd_nameidx_cnt;
  int32_t cmd_name_offset;
  int32_t cmd_name_cnt;

  int32_t call_nameidx_offset;
  int32_t call_nameidx_cnt;
  int32_t call_name_offset;
  int32_t call_name_cnt;

  int32_t namae_offset;
  int32_t namae_cnt;

  int32_t kidoku_offset;
  int32_t kidoku_cnt;
};

class Scene {
 public:
  Scene(std::string data, int id = -1, std::string name = "???");

  int id_;
  std::string scnname_;

  std::string data_;
  Scene_hdr const* hdr_;

  std::string_view scene_;

  std::vector<std::string> str_;

  std::vector<int> label;
  std::vector<int> zlabel;
  struct CmdLabel {
    int32_t cmd_id;
    int32_t offset;
  };
  std::vector<CmdLabel> cmdlabel;

  std::vector<Property> property;
  std::map<std::string, int> property_map;

  std::vector<Command> cmd;
  std::map<std::string, int> cmd_map;

  std::vector<std::string> callproperty;
  std::vector<int> namae;   // index to string list
  std::vector<int> kidoku;  // line_no

  std::string GetDebugTitle() const;
};

}  // namespace libsiglus
