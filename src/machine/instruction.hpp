// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
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

#include <memory>
#include <string>
#include <variant>

namespace libreallive {
class CommandElement;
class ExpressionElement;
class BytecodeElement;
};  // namespace libreallive
class RLMachine;

// Modifies the topmost stack frame.
struct Jump {
  int scenario;
  int entrypoint;
};

// Creates a new frame.
struct Farcall {
  int scenario;
  int entrypoint;
};

// Local jump. Modifies the topmost stack frame. Destination can be anywhere in
// the current scenario.
struct Goto {
  int location;
};

// Local jump, but creates a new frame.
struct Gosub {
  int location;
};

struct Kidoku {
  int num;
};

struct Line {
  int num;
};

struct rlCommand {
  libreallive::CommandElement const* cmd;
};

class rlExpression {
 public:
  rlExpression(libreallive::ExpressionElement const* expr);
  int Execute(RLMachine& machine);

  libreallive::ExpressionElement const* expr_;
};

struct Textout {
  std::string text;
};

struct End {
  std::string extra_text;
};

using Instruction = std::variant<std::monostate,
                                 Kidoku,
                                 Line,
                                 rlCommand,
                                 rlExpression,
                                 Textout,
                                 End>;
