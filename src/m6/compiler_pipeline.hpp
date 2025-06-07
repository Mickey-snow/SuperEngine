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

#include "m6/codegen.hpp"
#include "m6/error_formatter.hpp"
#include "m6/parser.hpp"
#include "m6/source_buffer.hpp"
#include "m6/token.hpp"
#include "m6/tokenizer.hpp"
#include "vm/gc.hpp"

#include <optional>
#include <vector>

namespace m6 {

class CompilerPipeline {
 public:
  CompilerPipeline(serilang::GarbageCollector& gc, bool repl = false)
      : gc_(gc), tz(tokens_), gen_(gc_, repl) {}

  void compile(std::shared_ptr<SourceBuffer> src) {
    Clear();

    tz.Parse(src);
    if (!tz.Ok()) {
      AddErrors(tz.GetErrors());
      tz.ClearErrors();
      return;
    }

    Parser parser(tokens_);
    asts_ = parser.ParseAll();

    if (!parser.Ok()) {
      AddErrors(parser.GetErrors());
      parser.ClearErrors();
      return;
    }

    for (const auto& it : asts_)
      gen_.Gen(it);
    if (!gen_.Ok()) {
      AddErrors(gen_.GetErrors());
      gen_.ClearErrors();
      return;
    }
  }

  std::shared_ptr<serilang::Chunk> Get() {
    auto result = gen_.GetChunk();
    gen_.SetChunk(std::make_shared<serilang::Chunk>());
    return result;
  }

  void Clear() {
    tokens_.clear();
    asts_.clear();
    errors_.clear();
  }

 private:
  std::vector<Token> tokens_;
  std::vector<std::shared_ptr<AST>> asts_;

  std::vector<Error> errors_;
  void AddErrors(std::span<const Error> errors_in) {
    for (const auto& e : errors_in)
      errors_.emplace_back(e);
  }

 public:
  bool Ok() const { return errors_.empty(); }
  std::string FormatErrors() {
    if (errors_.empty())
      return {};
    ErrorFormatter formatter;
    for (const auto& it : errors_) {
      if (!it.loc.has_value())
        formatter.Pushline(it.msg);
      else
        formatter.Highlight(it.loc.value(), it.msg);
    }
    return formatter.Str();
  }

 private:
  serilang::GarbageCollector& gc_;
  Tokenizer tz;
  CodeGenerator gen_;
};

}  // namespace m6
