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

#include "core/expr_ast.hpp"
#include "interpreter/parsing_error.hpp"

#include <boost/fusion/include/deque.hpp>
#include <boost/spirit/home/x3.hpp>

#include <iterator>
#include <sstream>
#include <unordered_set>

namespace x3 = boost::spirit::x3;

// Helper function to skip over tok::WS
template <std::forward_iterator Iterator>
inline void skip_ws(Iterator& first, Iterator const& last) {
  while (first != last && std::holds_alternative<tok::WS>(*first))
    ++first;
}

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
    skip_ws(first, last);
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
    skip_ws(first, last);
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

// Parser that matches an operator token (tok::Operator) and returns its
// operator.
struct op_token_parser : x3::parser<op_token_parser> {
  using attribute_type = Op;

  op_token_parser(std::initializer_list<Op> accepted_operators)
      : op_table_(accepted_operators) {}
  std::unordered_set<Op> op_table_;

  template <typename Iterator,
            typename Context,
            typename RContext,
            typename Attribute>
  bool parse(Iterator& first,
             Iterator const& last,
             Context const& /*context*/,
             RContext& /*rcontext*/,
             Attribute& attr) const {
    skip_ws(first, last);
    attr = Op::Unknown;

    if (first == last)
      return false;

    if (auto p = std::get_if<tok::Operator>(&*first)) {
      if (!op_table_.empty() && !op_table_.contains(p->op))
        return false;

      attr = p->op;
      ++first;
      return true;
    }
    return false;
  }
};

// Parser that matches a specified type of token.
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
    skip_ws(first, last);
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
      Op op = boost::fusion::at_c<0>(it);
      std::shared_ptr<ExprAST> rhs = boost::fusion::at_c<1>(it);

      BinaryExpr expr(op, result, rhs);
      result = std::make_shared<ExprAST>(std::move(expr));
    }

    x3::_val(ctx) = result;
    return result;
  }
} construct_ast_fn;
}  // namespace

[[maybe_unused]] auto const expression_rule_def =
    (assignment_expr >>
     *(op_token_parser{Op::Comma} >> assignment_expr))[construct_ast_fn];

// note: associativity should be right-to-left
[[maybe_unused]] auto const assignment_expr_def =
    (logical_or_expr >>
     *(op_token_parser{Op::Assign, Op::AddAssign, Op::SubAssign, Op::MulAssign,
                       Op::DivAssign, Op::ModAssign, Op::BitAndAssign,
                       Op::BitOrAssign, Op::BitXorAssign, Op::ShiftLeftAssign,
                       Op::ShiftRightAssign} >>
       logical_or_expr))[([](auto& ctx) {
      const auto& attr = x3::_attr(ctx);
      std::vector<std::shared_ptr<ExprAST>> terms{boost::fusion::at_c<0>(attr)};
      std::vector<Op> operators;

      // an array of (op,ast)
      for (const auto& it : boost::fusion::at_c<1>(attr)) {
        Op op = boost::fusion::at_c<0>(it);
        std::shared_ptr<ExprAST> term = boost::fusion::at_c<1>(it);

        terms.push_back(term);
        operators.push_back(op);
      }

      std::shared_ptr<ExprAST> result = terms.back();
      terms.pop_back();
      while (!terms.empty()) {
        auto lhs = terms.back();
        terms.pop_back();
        auto op = operators.back();
        operators.pop_back();

        BinaryExpr expr(op, lhs, result);
        result = std::make_shared<ExprAST>(std::move(expr));
      }
      x3::_val(ctx) = result;
    })];

[[maybe_unused]] auto const logical_or_expr_def =
    (logical_and_expr >>
     *(op_token_parser{Op::LogicalOr} >> logical_and_expr))[construct_ast_fn];

[[maybe_unused]] auto const logical_and_expr_def =
    (bitwise_or_expr >>
     *(op_token_parser{Op::LogicalAnd} >> bitwise_or_expr))[construct_ast_fn];

[[maybe_unused]] auto const bitwise_or_expr_def =
    (bitwise_xor_expr >>
     *(op_token_parser{Op::BitOr} >> bitwise_xor_expr))[construct_ast_fn];

[[maybe_unused]] auto const bitwise_xor_expr_def =
    (bitwise_and_expr >>
     *(op_token_parser{Op::BitXor} >> bitwise_and_expr))[construct_ast_fn];

[[maybe_unused]] auto const bitwise_and_expr_def =
    (equality_expr >>
     *(op_token_parser{Op::BitAnd} >> equality_expr))[construct_ast_fn];

[[maybe_unused]] auto const equality_expr_def =
    (relational_expr >> *(op_token_parser{Op::Equal, Op::NotEqual} >>
                          relational_expr))[construct_ast_fn];

[[maybe_unused]] auto const relational_expr_def =
    (shift_expr >> *(op_token_parser{Op::LessEqual, Op::Less, Op::GreaterEqual,
                                     Op::Greater} >>
                     shift_expr))[construct_ast_fn];

[[maybe_unused]] auto const shift_expr_def =
    (additive_expr >> *(op_token_parser{Op::ShiftLeft, Op::ShiftRight} >>
                        additive_expr))[construct_ast_fn];

[[maybe_unused]] auto const additive_expr_def =
    (multiplicative_expr >> *(op_token_parser{Op::Add, Op::Sub} >>
                              multiplicative_expr))[construct_ast_fn];

[[maybe_unused]] auto const multiplicative_expr_def =
    (unary_expr >> *(op_token_parser{Op::Mul, Op::Div, Op::Mod} >>
                     unary_expr))[construct_ast_fn];

[[maybe_unused]] auto const unary_expr_def =
    (*op_token_parser{Op::Tilde, Op::Sub, Op::Add} >>
     primary_expr)[([](auto& ctx) {
      const auto& attr = x3::_attr(ctx);
      std::shared_ptr<ExprAST> ast = boost::fusion::at_c<1>(attr);

      const auto& op_arr = boost::fusion::at_c<0>(attr);  // type is vector<Op>
      for (auto it = op_arr.crbegin(); it != op_arr.crend(); ++it) {
        UnaryExpr expr(*it, ast);
        ast = std::make_shared<ExprAST>(std::move(expr));
      }
      x3::_val(ctx) = ast;
    })];

[[maybe_unused]] auto const primary_expr_def =
    int_token_parser()[([](auto& ctx) {
      x3::_val(ctx) = std::make_shared<ExprAST>(x3::_attr(ctx));
    })] |  // integer literal

    (str_token_parser() >> -(token_parser<tok::SquareL>() >> expression_rule >>
                             token_parser<tok::SquareR>()))[([](auto& ctx) {
      const auto& attr = x3::_attr(ctx);
      const std::string& id = boost::fusion::at_c<0>(attr);
      const auto& remain = boost::fusion::at_c<1>(
          attr);  // boost::optional<boost::fusion::deque<Op, ptr<ExprAST>, OP>>
      if (remain.has_value()) {
        auto idx_ast = remain.value();
        x3::_val(ctx) = std::make_shared<ExprAST>(ReferenceExpr(id, idx_ast));
      } else {
        x3::_val(ctx) = std::make_shared<ExprAST>(id);
      }
    })] |  // memory reference

    (token_parser<tok::ParenthesisL>() >> expression_rule >>
     token_parser<tok::ParenthesisR>())[([](auto& ctx) {
      std::shared_ptr<ExprAST> sub = x3::_attr(ctx);
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

  if (!success) {
    // Parsing failed somewhere in the input.
    std::ptrdiff_t index = std::distance(input.begin(), begin);
    std::ostringstream oss;
    oss << "Parsing failed at token index " << index;

    if (begin != end) {
      oss << " (near " << std::visit(tok::DebugStringVisitor(), *begin) << ")";
    } else {
      oss << " (reached end of input unexpectedly)";
    }

    throw ParsingError(oss.str());
  }

  skip_ws(begin, end);  // consume leftover white spaces
  if (begin != end) {
    // Some tokens left unconsumed after a successful parse.
    std::ptrdiff_t index = std::distance(input.begin(), begin);
    std::ostringstream oss;
    oss << "Parsing did not consume all tokens. Leftover begins at index "
        << index << " with token "
        << std::visit(tok::DebugStringVisitor(), *begin);
    throw ParsingError(oss.str());
  }

  return result;
}
