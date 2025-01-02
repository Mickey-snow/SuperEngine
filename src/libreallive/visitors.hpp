// -----------------------------------------------------------------------
//
// This file is part of libreallive, a dependency of RLVM.
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
//
// -----------------------------------------------------------------------

#pragma once

#include <string>

class ModuleManager;

namespace libreallive {
class MetaElement;
class CommandElement;
class ExpressionElement;
class CommaElement;
class TextoutElement;

struct DebugStringVisitor {
  DebugStringVisitor(ModuleManager const* manager = nullptr);

  std::string operator()(MetaElement const*);
  std::string operator()(CommandElement const*);
  std::string operator()(ExpressionElement const*);
  std::string operator()(CommaElement const*);
  std::string operator()(TextoutElement const*);

 private:
  ModuleManager const* manager_;
};

}  // namespace libreallive
