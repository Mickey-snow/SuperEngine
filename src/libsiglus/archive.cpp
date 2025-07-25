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

#include "libsiglus/archive.hpp"

#include "core/compression.hpp"
#include "encodings/utf16.hpp"
#include "libsiglus/xorkey.hpp"
#include "utilities/byte_reader.hpp"
#include "utilities/mapped_file.hpp"

#include <array>
#include <string>

namespace libsiglus {

Archive Archive::Create(std::string_view raw_data) {
  for (const auto& it : keyring) {
    try {
      return Archive(raw_data, *it);
    } catch (...) {
    }
  }

  throw std::runtime_error("Archive::Create: no valid key found.");
}

Archive::Archive(std::string_view data, const XorKey& key)
    : data_(data), key_(key), cache(/*max_size=*/64) {
  hdr_ = reinterpret_cast<Pack_hdr const*>(data_.data());
  ParseScndata();
  CreateScnMap();

  ParseIncprop();
  CreateIncpropMap();

  ParseIncCmd();
  CreateIncCmdMap();
}

Scene Archive::ParseScene(int id) const {
  return cache.fetch_or_else(
      id, [&, id]() { return Scene(raw_scene_data_[id], id, scene_names_[id]); });
}

void Archive::ParseScndata() {
  raw_scene_data_.clear();
  raw_scene_data_.reserve(hdr_->scn_data_cnt);
  ByteReader reader(data_.substr(hdr_->scn_data_index_list_ofs,
                                 8 * hdr_->scn_data_index_cnt));

  for (int i = 0; i < hdr_->scn_data_cnt; ++i) {
    auto offset = reader.PopAs<uint32_t>(4);
    auto size = reader.PopAs<uint32_t>(4);

    std::string scene_data(
        data_.substr(offset + hdr_->scn_data_list_ofs, size));
    Decrypt(scene_data);
    scene_data = Decompress_lzss(scene_data);

    raw_scene_data_.emplace_back(std::move(scene_data));
  }
}

void Archive::Decrypt(std::string& scene_data) {
  if (hdr_->scn_data_exe_angou_mod != 0) {
    for (size_t i = 0; i < scene_data.length(); ++i) {
      scene_data[i] ^= key_.exekey[i & 0xf];
    }
  }

  static constexpr std::array<uint8_t, 256> easykey = {
      0x70, 0xf8, 0xa6, 0xb0, 0xa1, 0xa5, 0x28, 0x4f, 0xb5, 0x2f, 0x48, 0xfa,
      0xe1, 0xe9, 0x4b, 0xde, 0xb7, 0x4f, 0x62, 0x95, 0x8b, 0xe0, 0x03, 0x80,
      0xe7, 0xcf, 0x0f, 0x6b, 0x92, 0x01, 0xeb, 0xf8, 0xa2, 0x88, 0xce, 0x63,
      0x04, 0x38, 0xd2, 0x6d, 0x8c, 0xd2, 0x88, 0x76, 0xa7, 0x92, 0x71, 0x8f,
      0x4e, 0xb6, 0x8d, 0x01, 0x79, 0x88, 0x83, 0x0a, 0xf9, 0xe9, 0x2c, 0xdb,
      0x67, 0xdb, 0x91, 0x14, 0xd5, 0x9a, 0x4e, 0x79, 0x17, 0x23, 0x08, 0x96,
      0x0e, 0x1d, 0x15, 0xf9, 0xa5, 0xa0, 0x6f, 0x58, 0x17, 0xc8, 0xa9, 0x46,
      0xda, 0x22, 0xff, 0xfd, 0x87, 0x12, 0x42, 0xfb, 0xa9, 0xb8, 0x67, 0x6c,
      0x91, 0x67, 0x64, 0xf9, 0xd1, 0x1e, 0xe4, 0x50, 0x64, 0x6f, 0xf2, 0x0b,
      0xde, 0x40, 0xe7, 0x47, 0xf1, 0x03, 0xcc, 0x2a, 0xad, 0x7f, 0x34, 0x21,
      0xa0, 0x64, 0x26, 0x98, 0x6c, 0xed, 0x69, 0xf4, 0xb5, 0x23, 0x08, 0x6e,
      0x7d, 0x92, 0xf6, 0xeb, 0x93, 0xf0, 0x7a, 0x89, 0x5e, 0xf9, 0xf8, 0x7a,
      0xaf, 0xe8, 0xa9, 0x48, 0xc2, 0xac, 0x11, 0x6b, 0x2b, 0x33, 0xa7, 0x40,
      0x0d, 0xdc, 0x7d, 0xa7, 0x5b, 0xcf, 0xc8, 0x31, 0xd1, 0x77, 0x52, 0x8d,
      0x82, 0xac, 0x41, 0xb8, 0x73, 0xa5, 0x4f, 0x26, 0x7c, 0x0f, 0x39, 0xda,
      0x5b, 0x37, 0x4a, 0xde, 0xa4, 0x49, 0x0b, 0x7c, 0x17, 0xa3, 0x43, 0xae,
      0x77, 0x06, 0x64, 0x73, 0xc0, 0x43, 0xa3, 0x18, 0x5a, 0x0f, 0x9f, 0x02,
      0x4c, 0x7e, 0x8b, 0x01, 0x9f, 0x2d, 0xae, 0x72, 0x54, 0x13, 0xff, 0x96,
      0xae, 0x0b, 0x34, 0x58, 0xcf, 0xe3, 0x00, 0x78, 0xbe, 0xe3, 0xf5, 0x61,
      0xe4, 0x87, 0x7c, 0xfc, 0x80, 0xaf, 0xc4, 0x8d, 0x46, 0x3a, 0x5d, 0xd0,
      0x36, 0xbc, 0xe5, 0x60, 0x77, 0x68, 0x08, 0x4f, 0xbb, 0xab, 0xe2, 0x78,
      0x07, 0xe8, 0x73, 0xbf};

  for (size_t i = 0; i < scene_data.length(); ++i) {
    scene_data[i] ^= easykey[i & 0xff];
  }
}

void Archive::CreateScnMap() {
  std::u16string_view names =
      sv_to_u16sv(data_.substr(hdr_->scn_name_list_ofs));
  ByteReader reader(data_.substr(hdr_->scn_name_index_list_ofs,
                                 8 * hdr_->scn_name_index_cnt));

  // map from scene name to scene id
  scene_names_.reserve(hdr_->scn_name_cnt);
  for (int i = 0; i < hdr_->scn_name_cnt; ++i) {
    auto offset = reader.PopAs<uint32_t>(4);
    auto size = reader.PopAs<uint32_t>(4);
    const auto scnname = utf16le::Decode(names.substr(offset, size));

    scn_map_.emplace(scnname, i);
    scene_names_.emplace_back(std::move(scnname));
  }
}

void Archive::ParseIncprop() {
  ByteReader reader(data_.substr(hdr_->inc_prop_list_ofs,
                                 sizeof(Property) * hdr_->inc_prop_cnt));
  for (int i = 0; i < hdr_->inc_prop_cnt; ++i) {
    Property incprop;
    incprop.form = static_cast<Type>(reader.PopAs<int32_t>(4));
    incprop.size = reader.PopAs<int32_t>(4);
    prop_.emplace_back(std::move(incprop));
  }
}

void Archive::CreateIncpropMap() {
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

void Archive::ParseIncCmd() {
  ByteReader reader(data_.substr(hdr_->inc_cmd_list_ofs,
                                 sizeof(Command) * hdr_->inc_cmd_cnt));
  for (int i = 0; i < hdr_->inc_cmd_cnt; ++i) {
    Command inccmd;
    inccmd.scene_id = reader.PopAs<int32_t>(4);
    inccmd.offset = reader.PopAs<int32_t>(4);
    cmd_.emplace_back(std::move(inccmd));
  }
}

void Archive::CreateIncCmdMap() {
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

}  // namespace libsiglus
