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

#ifndef SRC_LIBSIGLUS_SCENE_HPP_
#define SRC_LIBSIGLUS_SCENE_HPP_

#include "base/compression.h"
#include "libsiglus/xorkey.hpp"
#include "utilities/byte_reader.h"

#include <cstdint>
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

  int32_t scnprop_offset;
  int32_t scnprop_cnt;
  int32_t scnprop_nameidx_offset;
  int32_t scnprop_nameidx_cnt;
  int32_t scnprop_name_offset;
  int32_t scnprop_name_cnt;

  int32_t cmd_offset;
  int32_t cmd_cnt;
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
  Scene(std::string data) : data_(std::move(data)) {}

  std::string data_;
};
}  // namespace libsiglus

#endif
