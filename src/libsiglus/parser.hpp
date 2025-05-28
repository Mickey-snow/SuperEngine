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

#include "libsiglus/element.hpp"
#include "libsiglus/lexeme.hpp"
#include "libsiglus/lexer.hpp"
#include "libsiglus/property.hpp"
#include "libsiglus/scene.hpp"
#include "libsiglus/stack.hpp"
#include "libsiglus/token.hpp"
#include "libsiglus/value.hpp"
#include "utilities/byte_reader.hpp"

#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <variant>

namespace libsiglus {

class Archive;
class Scene;

class Parser {
 public:
  class OutputBuffer {
   public:
    virtual ~OutputBuffer() = default;
    virtual void operator=(token::Token_t) = 0;
  };

  Parser(Archive& archive,
         Scene& scene,
         std::shared_ptr<OutputBuffer> out = nullptr);

  token::Token_t Next();
  void ParseAll();

 private:
  // helpers
  template <typename T>
  inline void emit_token(T&& t) {
    (*out_) = std::forward<T>(t);
  }

  Value add_var(Type type);
  Value pop(Type type);
  void add_label(int id);
  Element resolve_element(std::span<int> elm) const;

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
    throw std::runtime_error("Parser: Unsupported lexeme " + t.ToDebugString());
  }

 public:
  Archive& archive_;
  Scene& scene_;
  std::shared_ptr<OutputBuffer> out_;

  ByteReader reader_;

  int lineno_;
  Stack stack_;

  int var_cnt_;
  std::multimap<int, int> offset2labels_;
};

}  // namespace libsiglus
