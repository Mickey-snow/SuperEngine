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
// Parser that matches a string token (tok::ID) and returns its value.
struct str_token_parser : x3::parser<str_token_parser> {
  using attribute_type = std::string;

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

    if (auto p = std::get_if<tok::ID>(&*first)) {
      attr = p->id;
      ++first;
      return true;
    }
    return false;
  }
};

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

    if (auto p = std::get_if<tok::Int>(&*first)) {
      attr = p->value;
      ++first;
      return true;
    }
    return false;
  }
};

// Parser that matches a specified sequence of token
template <typename R>
struct seq_token_parser : x3::parser<seq_token_parser<R>> {
  using attribute_type = R;

  seq_token_parser(R attribute, std::vector<Token> tok)
      : attribute_(attribute), tok_seq_(std::move(tok)) {}
  R attribute_;
  std::vector<Token> tok_seq_;

  template <typename Iterator,
            typename Context,
            typename RContext,
            typename Attribute>
  bool parse(Iterator& first,
             Iterator const& last,
             Context const& /*context*/,
             RContext& /*rcontext*/,
             Attribute& attr) const {
    for (std::size_t i = 0; i < tok_seq_.size(); ++i)
      if (first + i == last || first[i] != tok_seq_[i])
        return false;

    first += tok_seq_.size();
    attr = attribute_;
    return true;
  }
};
}  // namespace
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// Expression rule declarations
// -----------------------------------------------------------------------
x3::rule<struct primary_expr_rule, std::shared_ptr<ExprAST>> const
    primary_expr = "primary_expr";

x3::rule<struct unary_expr_rule, std::shared_ptr<ExprAST>> const unary_expr =
    "unary_expr";
x3::rule<struct multiplicative_expr_rule, std::shared_ptr<ExprAST>> const
    multiplicative_expr = "multiplicative_expr";
x3::rule<struct additive_expr_rule, std::shared_ptr<ExprAST>> const
    additive_expr = "additive_expr";
x3::rule<struct shift_expr_rule, std::shared_ptr<ExprAST>> const shift_expr =
    "shift_expr";
x3::rule<struct relational_expr_rule, std::shared_ptr<ExprAST>> const
    relational_expr = "relational_expr";
x3::rule<struct equality_expr_rule, std::shared_ptr<ExprAST>> const
    equality_expr = "equality_expr";
x3::rule<struct bitwise_and_expr_rule, std::shared_ptr<ExprAST>> const
    bitwise_and_expr = "bitwise_and_expr";
x3::rule<struct bitwise_xor_expr_rule, std::shared_ptr<ExprAST>> const
    bitwise_xor_expr = "bitwise_xor_expr";
x3::rule<struct bitwise_or_expr_rule, std::shared_ptr<ExprAST>> const
    bitwise_or_expr = "bitwise_or_expr";
x3::rule<struct logical_and_expr_rule, std::shared_ptr<ExprAST>> const
    logical_and_expr = "logical_and_expr";
x3::rule<struct logical_or_expr_rule, std::shared_ptr<ExprAST>> const
    logical_or_expr = "logical_or_expr";
x3::rule<struct assignment_expr_rule, std::shared_ptr<ExprAST>> const
    assignment_expr = "assignment_expr";
x3::rule<struct expression_rule_class, std::shared_ptr<ExprAST>> const
    expression_rule = "expression_rule";

// -----------------------------------------------------------------------
// Rule definitions
// -----------------------------------------------------------------------

namespace {
struct construct_ast {
  std::shared_ptr<ExprAST> operator()(auto& ctx) {
    const auto& attr = x3::_attr(ctx);
    std::shared_ptr<ExprAST> result = boost::fusion::at_c<0>(attr);  // lhs

    // an array of (op,ast)
    for (const auto& it : boost::fusion::at_c<1>(attr)) {
      Op op = Op::Unknown;
      const auto& first = boost::fusion::at_c<0>(it);
      std::shared_ptr<ExprAST> rhs = boost::fusion::at_c<1>(it);  // second

      if constexpr (std::same_as<Op, std::decay_t<decltype(first)>>)
        op = first;
      else
        op = boost::apply_visitor([](const auto& x) { return x; }, first);

      BinaryExpr expr(op, result, rhs);
      result = std::make_shared<ExprAST>(std::move(expr));
    }

    x3::_val(ctx) = result;
    return result;
  }
} construct_ast_fn;
}  // namespace

[[maybe_unused]] auto const expression_rule_def =
    (assignment_expr >> *(seq_token_parser(Op::Comma, {tok::Comma()}) >>
                          assignment_expr))[construct_ast_fn];

[[maybe_unused]] auto const assignment_expr_def = logical_or_expr;

[[maybe_unused]] auto const logical_or_expr_def = logical_and_expr;

[[maybe_unused]] auto const logical_and_expr_def = bitwise_or_expr;

[[maybe_unused]] auto const bitwise_or_expr_def = bitwise_xor_expr;

[[maybe_unused]] auto const bitwise_xor_expr_def = bitwise_and_expr;

[[maybe_unused]] auto const bitwise_and_expr_def = equality_expr;

[[maybe_unused]] auto const equality_expr_def =
    (relational_expr >>
     *((seq_token_parser(Op::Equal, {tok::Eq(), tok::Eq()}) |
        seq_token_parser(Op::NotEqual, {tok::Exclam(), tok::Eq()})) >>
       relational_expr))[construct_ast_fn];

[[maybe_unused]] auto const relational_expr_def =
    (shift_expr >>
     *((seq_token_parser(Op::LessEqual, {tok::AngleL(), tok::Eq()}) |
        seq_token_parser(Op::Less, {tok::AngleL()}) |
        seq_token_parser(Op::GreaterEqual, {tok::AngleR(), tok::Eq()}) |
        seq_token_parser(Op::Greater, {tok::AngleR()})) >>
       shift_expr))[construct_ast_fn];

[[maybe_unused]] auto const shift_expr_def = additive_expr;

[[maybe_unused]] auto const additive_expr_def =
    (multiplicative_expr >> *((seq_token_parser(Op::Add, {tok::Plus()}) |
                               seq_token_parser(Op::Sub, {tok::Minus()})) >>
                              multiplicative_expr))[construct_ast_fn];

[[maybe_unused]] auto const multiplicative_expr_def =
    (unary_expr >> *((seq_token_parser(Op::Mul, {tok::Mult()}) |
                      seq_token_parser(Op::Div, {tok::Div()})) >>
                     unary_expr))[construct_ast_fn];

[[maybe_unused]] auto const unary_expr_def = primary_expr;

[[maybe_unused]] auto const primary_expr_def =
    int_token_parser()[([](auto& ctx) {
      x3::_val(ctx) = std::make_shared<ExprAST>(x3::_attr(ctx));
    })] |  // integer literal

    (str_token_parser() >>
     -(seq_token_parser(Op::Unknown, {tok::SquareL()}) >> expression_rule >>
       seq_token_parser(Op::Unknown, {tok::SquareR()})))[([](auto& ctx) {
      const auto& attr = x3::_attr(ctx);
      const std::string& id = boost::fusion::at_c<0>(attr);
      const auto& remain = boost::fusion::at_c<1>(
          attr);  // boost::optional<boost::fusion::deque<Op, ptr<ExprAST>, OP>>
      if (remain.has_value()) {
        auto idx_ast = boost::fusion::at_c<1>(remain.value());
        x3::_val(ctx) = std::make_shared<ExprAST>(ReferenceExpr(id, idx_ast));
      } else {
        x3::_val(ctx) = std::make_shared<ExprAST>(id);
      }
    })] |  // memory reference

    (seq_token_parser(Op::Unknown, {tok::ParenthesisL()}) >> expression_rule >>
     seq_token_parser(Op::Unknown, {tok::ParenthesisR()}))[([](auto& ctx) {
      const auto& attr = x3::_attr(ctx);
      std::shared_ptr<ExprAST> sub = boost::fusion::at_c<1>(attr);
      ParenExpr expr(sub);
      x3::_val(ctx) = std::make_shared<ExprAST>(std::move(expr));
    })];  // ( <expr> )

BOOST_SPIRIT_DEFINE(primary_expr,
                    assignment_expr,
                    unary_expr,
                    shift_expr,
                    relational_expr,
                    expression_rule,
                    equality_expr,
                    bitwise_and_expr,
                    bitwise_xor_expr,
                    bitwise_or_expr,
                    logical_and_expr,
                    logical_or_expr,
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
