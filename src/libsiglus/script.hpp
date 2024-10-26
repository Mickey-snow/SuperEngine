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

#ifndef SRC_LIBSIGLUS_SCRIPT_HPP_
#define SRC_LIBSIGLUS_SCRIPT_HPP_

#include "base/compression.h"
#include "libsiglus/scene.hpp"
#include "libsiglus/xorkey.hpp"
#include "utilities/byte_reader.h"

#include <cstdint>
#include <string_view>
#include <vector>

namespace libsiglus {
struct PackedScene_hdr {
  int32_t header_size;
  int32_t inc_prop_list_ofs;
  int32_t inc_prop_cnt;
  int32_t inc_prop_name_index_list_ofs;
  int32_t inc_prop_name_index_cnt;
  int32_t inc_prop_name_list_ofs;
  int32_t inc_prop_name_cnt;
  int32_t inc_cmd_list_ofs;
  int32_t inc_cmd_cnt;
  int32_t inc_cmd_name_index_list_ofs;
  int32_t inc_cmd_name_index_cnt;
  int32_t inc_cmd_name_list_ofs;
  int32_t inc_cmd_name_cnt;
  int32_t scn_name_index_list_ofs;
  int32_t scn_name_index_cnt;
  int32_t scn_name_list_ofs;
  int32_t scn_name_cnt;
  int32_t scn_data_index_list_ofs;
  int32_t scn_data_index_cnt;
  int32_t scn_data_list_ofs;
  int32_t scn_data_cnt;
  int32_t scn_data_exe_angou_mod;
  int32_t original_source_header_size;
};

class Script {
 public:
  Script(std::string_view data, const XorKey& key) : data_(data), key_(key) {
    hdr_ = reinterpret_cast<PackedScene_hdr const*>(data.data());
    ParseScndata();
  }

  void ParseScndata() {
    scndata.clear();
    ByteReader reader(data_.substr(hdr_->scn_data_index_list_ofs,
                                   8 * hdr_->scn_data_index_cnt));
    for (int i = 0; i < hdr_->scn_data_index_cnt; ++i) {
      auto offset = reader.PopAs<uint32_t>(4);
      auto size = reader.PopAs<uint32_t>(4);

      std::string scene_data = Decompress_lzss(data_.substr(offset, size));
      Decrypt(scene_data);
      scndata.emplace_back(std::move(scene_data), key_);
    }
  }

  void Decrypt(std::string& scene_data) {
    if (hdr_->scn_data_exe_angou_mod != 0) {
      for (size_t i = 0; i < scene_data.length(); ++i)
        scene_data[i] ^= key_.exekey[i & 0xf];
    }

    for (size_t i = 0; i < scene_data.length(); ++i)
      scene_data[i] ^= key_.easykey[i & 0xff];
  }

  std::string_view data_;
  const XorKey& key_;

  PackedScene_hdr const* hdr_;
  std::vector<Scene> scndata;
};

}  // namespace libsiglus

#endif
