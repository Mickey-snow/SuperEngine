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

#include "libsiglus/archive.hpp"
#include "libsiglus/parser.hpp"
#include "libsiglus/scene.hpp"

namespace libsiglus {

class ParserContext : public Parser::Context {
 public:
  ParserContext(Archive& archive, Scene& scene);

  virtual std::string_view SceneData() const override;
  virtual const std::vector<std::string>& Strings() const override;
  virtual const std::vector<int>& Labels() const override;

  virtual const std::vector<Property>& SceneProperties() const override;
  virtual const std::vector<Property>& GlobalProperties() const override;
  virtual const std::vector<Command>& SceneCommands() const override;
  virtual const std::vector<Command>& GlobalCommands() const override;

  virtual int SceneId() const override;
  virtual std::string GetDebugTitle() const override;

 private:
  Archive& archive_;
  Scene& scene_;
};
}  // namespace libsiglus
