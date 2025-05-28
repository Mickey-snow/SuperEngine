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

#include "libsiglus/types.hpp"

#include <memory>
#include <string>
#include <variant>

namespace libsiglus {

namespace lex {
struct None;
struct Line;
struct Push;
struct Pop;
struct Marker;
struct Command;
struct Property;
struct Operate1;
struct Operate2;
struct Goto;
struct Assign;
struct Copy;
struct CopyElm;
struct Gosub;
struct Return;
struct Namae;
struct EndOfScene;
struct Textout;
struct Arg;
struct Declare;
}  // namespace lex

using Lexeme = std::variant<lex::None,
                            lex::Line,
                            lex::Push,
                            lex::Pop,
                            lex::Marker,
                            lex::Command,
                            lex::Property,
                            lex::Operate1,
                            lex::Operate2,
                            lex::Goto,
                            lex::Assign,
                            lex::Copy,
                            lex::CopyElm,
                            lex::Gosub,
                            lex::Return,
                            lex::Namae,
                            lex::EndOfScene,
                            lex::Textout,
                            lex::Arg,
                            lex::Declare>;

}  // namespace libsiglus
