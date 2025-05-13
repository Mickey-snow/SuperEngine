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

#include "core/compression.hpp"
#include "encodings/utf16.hpp"
#include "libsiglus/property.hpp"
#include "libsiglus/scene.hpp"
#include "libsiglus/xorkey.hpp"
#include "utilities/byte_reader.hpp"

#include <cstdint>
#include <iostream>
#include <map>
#include <string_view>
#include <vector>

namespace libsiglus {
struct Pack_hdr {
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

class Archive {
 public:
  Archive(std::string_view data, const XorKey& key) : data_(data), key_(key) {
    hdr_ = reinterpret_cast<Pack_hdr const*>(data.data());
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
      scndata.emplace_back(std::move(scene_data), i);
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
      const auto scnname = utf16le::Decode(names.substr(offset, size));

      scn_map_.emplace(scnname, i);
      scndata[i].scnname_ = std::move(scnname);
    }
  }

  void ParseIncprop() {
    ByteReader reader(data_.substr(hdr_->inc_prop_list_ofs,
                                   sizeof(Property) * hdr_->inc_prop_cnt));
    for (int i = 0; i < hdr_->inc_prop_cnt; ++i) {
      Property incprop;
      incprop.form = static_cast<Type>(reader.PopAs<int32_t>(4));
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
      const auto name = utf16le::Decode(props.substr(offset, size));
      prop_map_.emplace(name, i);
      prop_[i].name = name;
    }
  }

  void ParseIncCmd() {
    ByteReader reader(data_.substr(hdr_->inc_cmd_list_ofs,
                                   sizeof(Command) * hdr_->inc_cmd_cnt));
    for (int i = 0; i < hdr_->inc_cmd_cnt; ++i) {
      Command inccmd;
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
      const auto name = utf16le::Decode(cmds.substr(offset, size));
      cmd_map_.emplace(name, i);
      cmd_[i].name = name;
    }
  }

  std::string_view data_;
  const XorKey& key_;

  Pack_hdr const* hdr_;
  std::vector<Scene> scndata;

  std::map<std::string, int> scn_map_;

  std::vector<Property> prop_;
  std::map<std::string, int> prop_map_;

  std::vector<Command> cmd_;
  std::map<std::string, int> cmd_map_;
};

}  // namespace libsiglus
