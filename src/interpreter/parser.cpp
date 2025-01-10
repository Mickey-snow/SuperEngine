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

#include <boost/fusion/include/deque.hpp>
#include <boost/spirit/home/x3.hpp>

namespace x3 = boost::spirit::x3;

// -----------------------------------------------------------------------
// primitive token parsers
namespace {
// Parser that matches an integer token (tok::Int) and returns its value.
struct int_token_parser : x3::parser<int_token_parser> {
  using attribute_type = int;

  template <typename Iterator,
            typename Context,
            typename RContext,
            typename Attribute>
  bool parse(Iterator& first,
             Iterator const& last,
             Context const& /*context*/,
             RContext& /*rcontext*/,
             Attribute& attr) const {
    if (first == last)
      return false;

    // Check if the current token is tok::Int
    if (auto p = std::get_if<tok::Int>(&*first)) {
      // Move integer value into the attribute
      x3::traits::move_to(p->value, attr);
      ++first;
      return true;
    }
    return false;
  }
};

// Parser that matches a specific token type.
template <typename Tok>
struct token_parser : x3::parser<token_parser<Tok>> {
  using attribute_type = Op;

  template <typename Iterator,
            typename Context,
            typename RContext,
            typename Attribute>
  bool parse(Iterator& first,
             Iterator const& last,
             Context const& /*context*/,
             RContext& /*rcontext*/,
             Attribute& attr) const {
    attr = Op::Unknown;
    if (first == last)
      return false;

    if (std::holds_alternative<Tok>(*first)) {
      if constexpr (std::same_as<Tok, tok::Plus>)
        attr = Op::Add;
      else if constexpr (std::same_as<Tok, tok::Minus>)
        attr = Op ::Sub;
      else if constexpr (std::same_as<Tok, tok::Mult>)
        attr = Op::Mul;
      else if constexpr (std::same_as<Tok, tok::Div>)
        attr = Op::Div;

      ++first;
      return true;
    }
    return false;
  }
};
}  // namespace
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// Expression rule declarations
// -----------------------------------------------------------------------
x3::rule<struct primary_expr_rule, std::shared_ptr<ExprAST>> const
    primary_expr = "primary_expr";
x3::rule<struct multiplicative_expr_rule, std::shared_ptr<ExprAST>> const
    multiplicative_expr = "multiplicative_expr";
x3::rule<struct additive_expr_rule, std::shared_ptr<ExprAST>> const
    additive_expr = "additive_expr";
x3::rule<struct expression_rule_class, std::shared_ptr<ExprAST>> const
    expression_rule = "expression_rule";

// -----------------------------------------------------------------------
// Rule definitions
// -----------------------------------------------------------------------
[[maybe_unused]] auto const primary_expr_def =
    int_token_parser()[([](auto& ctx) {
      x3::_val(ctx) = std::make_shared<ExprAST>(x3::_attr(ctx));
    })];

[[maybe_unused]] auto const expression_rule_def = additive_expr;

struct construct_ast {
  std::shared_ptr<ExprAST> operator()(auto& ctx) {
    const auto& attr = x3::_attr(ctx);
    std::shared_ptr<ExprAST> result = boost::fusion::at_c<0>(attr);
    for (const auto& it : boost::fusion::at_c<1>(attr)) {
      Op op = boost::apply_visitor([](auto& x) { return x; },
                                   boost::fusion::at_c<0>(it));
      std::shared_ptr<ExprAST> rhs = boost::fusion::at_c<1>(it);
      BinaryExpr expr(op, result, rhs);
      result = std::make_shared<ExprAST>(std::move(expr));
    }

    x3::_val(ctx) = result;
    return result;
  }
};

[[maybe_unused]] auto const additive_expr_def =
    (multiplicative_expr >>
     *((token_parser<tok::Plus>() | token_parser<tok::Minus>()) >>
       multiplicative_expr))[construct_ast()];

[[maybe_unused]] auto const multiplicative_expr_def =
    (primary_expr >> *((token_parser<tok::Mult>() | token_parser<tok::Div>()) >>
                       primary_expr))[construct_ast()];

BOOST_SPIRIT_DEFINE(primary_expr,
                    expression_rule,
                    multiplicative_expr,
                    additive_expr);

// -----------------------------------------------------------------------
// Parse function
// -----------------------------------------------------------------------
std::shared_ptr<ExprAST> ParseExpression(std::span<Token> input) {
  using iterator_t = std::span<Token>::iterator;
  iterator_t begin = input.begin();
  iterator_t end = input.end();

  std::shared_ptr<ExprAST> result;
  bool success = x3::parse(begin, end, expression_rule, result);

  if (!success || begin != end)
    throw std::runtime_error("parsing failed.");

  return result;
}
