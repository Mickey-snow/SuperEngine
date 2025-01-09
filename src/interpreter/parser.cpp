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
  using attribute_type = int;  // The parser synthesizes an int.

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

// Parser that matches a specific token type and ignores it.
template <typename Tok>
struct token_parser : x3::parser<token_parser<Tok>> {
  using attribute_type = x3::unused_type;

  template <typename Iterator,
            typename Context,
            typename RContext,
            typename Attribute>
  bool parse(Iterator& first,
             Iterator const& last,
             Context const& /*context*/,
             RContext& /*rcontext*/,
             Attribute& /*attr*/) const {
    if (first == last)
      return false;

    if (std::holds_alternative<Tok>(*first)) {
      ++first;
      return true;
    }
    return false;
  }
};
}  // namespace
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// X3 rule declarations
// -----------------------------------------------------------------------
x3::rule<struct int_token_rule, int> const int_token = "int_token";
x3::rule<struct multiplicative_rule, Op> const prec_mul_token =
    "precedence_multiplicative_token";
x3::rule<struct additive_rule, Op> const prec_add_token =
    "precedence_additive_token";
x3::rule<struct expression_rule_class, std::shared_ptr<ExprAST>> const
    expression_rule = "expression_rule";

// -----------------------------------------------------------------------
// Rule definitions
// -----------------------------------------------------------------------
[[maybe_unused]] auto const int_token_def = int_token_parser();

[[maybe_unused]] auto const prec_mul_token_def =
    (token_parser<tok::Mult>()[([](auto& ctx) { x3::_val(ctx) = Op::Mul; })] |
     token_parser<tok::Div>()[([](auto& ctx) { x3::_val(ctx) = Op::Div; })]);
[[maybe_unused]] auto const prec_add_token_def =
    (token_parser<tok::Plus>()[([](auto& ctx) { x3::_val(ctx) = Op::Add; })] |
     token_parser<tok::Minus>()[([](auto& ctx) { x3::_val(ctx) = Op::Sub; })]);

[[maybe_unused]] auto const expression_rule_def =
    (int_token >> prec_mul_token >> int_token)[([](auto& ctx) {
      const auto& attr = x3::_attr(ctx);
      auto lhs = boost::fusion::at_c<0>(attr);
      auto op = boost::fusion::at_c<1>(attr);
      auto rhs = boost::fusion::at_c<2>(attr);

      auto lhs_ast = std::make_shared<ExprAST>(lhs);
      auto rhs_ast = std::make_shared<ExprAST>(rhs);

      auto binary_expr = BinaryExpr(op, lhs_ast, rhs_ast);
      x3::_val(ctx) = std::make_shared<ExprAST>(binary_expr);
    })] |
    (int_token >> prec_add_token >> int_token)[([](auto& ctx) {
      const auto& attr = x3::_attr(ctx);
      auto lhs = boost::fusion::at_c<0>(attr);
      auto op = boost::fusion::at_c<1>(attr);
      auto rhs = boost::fusion::at_c<2>(attr);

      auto lhs_ast = std::make_shared<ExprAST>(lhs);
      auto rhs_ast = std::make_shared<ExprAST>(rhs);

      auto binary_expr = BinaryExpr(op, lhs_ast, rhs_ast);
      x3::_val(ctx) = std::make_shared<ExprAST>(binary_expr);
    })];

BOOST_SPIRIT_DEFINE(int_token, expression_rule, prec_mul_token, prec_add_token);

// -----------------------------------------------------------------------
// Parse function
// -----------------------------------------------------------------------
std::shared_ptr<ExprAST> ParseExpression(std::span<Token> input) {
  using iterator_t = std::span<Token>::iterator;
  iterator_t begin = input.begin();
  iterator_t end = input.end();

  std::shared_ptr<ExprAST> result;
  // Attempt to parse the entire input with our expression_rule
  bool success = x3::parse(begin, end, expression_rule, result);

  if (!success || begin != end)
    throw std::runtime_error("parsing failed.");

  return result;
}
