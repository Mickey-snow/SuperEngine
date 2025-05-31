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
#include "libsiglus/element_code.hpp"
#include "libsiglus/lexeme.hpp"
#include "libsiglus/lexer.hpp"
#include "libsiglus/property.hpp"
#include "libsiglus/scene.hpp"
#include "libsiglus/stack.hpp"
#include "libsiglus/token.hpp"
#include "libsiglus/value.hpp"
#include "utilities/byte_reader.hpp"

#include <iomanip>
#include <map>
#include <sstream>
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

  void ParseAll();

 private:
  // helpers
  template <typename T>
  inline void emit_token(T&& t) {
    (*out_) = std::forward<T>(t);
  }

  Value pop(Type type);
  inline void push(Value val) { stack_.Push(std::move(val)); }
  inline void push(ElementCode elm) { stack_.Push(std::move(elm)); }
  void push(const token::GetProperty& prop);
  void debug_assert_stack_empty();

  Value add_var(Type type);
  void add_label(int id);

  elm::AccessChain resolve_element(const ElementCode& elm);
  elm::AccessChain resolve_usrcmd(const ElementCode& elm, size_t idx);
  elm::AccessChain resolve_usrprop(const ElementCode& elm, size_t idx);
  elm::AccessChain make_element(const ElementCode& elm);

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
  void Add(lex::Gosub);
  void Add(lex::Arg);
  void Add(lex::Return);
  void Add(lex::Declare);
  void Add(lex::Namae);
  void Add(lex::Textout);
  void Add(lex::EndOfScene);

  template <typename T>
  void Add(T t) {
    reader_.Proceed(-t.ByteLength());

    std::stringstream ss;
    ss << "Parser: Unsupported lexeme " + t.ToDebugString();
    ss << " [";
    for (size_t i = 0; i < 128; ++i) {
      if (reader_.Position() >= reader_.Size())
        break;
      ss << std::setfill('0') << std::setw(2) << std::hex
         << reader_.PopAs<int>(1) << ' ';
    }
    ss << "]";
    throw std::runtime_error(ss.str());
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
  std::unordered_map<int, int> offset2cmd_;

  Command* curcall_cmd_ = nullptr;
  std::vector<Type> curcall_args_;
};

}  // namespace libsiglus
