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

#include "libsiglus/scene.hpp"

#include "core/compression.hpp"
#include "encodings/utf16.hpp"
#include "utilities/byte_reader.hpp"

#include <utility>

namespace libsiglus {

Scene::Scene(std::string data, int id, std::string name)
    : id_(id), scnname_(std::move(name)), data_(std::move(data)) {
  hdr_ = reinterpret_cast<Scene_hdr const*>(data_.data());
  std::string_view sv = data_;

  scene_ = sv.substr(hdr_->scene_offset, hdr_->scene_size);

  {
    std::string_view stridx =
        sv.substr(hdr_->str_idxlist_offset, 8 * hdr_->str_idxlist_size);
    std::u16string_view strdata = sv_to_u16sv(sv.substr(hdr_->str_list_offset));
    ByteReader reader(stridx);
    for (int i = 0; i < hdr_->str_idxlist_size; ++i) {
      auto offset = reader.PopAs<uint32_t>(4);
      auto size = reader.PopAs<uint32_t>(4);

      std::u16string str(strdata.substr(offset, size));
      for (char16_t& c : str)
        c ^= 28807 * i;

      str_.emplace_back(utf16le::Decode(str));
    }
  }

  {
    ByteReader reader(sv.substr(hdr_->label_list_offset, 4 * hdr_->label_cnt));
    label.reserve(hdr_->label_cnt);
    for (int i = 0; i < hdr_->label_cnt; ++i)
      label.push_back(reader.PopAs<int32_t>(4));
  }

  {
    ByteReader reader(
        sv.substr(hdr_->zlabel_list_offset, 4 * hdr_->zlabel_cnt));
    zlabel.reserve(hdr_->zlabel_cnt);
    for (int i = 0; i < hdr_->zlabel_cnt; ++i)
      zlabel.push_back(reader.PopAs<int32_t>(4));
  }

  {
    ByteReader reader(sv.substr(hdr_->cmdlabel_list_offset,
                                sizeof(CmdLabel) * hdr_->cmdlabel_cnt));
    for (int i = 0; i < hdr_->cmdlabel_cnt; ++i) {
      CmdLabel label;
      label.cmd_id = reader.PopAs<int32_t>(4);
      label.offset = reader.PopAs<int32_t>(4);
      cmdlabel.push_back(label);
    }
  }

  {
    ByteReader reader(
        sv.substr(hdr_->prop_offset, sizeof(Property) * hdr_->prop_cnt));
    property.reserve(hdr_->prop_cnt);
    for (int i = 0; i < hdr_->prop_cnt; ++i) {
      Property prop;
      prop.form = static_cast<Type>(reader.PopAs<int32_t>(4));
      prop.size = reader.PopAs<int32_t>(4);
      prop.name = "???";

      property.emplace_back(std::move(prop));
    }

    std::u16string_view names = sv_to_u16sv(sv.substr(hdr_->prop_name_offset));
    reader = ByteReader(
        sv.substr(hdr_->prop_nameidx_offset, 8 * hdr_->prop_nameidx_cnt));
    for (int i = 0; i < hdr_->prop_nameidx_cnt; ++i) {
      auto offset = reader.PopAs<int32_t>(4);
      auto size = reader.PopAs<int32_t>(4);
      const std::string name = utf16le::Decode(names.substr(offset, size));

      property_map[name] = i;
      property[i].name = std::move(name);
    }
  }

  {
    ByteReader reader(sv.substr(hdr_->cmdlist_offset, 4 * hdr_->cmd_cnt));
    for (int i = 0; i < hdr_->cmd_cnt; ++i) {
      Command usrcmd;
      usrcmd.scene_id = id_;
      usrcmd.offset = reader.PopAs<int32_t>(4);
      cmd.push_back(std::move(usrcmd));
    }

    std::u16string_view names = sv_to_u16sv(sv.substr(hdr_->cmd_name_offset));
    reader = ByteReader(
        sv.substr(hdr_->cmd_nameidx_offset, 8 * hdr_->cmd_nameidx_cnt));
    for (int i = 0; i < hdr_->cmd_nameidx_cnt; ++i) {
      auto offset = reader.PopAs<int32_t>(4);
      auto size = reader.PopAs<int32_t>(4);

      const auto name = utf16le::Decode(names.substr(offset, size));
      cmd_map.emplace(name, i);
      cmd[i].name = std::move(name);
    }
  }

  {
    ByteReader reader(
        sv.substr(hdr_->call_nameidx_offset, 8 * hdr_->call_nameidx_cnt));
    std::u16string_view names = sv_to_u16sv(sv.substr(hdr_->call_name_offset));
    for (int i = 0; i < hdr_->call_nameidx_cnt; ++i) {
      auto offset = reader.PopAs<int32_t>(4);
      auto size = reader.PopAs<int32_t>(4);
      callproperty.emplace_back(utf16le::Decode(names.substr(offset, size)));
    }
  }

  {
    ByteReader reader(sv.substr(hdr_->namae_offset, 4 * hdr_->namae_cnt));
    for (int i = 0; i < hdr_->namae_cnt; ++i)
      namae.push_back(reader.PopAs<int32_t>(4));
  }

  {
    ByteReader reader(sv.substr(hdr_->kidoku_offset, 4 * hdr_->kidoku_cnt));
    for (int i = 0; i < hdr_->kidoku_cnt; ++i)
      kidoku.push_back(reader.PopAs<int32_t>(4));
  }
}

std::string Scene::GetDebugTitle() const {
  return std::to_string(id_) + ' ' + scnname_;
}

}  // namespace libsiglus
