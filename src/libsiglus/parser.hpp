// -----------------------------------------------------------------------
//
// This file is part of RLVM
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
// -----------------------------------------------------------------------

#pragma once

#include "libsiglus/lexeme.hpp"
#include "libsiglus/lexer.hpp"
#include "libsiglus/property.hpp"
#include "libsiglus/stack.hpp"
#include "libsiglus/value.hpp"
#include "utilities/byte_reader.hpp"

#include <iostream>
#include <map>
#include <string>
#include <variant>

namespace libsiglus {

namespace token {
struct Command {
  int overload_id;
  std::vector<int> elm;
  std::string name;
  std::vector<Value> arg;
  std::vector<std::pair<int, Value>> named_arg;
  Type return_type;

  Value dst;

  std::string ToDebugString() const;
};

struct Name {
  std::string str;

  std::string ToDebugString() const { return "Name(" + str + ')'; }
};

struct Textout {
  int kidoku;
  std::string str;

  std::string ToDebugString() const;
};

struct GetProperty {
  ElementCode elm;
  std::string name;
  Value dst;
  std::string ToDebugString() const;
};

struct Goto {
  int label;
  std::string ToDebugString() const;
};

struct GotoIf {
  int label;
  bool cond;
  Value src;

  std::string ToDebugString() const;
};

struct Label {
  int id;
  std::string ToDebugString() const;
};

struct Operate1 {
  OperatorCode op;
  Value rhs, dst;
  std::string ToDebugString() const;
};

struct Operate2 {
  OperatorCode op;
  Value lhs, rhs, dst;
  std::string ToDebugString() const;
};

struct Assign {
  ElementCode dst;
  Value src;
  std::string ToDebugString() const;
};

using Token_t = std::variant<Command,
                             Name,
                             Textout,
                             GetProperty,
                             Operate1,
                             Operate2,
                             Label,
                             Goto,
                             GotoIf,
                             Assign>;
inline std::string ToDebugString(const Token_t& stmt) {
  return std::visit(
      [](const auto& v) -> std::string { return v.ToDebugString(); }, stmt);
}
}  // namespace token

class Archive;
class Scene;
class Parser {
 public:
  Parser(Archive& archive, Scene& scene);

  token::Token_t Next();
  void ParseAll();

  // debug
  std::string DumpTokens() const;

 private:
  Value add_var(Type type);
  Value pop(Type type);
  void add_label(int id);

  // dispatch functions
  void Add(lex::Push);
  void Add(lex::Pop);
  void Add(lex::Line);
  void Add(lex::Marker);
  void Add(lex::Operate1);
  void Add(lex::Operate2);
  void Add(lex::Copy);
  void Add(lex::CopyElm);
  void Add(lex::Goto);
  void Add(lex::Property);
  void Add(lex::Command);
  void Add(lex::Assign);
  // void Add(lex::Namae);
  // void Add(lex::Textout);

  template <typename T>
  void Add(T t) {
    token_append(Lexeme{std::move(t)});
  }

 public:
  Archive& archive_;
  Scene& scene_;

  ByteReader reader_;
  Lexer lexer_;

  using token_t = std::variant<Lexeme, token::Token_t>;
  std::vector<token_t> token_;
  // helpers
  template <typename T>
  inline void token_append(T&& t) {
    token_.emplace_back(std::forward<T>(t));
  }

  int lineno_;
  Stack stack_;

  int var_cnt_;
  std::multimap<int, int> label_at_;
};

}  // namespace libsiglus
