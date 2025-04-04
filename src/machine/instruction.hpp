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

#include "machine/op.hpp"
#include "machine/value.hpp"

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

struct Push {
  Value value;
};

struct Pop {
  size_t count = 1;
};

struct End {
  std::string extra_text;
};

struct BinaryOp {
  Op op;
};

struct UnaryOp {
  Op op;
};

struct Load {
  size_t offset;
};

struct LoadGlobal {
  size_t offset;
};

struct Store {
  size_t offset;
};

struct StoreGlobal {
  size_t offset;
};

struct Invoke {
  size_t arity;
};

struct Jmp {
  int offset;
};

struct Jt {
  int offset;
};

struct Jf {
  int offset;
};

using Instruction = std::variant<std::monostate,
                                 Kidoku,
                                 Line,
                                 rlCommand,
                                 rlExpression,
                                 Textout,
                                 Push,
                                 Pop,
                                 BinaryOp,
                                 UnaryOp,
                                 Load,
                                 LoadGlobal,
                                 Store,
                                 StoreGlobal,
                                 Invoke,
                                 Jmp,
                                 Jt,
                                 Jf,
                                 End>;

class ModuleManager;
class InstructionToString {
 public:
  InstructionToString(ModuleManager const* manager = nullptr);

  std::string operator()(std::monostate) const;
  std::string operator()(const Kidoku&) const;
  std::string operator()(const Line&) const;
  std::string operator()(const rlCommand&) const;
  std::string operator()(const rlExpression&) const;
  std::string operator()(const Textout&) const;
  std::string operator()(const Push&) const;
  std::string operator()(const Pop&) const;
  std::string operator()(const BinaryOp&) const;
  std::string operator()(const UnaryOp&) const;
  std::string operator()(const Load&) const;
  std::string operator()(const Store&) const;
  std::string operator()(const LoadGlobal&) const;
  std::string operator()(const StoreGlobal&) const;
  std::string operator()(const Invoke&) const;
  std::string operator()(const Jmp&) const;
  std::string operator()(const Jt&) const;
  std::string operator()(const Jf&) const;
  std::string operator()(const End&) const;

 private:
  ModuleManager const* manager_;
};
