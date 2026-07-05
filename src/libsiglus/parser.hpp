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
#include "utilities/expected.hpp"

#include <iomanip>
#include <map>
#include <span>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

namespace libsiglus {

class Parser {
 public:
  Parser(std::string_view rawdata,
         std::span<const std::string> strpool,
         std::span<const int> labels,
         std::span<const int> zlabels,
         std::span<const Property> scnprop,
         std::span<const Property> globalprop,
         std::span<const Command> scncmd,
         std::span<const Command> gcmd,
         int scnno = -1,
         std::string_view debug_title = {});

  struct ParsedToken {
    int line;
    token::Token_t token;
  };
  using Tokens = std::vector<ParsedToken>;
  using Warnings = std::vector<std::string>;
  expected<std::pair<Tokens, Warnings>, std::string> ParseAll() noexcept;
  const Tokens& ParsedTokens() const noexcept { return parsed_; }

  inline void Add(Lexeme lex) {
    std::visit([&](auto&& x) { this->Add(std::forward<decltype(x)>(x)); },
               std::move(lex));
  }

 private:
  // helpers
  template <typename T>
  inline void emit_token(T&& t) {
    parsed_.emplace_back(++lineno_, std::forward<T>(t));
  }

  inline auto read_kidoku() { return reader_.PopAs<int>(4); }

  Value pop(Type type);
  Value pop_arg(const elm::ArgumentList::node_t& node);
  inline void push(Value val) { stack_.Push(std::move(val)); }
  inline void push(elm::ElementCode elm) { stack_.Push(std::move(elm)); }
  void push(const token::GetProperty& prop);
  void debug_assert_stack_empty();

  Variable add_var(Type type);
  void add_label(int id);
  void add_zlabel(int id);

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
  std::string_view raw_;
  std::string_view debug_title_;
  std::span<const std::string> strpool_;
  std::span<const int> labels_, zlabels_;
  std::span<const Property> scnprop_, gprop_;
  std::span<const Command> scncmd_, gcmd_;

  ByteReader reader_;
  int scnno_ = -1;
  int lineno_ = 0;
  Stack stack_;
  int var_cnt_ = 0;
  std::multimap<int, int> offset2labels_;
  std::multimap<int, int> offset2zlabels_;
  std::unordered_map<int, const Command*> offset2cmd_;

  const Command* curcall_cmd_ = nullptr;
  bool inside_curcall_body = false;
  std::vector<Type> curcall_args_;

  elm::ElementParser elm_parser_;

  std::vector<ParsedToken> parsed_;
  Warnings warnings_;
};

}  // namespace libsiglus
