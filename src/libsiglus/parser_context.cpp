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

#include "libsiglus/parser_context.hpp"

namespace libsiglus {

ParserContext::ParserContext(Archive& archive, Scene& scene)
    : archive_(archive), scene_(scene) {}

std::string_view ParserContext::SceneData() const { return scene_.scene_; }
const std::vector<std::string>& ParserContext::Strings() const {
  return scene_.str_;
}
const std::vector<int>& ParserContext::Labels() const { return scene_.label; }

const std::vector<Property>& ParserContext::SceneProperties() const {
  return scene_.property;
}
const std::vector<Property>& ParserContext::GlobalProperties() const {
  return archive_.prop_;
}
const std::vector<Command>& ParserContext::SceneCommands() const {
  return scene_.cmd;
}
const std::vector<Command>& ParserContext::GlobalCommands() const {
  return archive_.cmd_;
}

int ParserContext::SceneId() const { return scene_.id_; }
std::string ParserContext::GetDebugTitle() const { return scene_.scnname_; }

}  // namespace libsiglus
