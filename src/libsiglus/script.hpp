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
#include "encodings/utf16.hpp"
#include "libsiglus/scene.hpp"
#include "libsiglus/xorkey.hpp"
#include "utilities/byte_reader.h"

#include <cstdint>
#include <iostream>
#include <map>
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

inline static std::u16string_view sv_to_u16sv(std::string_view sv) {
  return std::u16string_view(reinterpret_cast<const char16_t*>(sv.data()),
                             sv.size() / sizeof(char16_t));
}

class Script {
 public:
  Script(std::string_view data, const XorKey& key) : data_(data), key_(key) {
    hdr_ = reinterpret_cast<PackedScene_hdr const*>(data.data());
    ParseScndata();
    CreateScnMap();

    ParseIncprop();
    CreateIncpropMap();

    ParseIncCmd();
    CreateIncCmdMap();
  }

  void ParseScndata() {
    scndata.clear();
    ByteReader reader(data_.substr(hdr_->scn_data_index_list_ofs,
                                   8 * hdr_->scn_data_index_cnt));
    for (int i = 0; i < hdr_->scn_data_cnt; ++i) {
      auto offset = reader.PopAs<uint32_t>(4);
      auto size = reader.PopAs<uint32_t>(4);

      std::string scene_data(
          data_.substr(offset + hdr_->scn_data_list_ofs, size));
      Decrypt(scene_data);
      scene_data = Decompress_lzss(scene_data);
      scndata.emplace_back(std::move(scene_data));
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

  void CreateScnMap() {
    std::u16string_view names =
        sv_to_u16sv(data_.substr(hdr_->scn_name_list_ofs));
    ByteReader reader(data_.substr(hdr_->scn_name_index_list_ofs,
                                   8 * hdr_->scn_name_index_cnt));

    // map from scene name to scene id
    for (int i = 0; i < hdr_->scn_name_cnt; ++i) {
      auto offset = reader.PopAs<uint32_t>(4);
      auto size = reader.PopAs<uint32_t>(4);
      scn_map_.emplace(utf16le::Decode(names.substr(offset, size)), i);
    }
  }

  void ParseIncprop() {
    ByteReader reader(data_.substr(hdr_->inc_prop_list_ofs,
                                   sizeof(Incprop) * hdr_->inc_prop_cnt));
    for (int i = 0; i < hdr_->inc_prop_cnt; ++i) {
      Incprop incprop;
      incprop.form = reader.PopAs<int32_t>(4);
      incprop.size = reader.PopAs<int32_t>(4);
      prop_.emplace_back(std::move(incprop));
    }
  }

  void CreateIncpropMap() {
    ByteReader reader(data_.substr(hdr_->inc_prop_name_index_list_ofs,
                                   8 * hdr_->inc_prop_name_cnt));
    std::u16string_view props =
        sv_to_u16sv(data_.substr(hdr_->inc_prop_name_list_ofs));

    for (int i = 0; i < hdr_->inc_prop_name_cnt; ++i) {
      auto offset = reader.PopAs<uint32_t>(4);
      auto size = reader.PopAs<uint32_t>(4);
      prop_map_.emplace(utf16le::Decode(props.substr(offset, size)), i);
    }
  }

  void ParseIncCmd() {
    ByteReader reader(data_.substr(hdr_->inc_cmd_list_ofs,
                                   sizeof(Inccmd) * hdr_->inc_cmd_cnt));
    for (int i = 0; i < hdr_->inc_cmd_cnt; ++i) {
      Inccmd inccmd;
      inccmd.scene_id = reader.PopAs<int32_t>(4);
      inccmd.offset = reader.PopAs<int32_t>(4);
      cmd_.emplace_back(std::move(inccmd));
    }
  }

  void CreateIncCmdMap() {
    ByteReader reader(data_.substr(hdr_->inc_cmd_name_index_list_ofs,
                                   8 * hdr_->inc_cmd_name_index_cnt));
    std::u16string_view cmds =
        sv_to_u16sv(data_.substr(hdr_->inc_cmd_name_list_ofs));

    for (int i = 0; i < hdr_->inc_cmd_name_cnt; ++i) {
      auto offset = reader.PopAs<uint32_t>(4);
      auto size = reader.PopAs<uint32_t>(4);
      cmd_map_.emplace(utf16le::Decode(cmds.substr(offset, size)), i);
    }
  }

  std::string_view data_;
  const XorKey& key_;

  PackedScene_hdr const* hdr_;
  std::vector<Scene> scndata;

  std::map<std::string, int> scn_map_;

  struct Incprop {
    int32_t form;
    int32_t size;
  };
  std::vector<Incprop> prop_;
  std::map<std::string, int> prop_map_;

  struct Inccmd {
    int32_t scene_id;
    int32_t offset;
  };
  std::vector<Inccmd> cmd_;
  std::map<std::string, int> cmd_map_;
};

}  // namespace libsiglus

#endif
