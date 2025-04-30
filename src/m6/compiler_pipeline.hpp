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
#include "m6/parser.hpp"
#include "m6/token.hpp"
#include "m6/tokenizer.hpp"

#include <optional>
#include <vector>

namespace m6 {

class CompilerPipeline {
 public:
  CompilerPipeline(bool repl = false) : tz(tokens), gen_(repl) {}

  void compile(std::string input_) {
    if (errors.empty()) {
      value_len_ = src_.length();
      tokens.clear();
      asts.clear();
    }

    std::string_view input =
        [&](std::string input_) {  // append input to source, then convert to a
                                   // string_view
          auto n = src_.length();
          src_ += std::move(input_);
          std::string_view src = src_;
          return src.substr(n);
        }(std::move(input_));

    tz.Parse(input);
    if (!tz.errors_.empty()) {
      for (auto const& e : tz.errors_)
        errors.emplace_back(e.where(), e.what());
      return;
    }

    Parser parser(tokens);
    asts = parser.ParseAll();

    if (!parser.Ok()) {
      for (auto const& e : parser.GetErrors())
        errors.emplace_back(e.loc, std::string(e.msg));
      return;
    }

    try {
      for (const auto& it : asts)
        gen_.Gen(it);
    } catch (std::exception const& ex) {
      errors.emplace_back(std::nullopt, ex.what());
    }
  }

  std::shared_ptr<serilang::Chunk> Get() {
    auto result = gen_.GetChunk();
    gen_.SetChunk(std::make_shared<serilang::Chunk>());
    return result;
  }

 public:
  /**
   * @brief Holds the details of one error.
   */
  struct ErrorInfo {
    std::optional<SourceLocation> loc;
    std::string msg;
  };

  std::vector<Token> tokens;
  std::vector<std::shared_ptr<AST>> asts;
  std::vector<ErrorInfo> errors;

 public:
  bool Ok() const { return errors.empty(); }

 private:
  Tokenizer tz;
  CodeGenerator gen_;
  std::string src_;
  size_t value_len_;
};

}  // namespace m6
