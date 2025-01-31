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

#include "m6/parsing_error.hpp"

#include <boost/regex.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>

#include <optional>

namespace m6 {

Tokenizer::Tokenizer(std::string_view input, bool should_parse)
    : input_(input) {
  if (should_parse)
    Parse();
}

void Tokenizer::Parse() {
  namespace lex = boost::spirit::lex;
  enum token_ids {
    ID_identifier = 1000,
    ID_ws,
    ID_int,
    ID_bracket,
    ID_op,
    ID_string
  };
  struct lexer : lex::lexer<lex::lexertl::lexer<>> {
    lex::token_def<std::string> identifier;
    lex::token_def<std::string> literal;
    lex::token_def<> integer;
    lex::token_def<> ws;
    lex::token_def<> bracket;
    lex::token_def<> op;

    lexer()
        : identifier("[a-zA-Z_][a-zA-Z0-9_]*"),
          literal(R"(\"([^\"\\]|\\.)*\")"),
          integer("[0-9]+"),
          ws("[ \t\n]+"),
          bracket(R"([\(\)\[\]\{\}])"),  // matches any of ()[]{}
          op(R"(>>>=|>>>|>>=|>>|<<=|<<|\+=|\-=|\*=|\/=|%=|&=|\|=|\^=|==|!=|<=|>=|\|\||&&|=|\+|\-|\*|\/|%|~|&|\||\^|<|>|,)") {
      this->self.add(identifier, ID_identifier)(ws, ID_ws)(
          integer, ID_int)(bracket, ID_bracket)(op, ID_op)(literal, ID_string);
    }
  };

  parsed_tok_.clear();

  lexer mylexer;
  auto begin = input_.cbegin();
  auto end = input_.cend();

  std::optional<std::string> error_token = std::nullopt;
  bool success =
      lex::tokenize(begin, end, mylexer, [&](const auto& tok) -> bool {
        auto value = std::string(tok.value().begin(), tok.value().end());

        switch (tok.id()) {
          case ID_identifier:
            parsed_tok_.emplace_back(tok::ID(value));
            break;

          case ID_ws:
            parsed_tok_.emplace_back(tok::WS());
            break;

          case ID_int:
            parsed_tok_.emplace_back(tok::Int(std::stoi(value)));
            break;

          case ID_bracket: {
            static const std::unordered_map<char, Token> symbol_table{
                {'[', tok::SquareL()},      {']', tok::SquareR()},
                {'{', tok::CurlyL()},       {'}', tok::CurlyR()},
                {'(', tok::ParenthesisL()}, {')', tok::ParenthesisR()}};
            parsed_tok_.emplace_back(symbol_table.at(value.front()));
            break;
          }

          case ID_op:
            parsed_tok_.emplace_back(tok::Operator(CreateOp(value)));
            break;

          case ID_string: {
            std::string inner = value.substr(1, value.size() - 2);

            // Now parse escapes inside `inner` so that "\n" becomes an actual
            // newline,
            // '\"' becomes '"', etc.  We'll do a simple pass here:
            std::string result;
            result.reserve(inner.size());

            for (std::size_t i = 0; i < inner.size(); ++i) {
              if (inner[i] == '\\' && i + 1 < inner.size()) {
                // escaped character
                char c = inner[++i];
                switch (c) {
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
                    //...
                  default:
                    // just store the raw character, for now, e.g. '\x'
                    result.push_back(c);
                    break;
                }
              } else {
                result.push_back(inner[i]);
              }
            }

            parsed_tok_.emplace_back(tok::Literal(result));
            break;
          }

          default:
            error_token = value;
            return false;
        }
        return true;
      });

  if (!success) {
    if (error_token) {
      throw ParsingError("Tokenizer error: unexpected token '" +
                         error_token.value() + "'.");
    }
    throw ParsingError("Tokenizer: unable to parse " + std::string(input_));
  }
}

}  // namespace m6
