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

#include "libsiglus/element.hpp"
#include "libsiglus/property.hpp"

#include <memory>

namespace libsiglus::elm {

class ElementParser {
 public:
  class Context {
   public:
    virtual ~Context() = default;

    virtual const std::vector<Property>& SceneProperties() const = 0;
    virtual const std::vector<Property>& GlobalProperties() const = 0;
    virtual const std::vector<Command>& SceneCommands() const = 0;
    virtual const std::vector<Command>& GlobalCommands() const = 0;
    virtual const std::vector<Type>& CurcallArgs() const = 0;

    virtual int ReadKidoku() = 0;

    virtual int SceneId() const = 0;

    virtual void Warn(std::string message) = 0;
  };

 public:
  ElementParser(std::unique_ptr<Context> ctx);
  ~ElementParser();

  AccessChain Parse(ElementCode& elm);

 private:
  AccessChain resolve_usrcmd(ElementCode& elm, size_t idx);
  AccessChain resolve_usrprop(ElementCode& elm, size_t idx);
  AccessChain resolve_element(ElementCode& elm);

 private:
  std::unique_ptr<Context> ctx_;
};

}  // namespace libsiglus::elm
