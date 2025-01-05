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
// Along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
//
// -----------------------------------------------------------------------

#pragma once

#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace tok {

struct ID {
  std::string id;
  auto operator<=>(const ID& rhs) const = default;
};

struct WS {
  auto operator<=>(const WS& rhs) const = default;
};

struct Int {
  int value;
  auto operator<=>(const Int& rhs) const = default;
};

struct Dollar {
  auto operator<=>(const Dollar& rhs) const = default;
};

struct Plus {
  auto operator<=>(const Plus& rhs) const = default;
};

struct Minus {
  auto operator<=>(const Minus& rhs) const = default;
};

struct Mult {
  auto operator<=>(const Mult& rhs) const = default;
};

struct Div {
  auto operator<=>(const Div& rhs) const = default;
};

struct Eq {
  auto operator<=>(const Eq& rhs) const = default;
};

struct SquareL {
  auto operator<=>(const SquareL& rhs) const = default;
};

struct SquareR {
  auto operator<=>(const SquareR& rhs) const = default;
};

struct CurlyL {
  auto operator<=>(const CurlyL& rhs) const = default;
};

struct CurlyR {
  auto operator<=>(const CurlyR& rhs) const = default;
};

struct ParenthesisL {
  auto operator<=>(const ParenthesisL& rhs) const = default;
};

struct ParenthesisR {
  auto operator<=>(const ParenthesisR& rhs) const = default;
};

}  // namespace tok

using Token = std::variant<tok::ID,
                           tok::WS,
                           tok::Int,
                           tok::Dollar,
                           tok::Plus,
                           tok::Minus,
                           tok::Mult,
                           tok::Div,
                           tok::Eq,
                           tok::SquareL,
                           tok::SquareR,
                           tok::CurlyL,
                           tok::CurlyR,
                           tok::ParenthesisL,
                           tok::ParenthesisR>;

class Tokenizer {
 public:
  Tokenizer(std::string_view input, bool should_parse = true);

  void Parse();

  std::vector<Token> parsed_tok_;

 private:
  std::string_view input_;
};

template <typename T>
struct match_token {
  using result_type = bool;
  bool operator()(const Token& t) const { return std::holds_alternative<T>(t); }
};

struct match_int {
  using result_type = bool;
  bool operator()(const Token& t, int& value) const {
    if (auto p = std::get_if<tok::Int>(&t)) {
      value = p->value;
      return true;
    }
    return false;
  }
};

struct match_id {
  using result_type = bool;
  bool operator()(const Token& t, std::string& value) const {
    if (auto p = std::get_if<tok::ID>(&t)) {
      value = p->id;
      return true;
    }
    return false;
  }
};
