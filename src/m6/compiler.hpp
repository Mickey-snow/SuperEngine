// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
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
//
// -----------------------------------------------------------------------

#pragma once

#include "machine/instruction.hpp"
#include "machine/value.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace m6 {

class ExprAST;
class AST;

class Compiler {
 private:
  struct Visitor;

 public:
  Compiler();
  ~Compiler() = default;

  std::vector<Instruction> Compile(std::shared_ptr<ExprAST> expr);
  std::vector<Instruction> Compile(std::shared_ptr<AST> stmt);

  void Compile(std::shared_ptr<ExprAST> expr, std::vector<Instruction>&);
  void Compile(std::shared_ptr<AST> stmt, std::vector<Instruction>&);

  void AddNative(Value fn);

 private:
  void PushScope();
  size_t PopScope();

  size_t AddGlobal(const std::string& id);
  std::optional<size_t> FindLocal(const std::string& id) const;
  size_t AddLocal(const std::string& id);

  std::unordered_map<std::string, size_t> global_variable_;
  std::vector<std::unordered_map<std::string, size_t>> local_variable_;
  size_t local_cnt_;

  std::unordered_map<std::string, Value> native_fn_;
};

}  // namespace m6
