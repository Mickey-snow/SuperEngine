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
  Scene(std::string data) : data_(std::move(data)) {
    hdr_ = reinterpret_cast<Scene_hdr const*>(data_.data());
    std::string_view sv = data_;

    scene_ = sv.substr(hdr_->scene_offset, hdr_->scene_size);
    stridx_ = sv.substr(hdr_->str_idxlist_offset, hdr_->str_idxlist_size);
    str_ = sv.substr(hdr_->str_list_offset, hdr_->str_list_size);

    labellist_ = sv.substr(hdr_->label_list_offset, 4 * hdr_->label_cnt);
    zlabellist_ = sv.substr(hdr_->zlabel_list_offset, 4 * hdr_->zlabel_cnt);
    cmdlabellist_ =
        sv.substr(hdr_->cmdlabel_list_offset, 8 * hdr_->cmdlabel_cnt);

    scnprop_ = sv.substr(hdr_->scnprop_offset, hdr_->scnprop_cnt);
    scnprop_name_ =
        sv.substr(hdr_->scnprop_name_offset, hdr_->scnprop_name_cnt);
    scnprop_nameidx_ =
        sv.substr(hdr_->scnprop_nameidx_offset, hdr_->scnprop_nameidx_cnt);

    namae_ = sv.substr(hdr_->namae_offset, hdr_->namae_cnt);
    kidoku_ = sv.substr(hdr_->kidoku_offset, hdr_->kidoku_cnt);
  }

  std::string data_;
  Scene_hdr const* hdr_;

  std::string_view scene_;
  std::string_view stridx_, str_;
  std::string_view labellist_, label_, zlabellist_, zlabel_, cmdlabellist_,
      cmdlabel_;
  std::string_view scnprop_, scnprop_nameidx_, scnprop_name_;
  std::string_view cmd_, cmdname_;
  std::string_view callnameidx_, callname_;
  std::string_view namae_;
  std::string_view kidoku_;
};
}  // namespace libsiglus

#endif
