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

#include "interpreter/parser.hpp"
#include "base/expr_ast.hpp"

#include <boost/phoenix/core.hpp>
#include <boost/phoenix/object.hpp>
#include <boost/phoenix/operator.hpp>
#include <boost/spirit/home/x3.hpp>
#include <boost/spirit/include/lex_lexertl.hpp>
#include <boost/spirit/include/qi.hpp>

namespace qi = boost::spirit::qi;
namespace phx = boost::phoenix;
namespace x3 = boost::spirit::x3;

// -----------------------------------------------------------------------
// primitive token parsers
namespace {
struct int_token_parser : qi::primitive_parser<int_token_parser> {
  template <typename Context, typename Iterator>
  struct attribute {
    using type = int;
  };

  template <typename Iterator,
            typename Context,
            typename Skipper,
            typename Attribute>
  bool parse(Iterator& first,
             Iterator const& last,
             Context&,
             Skipper const&,
             Attribute& attr) const {
    if (first == last)
      return false;

    if (auto p = std::get_if<tok::Int>(&*first)) {
      attr = p->value;
      ++first;
      return true;
    }
    return false;
  }

  template <typename Context>
  boost::spirit::info what(Context&) const {
    return boost::spirit::info{"int_token_parser"};
  }
};

template <typename tok_t>
struct token_parser : qi::primitive_parser<token_parser<tok_t>> {
  template <typename Context, typename Iterator>
  struct attribute {
    using type = boost::spirit::unused_type;
  };

  template <typename Iterator,
            typename Context,
            typename Skipper,
            typename Attribute>
  bool parse(Iterator& first,
             Iterator const& last,
             Context&,
             Skipper const&,
             Attribute&) const {
    if (first == last)
      return false;

    if (std::holds_alternative<tok_t>(*first)) {
      ++first;
      return true;
    }
    return false;
  }

  template <typename Context>
  boost::spirit::info what(Context&) const {
    return boost::spirit::info{"token_parser"};
  }
};

}  // namespace
// -----------------------------------------------------------------------

template <typename Iterator>
struct expr_grammar : qi::grammar<Iterator, std::shared_ptr<ExprAST>()> {
  expr_grammar() : expr_grammar::base_type(expression_rule) {
    expression_rule =
        (int_token >> token_parser<typename tok::Plus>() >>
         int_token)[qi::_val = phx::construct<std::shared_ptr<ExprAST>>(
                        phx::new_<ExprAST>(phx::construct<BinaryExpr>(
                            Op::Add,
                            phx::construct<std::shared_ptr<ExprAST>>(
                                phx::new_<ExprAST>(qi::_1)),
                            phx::construct<std::shared_ptr<ExprAST>>(
                                phx::new_<ExprAST>(qi::_2)))))];
  }

  qi::rule<Iterator, int()> int_token = int_token_parser();

  qi::rule<Iterator, Op()> precedence_add_token;   // + -
  qi::rule<Iterator, Op()> precedence_mult_token;  // * / %

  qi::rule<Iterator, std::shared_ptr<ExprAST>()> expression_rule;
};

std::shared_ptr<ExprAST> ParseExpression(std::span<Token> input) {
  using iterator_t = typename std::span<Token>::iterator;
  iterator_t begin = input.begin(), end = input.end();

  std::shared_ptr<ExprAST> result = nullptr;
  expr_grammar<iterator_t> grammar;

  bool success = qi::parse(begin, end, grammar, result);

  if (!success || begin != end)
    throw std::runtime_error("parsing failed.");
  return result;
}
