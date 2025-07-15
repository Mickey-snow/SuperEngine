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

#include "libsiglus/element_parser.hpp"
#include "libsiglus/lexeme.hpp"
#include "libsiglus/lexer.hpp"
#include "libsiglus/property.hpp"
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
#include <vector>

namespace libsiglus {

class Parser {
 public:
  class Context {
   public:
    virtual ~Context() = default;

    virtual std::string_view SceneData() const = 0;
    virtual const std::vector<std::string>& Strings() const = 0;
    virtual const std::vector<int>& Labels() const = 0;

    virtual const std::vector<Property>& SceneProperties() const = 0;
    virtual const std::vector<Property>& GlobalProperties() const = 0;
    virtual const std::vector<Command>& SceneCommands() const = 0;
    virtual const std::vector<Command>& GlobalCommands() const = 0;

    virtual int SceneId() const = 0;
    virtual std::string GetDebugTitle() const = 0;

    virtual void Emit(token::Token_t) = 0;
    virtual void Warn(std::string message) = 0;
  };

  Parser(Context& ctx);

  void ParseAll();

  inline void Add(Lexeme lex) {
    std::visit([&](auto&& x) { this->Add(std::forward<decltype(x)>(x)); },
               std::move(lex));
  }

 private:
  // helpers
  template <typename T>
  inline void emit_token(T&& t) {
    ctx_.Emit(std::forward<T>(t));
  }

  inline auto read_kidoku() { return reader_.PopAs<int>(4); }

  Value pop(Type type);
  Value pop_arg(const elm::ArgumentList::node_t& node);
  inline void push(Value val) { stack_.Push(std::move(val)); }
  inline void push(elm::ElementCode elm) { stack_.Push(std::move(elm)); }
  void push(const token::GetProperty& prop);
  void debug_assert_stack_empty();

  Value add_var(Type type);
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
  void Add(lex::Gosub);
  void Add(lex::Arg);
  void Add(lex::Return);
  void Add(lex::Declare);
  void Add(lex::Namae);
  void Add(lex::Textout);
  void Add(lex::EndOfScene);
  void Add(lex::SelBegin);
  void Add(lex::SelEnd);

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

 private:
  Context& ctx_;

  ByteReader reader_;
  int lineno_ = 0;
  Stack stack_;
  int var_cnt_ = 0;
  std::multimap<int, int> offset2labels_;
  std::unordered_map<int, const Command*> offset2cmd_;

  const Command* curcall_cmd_ = nullptr;
  std::vector<Type> curcall_args_;

  std::unique_ptr<elm::ElementParser> elm_parser_;
};

}  // namespace libsiglus
