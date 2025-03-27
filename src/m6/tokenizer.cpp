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

#include "machine/op.hpp"

#include <cctype>
#include <optional>
#include <unordered_map>
#include <unordered_set>

namespace m6 {

Tokenizer::Tokenizer(std::string_view input, bool should_parse)
    : input_(input) {
  if (should_parse)
    Parse();
}

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

static const std::unordered_set<std::string> RESERVED_KEYWORDS = {"if", "else"};

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

void Tokenizer::Parse() {
  parsed_tok_.clear();

  const std::string_view& input = input_;
  size_t pos = 0;
  const size_t len = input.size();

  while (pos < len) {
    const size_t offset = pos;
    char c = input[pos];

    // 1) Check whitespace
    if (std::isspace(static_cast<unsigned char>(c))) {
      // Accumulate all consecutive whitespace into one token
      size_t start = pos;
      while (pos < len &&
             std::isspace(static_cast<unsigned char>(input[pos]))) {
        ++pos;
      }
      // We store a single WS token for this contiguous block
      parsed_tok_.emplace_back(tok::WS(), start);
      continue;
    }

    // 2) Check single character token
    if (SINGLE_CHAR_TOKEN.find(c) != SINGLE_CHAR_TOKEN.end()) {
      parsed_tok_.emplace_back(SINGLE_CHAR_TOKEN.at(c), offset);
      ++pos;
      continue;
    }

    // 3) Check operator (longest match)
    {
      std::string opMatch = matchLongestOperator(input, pos);
      if (!opMatch.empty()) {
        // we matched an operator token
        parsed_tok_.emplace_back(tok::Operator(CreateOp(opMatch)), offset);
        pos += opMatch.size();
        continue;
      }
    }

    size_t start = pos;
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
      parsed_tok_.emplace_back(tok::Reserved(std::move(idVal)), start);
      continue;
    }

    // 5) Check identifier: [a-zA-Z_][a-zA-Z0-9_]*
    if (!idVal.empty()) {
      parsed_tok_.emplace_back(tok::ID(std::move(idVal)), start);
      continue;
    }

    // 6) Check integer: [0-9]+
    if (std::isdigit(static_cast<unsigned char>(c))) {
      size_t start = pos;
      ++pos;
      while (pos < len &&
             std::isdigit(static_cast<unsigned char>(input[pos]))) {
        ++pos;
      }
      std::string numVal = std::string(input.substr(start, pos - start));
      parsed_tok_.emplace_back(tok::Int(std::stoi(numVal)), start);
      continue;
    }

    // 7) Check string literal: \"([^\"\\\\]|\\\\.)*\"
    // A naive approach: detect '\"', then parse until matching '\"' or end of
    // input.
    if (c == '\"') {
      size_t start = pos;
      ++pos;  // consume the opening quote
      bool closed = false;
      while (pos < len) {
        if (input[pos] == '\\') {
          // skip escaped character
          pos += 2;
        } else if (pos < len && input[pos] == '\"') {
          // found closing quote
          ++pos;
          closed = true;
          break;
        } else {
          ++pos;
        }
      }

      if (!closed) {
        parsed_tok_.emplace_back(tok::Literal(std::string(input.substr(start))),
                                 start);
        parsed_tok_.emplace_back(tok::Error("Expected '\"'"), pos);
      } else {
        // substring from start to pos is the entire literal (including quotes)
        std::string fullString = std::string(input.substr(start, pos - start));
        // unescape it
        std::string unescaped = unescapeString(fullString);
        parsed_tok_.emplace_back(tok::Literal(std::move(unescaped)), start);
      }
      continue;
    }

    // 8) If none matched, mark it as an error token. We consume one character.
    static const tok::Token_t error_tok = tok::Error("Unknown token");
    if (!parsed_tok_.empty() && parsed_tok_.back() != error_tok)
      parsed_tok_.emplace_back(error_tok, offset);
    ++pos;
  }

  // Finally, add an eof token
  parsed_tok_.emplace_back(tok::Eof(), len);
}

}  // namespace m6
