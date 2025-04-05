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
  tokenizer.Parse(input);

  auto skipws = [](Token*& it, Token* end) {
    while (it < end && (it->HoldsAlternative<tok::Eof>() ||
                        it->HoldsAlternative<tok::WS>()))
      ++it;
  };
  for (auto begin = ret.tokens.data(), end = ret.tokens.data() + ret.tokens.size();
       begin < end; skipws(begin, end)) {
    try {
      auto stmt = ParseStmt(begin, end);
      ret.asts.push_back(stmt);

      auto ins_begin = ret.instructions.size();
      if (!compiler_)
        continue;
      compiler_->Compile(stmt, ret.instructions);

      if (!machine_)
        continue;
      bool is_expression = stmt->HoldsAlternative<std::shared_ptr<ExprAST>>();

      machine_->halted_ = false;
      machine_->ip_ = 0;
      machine_->script_ =
          std::span(ret.instructions.begin() + ins_begin, ret.instructions.end());
      machine_->Execute();
      if (is_expression) {
        ret.intermediateValues.emplace_back(std::move(machine_->stack_.back()));
        machine_->stack_.pop_back();
      }
    } catch (const CompileError& err) {
      errors_.push_back(err);

      while (begin < end) {  // perform error recovery
        if (begin->HoldsAlternative<tok::Semicol>() ||
            begin->HoldsAlternative<tok::CurlyR>()) {
          ++begin;
          break;
        }
        ++begin;
      }
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

  std::vector<size_t> lineStart;
  lineStart.push_back(0);
  for (size_t i = 0; i < src_.size(); ++i)
    if (src_[i] == '\n')
      lineStart.push_back(i + 1);
  lineStart.push_back(src_.size() + 1);  // Push a sentinel for "end of file"

  auto findLine = [&](size_t offset) {
    auto it = std::upper_bound(lineStart.begin(), lineStart.end(), offset);
    size_t idx = (it == lineStart.begin()) ? 0 : (it - lineStart.begin() - 1);
    return idx;
  };

  std::ostringstream oss;

  for (const auto& err : errors_) {
    oss << err.what() << "\n";

    auto loc = err.where();
    if (!loc.has_value()) {
      continue;
    }

    size_t begin = loc->begin_offset;
    size_t end = loc->end_offset;
    if (begin >= src_.size())
      begin = src_.size();
    if (end > src_.size())
      end = src_.size();

    size_t lineBegin = findLine(begin);
    size_t colBegin = begin - lineStart[lineBegin];

    size_t lineEnd = findLine(end);
    size_t colEnd = end - lineStart[lineEnd];

    for (size_t lineIdx = lineBegin; lineIdx <= lineEnd; ++lineIdx) {
      size_t thisLineOffset = lineStart[lineIdx];
      size_t nextLineOffset = lineStart[lineIdx + 1];

      std::string lineText =
          src_.substr(thisLineOffset, nextLineOffset - thisLineOffset);
      if (!lineText.empty() && lineText.back() == '\n')
        lineText.pop_back();

      oss << lineText << "\n";

      // Now build the caret (^) line.
      // We'll place spaces up to the highlight start,
      // then ^ up to the highlight end (or line end).
      // The highlight range differs depending on whether this is the
      // first line of the error, a middle line, or the last line.
      size_t highlightBeginCol = 0;
      size_t highlightEndCol = lineText.size();  // highlight to EOL by default

      if (lineIdx == lineBegin) {
        // If we're on the very first error line, highlight from colBegin
        highlightBeginCol = colBegin;
      }
      if (lineIdx == lineEnd) {
        // If we're on the very last error line, highlight only until colEnd
        highlightEndCol = colEnd;
      }

      // Clamp the highlight range to [0..lineText.size()]
      if (highlightBeginCol > lineText.size())
        highlightBeginCol = lineText.size();
      if (highlightEndCol > lineText.size())
        highlightEndCol = lineText.size();

      if (highlightBeginCol < highlightEndCol) {
        std::string caretLine(lineText.size(), ' ');
        std::fill(caretLine.begin() + highlightBeginCol,
                  caretLine.begin() + highlightBeginCol, '^');
        oss << std::move(caretLine) << "\n";
      }

      if (lineIdx == lineEnd)
        break;
    }
    oss << "\n";
  }

  errors_.clear();
  src_.resize(valid_len_);
  return oss.str();
}

}  // namespace m6
