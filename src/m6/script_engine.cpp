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

#include "m6/script_engine.hpp"

#include "m6/compiler.hpp"
#include "m6/error_formatter.hpp"
#include "m6/parser.hpp"
#include "machine/rlmachine.hpp"

namespace m6 {

ScriptEngine::ScriptEngine()
    : src_(), valid_len_(0), compiler_(nullptr), machine_(nullptr) {}

ScriptEngine::ScriptEngine(std::shared_ptr<Compiler> compiler)
    : src_(), valid_len_(0), compiler_(compiler), machine_(nullptr) {
  if (compiler_ == nullptr)
    compiler_ = std::make_shared<Compiler>();
}

ScriptEngine::ScriptEngine(std::shared_ptr<Compiler> compiler,
                           std::shared_ptr<RLMachine> machine)
    : src_(), valid_len_(0), compiler_(compiler), machine_(machine) {
  if (compiler_ == nullptr)
    compiler_ = std::make_shared<Compiler>();
  if (machine_ == nullptr)
    machine_ = std::make_shared<RLMachine>(nullptr, nullptr, nullptr);
}

ScriptEngine::ExecutionResult ScriptEngine::Execute(std::string input_) {
  if (errors_.empty())
    valid_len_ = src_.length();

  std::string_view input =
      [&](std::string input_) {  // append input to source, then convert to a
                                 // string_view
        auto n = src_.length();
        src_ += input_;
        std::string_view src = src_;
        return src.substr(n);
      }(std::move(input_));

  ExecutionResult ret;
  Tokenizer tokenizer(ret.tokens);
  tokenizer.add_eof_ = true;
  tokenizer.skip_ws_ = true;
  tokenizer.Parse(input);

  for (auto& it : tokenizer.errors_)
    errors_.emplace_back(it.where(), it.what());
  if (!errors_.empty()) {
    ret.errors = errors_;
    return ret;
  }

  Parser parser(ret.tokens);
  ret.asts = parser.ParseAll();
  if (!parser.Ok()) {
    for (const auto& it : parser.GetErrors())
      errors_.emplace_back(it.loc, std::string(it.msg));
    ret.errors = errors_;
    return ret;
  }

  for (const auto& stmt : ret.asts) {
    if (!stmt)
      continue;

    try {
      auto ins_begin = ret.instructions.size();
      if (!compiler_)
        continue;
      compiler_->Compile(stmt, ret.instructions);

      if (!machine_)
        continue;
      bool is_expression = stmt->HoldsAlternative<std::shared_ptr<ExprAST>>();

      machine_->halted_ = false;
      machine_->ip_ = 0;
      machine_->script_ = std::span(ret.instructions.begin() + ins_begin,
                                    ret.instructions.end());
      machine_->Execute();
      if (is_expression) {
        ret.intermediateValues.emplace_back(std::move(machine_->stack_.back()));
        machine_->stack_.pop_back();
      }
    } catch (CompileError& e) {
      errors_.emplace_back(e.where(), e.what());
    }
  }

  if (!errors_.empty()) {
    ret.errors = errors_;
    return ret;
  }

  return ret;
}

std::string ScriptEngine::FlushErrors() {
  if (errors_.empty())
    return {};

  ErrorFormatter formatter(src_);
  for (const auto& it : errors_) {
    if (!it.loc.has_value())
      formatter.Pushline(it.msg);
    else
      formatter.Highlight(it.loc->begin_offset, it.loc->end_offset, it.msg);
  }

  errors_.clear();
  src_.resize(valid_len_);
  return formatter.Str();
}

}  // namespace m6
