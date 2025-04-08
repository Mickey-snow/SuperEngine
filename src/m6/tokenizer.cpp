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

#include "m6/tokenizer.hpp"

#include "log/domain_logger.hpp"
#include "machine/op.hpp"

#include <cctype>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace m6 {

namespace {
// A static lookup table for single-character tokens
static const std::unordered_map<char, tok::Token_t> SINGLE_CHAR_TOKEN = {
    {'[', tok::SquareL()},      {']', tok::SquareR()},
    {'{', tok::CurlyL()},       {'}', tok::CurlyR()},
    {'(', tok::ParenthesisL()}, {')', tok::ParenthesisR()},
    {';', tok::Semicol()}};

// Sorted by descending length, so we can match the longest possible operator
// first.
static const std::vector<std::string> OPERATORS = {
    ">>>=", ">>>", ">>=", ">>", "<<=", "<<", "+=", "-=", "*=", "/=", "%=", "&=",
    "|=",   "^=",  "==",  "!=", "<=",  ">=", "||", "&&", "=",  "+",  "-",  "*",
    "/",    "%",   "~",   "&",  "|",   "^",  "<",  ">",  ",",  "."};

static const std::unordered_set<std::string> RESERVED_KEYWORDS = {
    "if", "else", "while", "for"};

// Attempt to match an operator from position `pos` in `input`.
// Returns the matched operator string if successful, else an empty string.
static std::string matchLongestOperator(std::string_view input, size_t pos) {
  for (auto const& op : OPERATORS) {
    if (pos + op.size() <= input.size()) {
      if (input.compare(pos, op.size(), op) == 0) {
        return op;
      }
    }
  }
  return {};
}

// Unescape a string literal (removes surrounding quotes and handles backslash
// escapes).
static std::string unescapeString(std::string_view value) {
  // e.g., if value = "\"some\\nstuff\"", we remove leading/trailing quotes,
  // then parse escapes
  if (value.size() < 2) {
    // malformed, but we'll just return empty
    return {};
  }

  // Remove leading and trailing quote (")
  std::string_view inner = value.substr(1, value.size() - 2);

  std::string result;
  result.reserve(inner.size());

  for (std::size_t i = 0; i < inner.size(); ++i) {
    char c = inner[i];
    if (c == '\\' && i + 1 < inner.size()) {
      // escaped character
      char next = inner[++i];
      switch (next) {
        case 'n':
          result.push_back('\n');
          break;
        case 't':
          result.push_back('\t');
          break;
        case 'r':
          result.push_back('\r');
          break;
        case '\\':
          result.push_back('\\');
          break;
        case '"':
          result.push_back('"');
          break;
        // ...
        default:
          result.push_back(next);
          break;
      }
    } else {
      result.push_back(c);
    }
  }
  return result;
}

}  // namespace

void Tokenizer::Parse(std::string_view input) {
  if (!errors_.empty()) {
    DomainLogger logger("Tokenizer");
    logger(Severity::Warn) << "Unhandled errors.";
  }

  size_t pos = 0;
  const size_t len = input.size();

  while (pos < len) {
    const size_t start = pos;
    char c = input[pos];

    // 1) Check whitespace
    if (std::isspace(static_cast<unsigned char>(c))) {
      // Accumulate all consecutive whitespace into one token
      while (pos < len &&
             std::isspace(static_cast<unsigned char>(input[pos]))) {
        ++pos;
      }

      if (!skip_ws_) {
        storage_.emplace_back(tok::WS(), SourceLocation(start, pos));
      }
      continue;
    }

    // 2) Check single character token
    if (SINGLE_CHAR_TOKEN.find(c) != SINGLE_CHAR_TOKEN.end()) {
      storage_.emplace_back(SINGLE_CHAR_TOKEN.at(c),
                            SourceLocation(start, ++pos));
      continue;
    }

    // 3) Check operator (longest match)
    {
      std::string opMatch = matchLongestOperator(input, pos);
      if (!opMatch.empty()) {
        // we matched an operator token
        pos += opMatch.size();
        storage_.emplace_back(tok::Operator(CreateOp(opMatch)),
                              SourceLocation(start, pos));

        continue;
      }
    }

    std::string idVal = [&]() -> std::string {
      if (!std::isalpha(static_cast<unsigned char>(c)) && c != '_')
        return "";

      ++pos;
      while (pos < len &&
             (std::isalnum(static_cast<unsigned char>(input[pos])) ||
              input[pos] == '_')) {
        ++pos;
      }
      return std::string(input.substr(start, pos - start));
    }();

    // 4) Check reserved keywords
    if (RESERVED_KEYWORDS.contains(idVal)) {
      storage_.emplace_back(tok::Reserved(std::move(idVal)),
                            SourceLocation(start, pos));
      continue;
    }

    // 5) Check identifier: [a-zA-Z_][a-zA-Z0-9_]*
    if (!idVal.empty()) {
      storage_.emplace_back(tok::ID(std::move(idVal)),
                            SourceLocation(start, pos));
      continue;
    }

    // 6) Check integer: [0-9]+
    if (std::isdigit(static_cast<unsigned char>(c))) {
      ++pos;
      while (pos < len &&
             std::isdigit(static_cast<unsigned char>(input[pos]))) {
        ++pos;
      }
      std::string numVal = std::string(input.substr(start, pos - start));
      storage_.emplace_back(tok::Int(std::stoi(numVal)),
                            SourceLocation(start, pos));
      continue;
    }

    // 7) Check string literal: \"([^\"\\\\]|\\\\.)*\"
    // A naive approach: detect '\"', then parse until matching '\"' or end of
    // input.
    if (c == '\"') {
      ++pos;  // consume the opening quote
      bool closed = false;
      while (pos < len) {
        if (input[pos] == '\\') {  // skip escaped character
          pos += 2;
        } else if (input[pos] == '\"') {  // found closing quote
          ++pos;
          closed = true;
          break;
        } else if (input[pos] == '\n') {  // disallow multi-line string literal
          closed = false;
          break;
        } else {
          ++pos;
        }
      }

      if (!closed) {
        storage_.emplace_back(tok::Literal(std::string(input.substr(start))),
                              SourceLocation(start, pos));
        errors_.emplace_back("Expected '\"'", SourceLocation::At(pos));
      } else {
        // substring from start to pos is the entire literal (including quotes)
        std::string fullString = std::string(input.substr(start, pos - start));
        // unescape it
        std::string unescaped = unescapeString(fullString);
        storage_.emplace_back(tok::Literal(std::move(unescaped)),
                              SourceLocation(start, pos));
      }
      continue;
    }

    // 8) If none matched, mark it as an error. We consume one character.
    errors_.emplace_back("Unknown token", SourceLocation(start, start + 1));
    ++pos;
  }

  if (add_eof_)
    storage_.emplace_back(tok::Eof(), SourceLocation(len, len + 1));
}

Tokenizer::Tokenizer(std::vector<Token>& s)
    : errors_(), skip_ws_(true), add_eof_(true), storage_(s) {}

}  // namespace m6
