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
#include "libsiglus/scene.hpp"
#include "libsiglus/stack.hpp"
#include "libsiglus/value.hpp"

#include <iostream>
#include <string>
#include <variant>

namespace libsiglus {

namespace ast {

struct Command {
  int overload_id;
  std::vector<int> elm;
  std::vector<Value> arg;
  std::vector<std::pair<int, Value>> named_arg;
  Type return_type;

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
  std::string ToDebugString() const;
};

struct Goto {
  int label;
  std::string ToDebugString() const;
};

struct Label {
  int id;
  std::string ToDebugString() const;
};

using Stmt = std::variant<Command, Name, Textout, GetProperty>;
inline std::string ToDebugString(const Stmt& stmt) {
  return std::visit(
      [](const auto& v) -> std::string { return v.ToDebugString(); }, stmt);
}
}  // namespace ast

class Parser {
 public:
  Parser(Scene& scene);

  ast::Stmt Next();
  void ParseAll();

 private:
  // dispatch functions
  void Add(lex::Push);
  void Add(lex::Pop);
  void Add(lex::Line);
  void Add(lex::Marker);
  void Add(lex::Operate1);
  void Add(lex::Operate2);
  void Add(lex::Copy);
  void Add(lex::CopyElm);

  void Add(lex::Property);
  void Add(lex::Command);
  void Add(lex::Namae);
  void Add(lex::Textout);

  template <typename T>
  void Add(T t) {
    token_append(std::move(t));
  }

 public:
  Scene& scene_;

  ByteReader reader_;
  Lexer lexer_;

  using token_t = std::variant<Lexeme, ast::Stmt>;
  std::vector<token_t> token_;
  // helpers
  template <typename T>
  inline void token_append(T&& t) {
    token_.emplace_back(std::forward<T>(t));
  }

  // debug
  std::string DumpTokens() const;

  int lineno_;
  Stack stack_;
};

}  // namespace libsiglus
