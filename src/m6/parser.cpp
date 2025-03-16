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

#include "m6/parser.hpp"
#include "m6/expr_ast.hpp"
#include "m6/parsing_error.hpp"
#include "machine/op.hpp"

#include <iterator>
#include <optional>
#include <sstream>
#include <unordered_set>

namespace m6 {

using iterator_t = std::span<Token>::iterator;

// Helper to skip whitespace tokens in-place.
static void skipWS(iterator_t& it, iterator_t end) {
  while (it != end && std::holds_alternative<tok::WS>(*it)) {
    ++it;
  }
}

// Handy checker function for matching an operator from a specified set.
static std::optional<Op> tryConsumeAny(iterator_t& it,
                                       iterator_t end,
                                       std::initializer_list<Op> ops) {
  skipWS(it, end);
  if (it == end) {
    return std::nullopt;
  }

  if (auto p = std::get_if<tok::Operator>(&*it)) {
    for (auto candidate : ops)
      if (candidate == p->op) {
        ++it;
        return p->op;
      }
  }
  return std::nullopt;
}

// Handy checker function for matching a single operator token.
static bool tryConsumeOp(iterator_t& it, iterator_t end, Op op) {
  skipWS(it, end);
  if (it == end) {
    return false;
  }
  if (auto p = std::get_if<tok::Operator>(&*it)) {
    if (p->op == op) {
      ++it;
      return true;
    }
  }
  return false;
}

// Handy checker function for matching a token type T.
template <typename T>
static bool tryConsumeToken(iterator_t& it, iterator_t end) {
  skipWS(it, end);
  if (it == end) {
    return false;
  }
  if (std::holds_alternative<T>(*it)) {
    ++it;
    return true;
  }
  return false;
}

// Forward declarations of recursive parsing functions.
static std::shared_ptr<ExprAST> parseExpression(iterator_t& it, iterator_t end);
static std::shared_ptr<ExprAST> parseAssignment(iterator_t& it, iterator_t end);
static std::shared_ptr<ExprAST> parseLogicalOr(iterator_t& it, iterator_t end);
static std::shared_ptr<ExprAST> parseLogicalAnd(iterator_t& it, iterator_t end);
static std::shared_ptr<ExprAST> parseBitwiseOr(iterator_t& it, iterator_t end);
static std::shared_ptr<ExprAST> parseBitwiseXor(iterator_t& it, iterator_t end);
static std::shared_ptr<ExprAST> parseBitwiseAnd(iterator_t& it, iterator_t end);
static std::shared_ptr<ExprAST> parseEquality(iterator_t& it, iterator_t end);
static std::shared_ptr<ExprAST> parseRelational(iterator_t& it, iterator_t end);
static std::shared_ptr<ExprAST> parseShift(iterator_t& it, iterator_t end);
static std::shared_ptr<ExprAST> parseAdditive(iterator_t& it, iterator_t end);
static std::shared_ptr<ExprAST> parseMultiplicative(iterator_t& it,
                                                    iterator_t end);
static std::shared_ptr<ExprAST> parseUnary(iterator_t& it, iterator_t end);
static std::shared_ptr<ExprAST> parsePostfix(iterator_t& it, iterator_t end);
static std::shared_ptr<ExprAST> parsePrimary(iterator_t& it, iterator_t end);

//=================================================================================
// parseExpression() - top-level parse for expressions, handling comma.
//     expression -> assignment_expr (',' assignment_expr)*
//=================================================================================
static std::shared_ptr<ExprAST> parseExpression(iterator_t& it,
                                                iterator_t end) {
  auto lhs = parseAssignment(it, end);
  // Now handle comma operators, left-associative.
  while (true) {
    skipWS(it, end);
    if (!tryConsumeOp(it, end, Op::Comma)) {
      break;
    }
    auto rhs = parseAssignment(it, end);
    BinaryExpr expr(Op::Comma, lhs, rhs);
    lhs = std::make_shared<ExprAST>(std::move(expr));
  }
  return lhs;
}

//=================================================================================
// parseAssignment() - right-associative assignment ops.
//     assignment_expr -> logical_or_expr ( ASSIGN_OP assignment_expr )*
//     where ASSIGN_OP is one of =, +=, -=, etc.
//=================================================================================
static std::shared_ptr<ExprAST> parseAssignment(iterator_t& it,
                                                iterator_t end) {
  auto lhs = parseLogicalOr(it, end);

  // We can do repeated assignment in a right-associative manner:
  // We'll use a loop that rebinds 'lhs' from the right side.
  while (true) {
    skipWS(it, end);
    auto matched =
        tryConsumeAny(it, end,
                      {Op::Assign, Op::AddAssign, Op::SubAssign, Op::MulAssign,
                       Op::DivAssign, Op::ModAssign, Op::BitAndAssign,
                       Op::BitOrAssign, Op::BitXorAssign, Op::ShiftLeftAssign,
                       Op::ShiftRightAssign, Op::ShiftUnsignedRightAssign});
    if (!matched.has_value())
      break;
    auto assignmentOp = matched.value();

    // Parse the right-hand side via the same parseAssignment for R->L
    // associativity.
    auto rhs = parseAssignment(it, end);
    if (assignmentOp == Op::Assign) {
      // simple assignment
      lhs = std::make_shared<ExprAST>(AssignExpr(lhs, rhs));
    } else {
      // compound assignment
      BinaryExpr expr(assignmentOp, lhs, rhs);
      lhs = std::make_shared<ExprAST>(std::move(expr));
    }
  }

  return lhs;
}

//=================================================================================
// parseLogicalOr() - left-associative '||'.
//     logical_or_expr -> logical_and_expr ( '||' logical_and_expr )*
//=================================================================================
static std::shared_ptr<ExprAST> parseLogicalOr(iterator_t& it, iterator_t end) {
  auto lhs = parseLogicalAnd(it, end);
  while (true) {
    skipWS(it, end);
    if (!tryConsumeOp(it, end, Op::LogicalOr)) {
      break;
    }
    auto rhs = parseLogicalAnd(it, end);
    BinaryExpr expr(Op::LogicalOr, lhs, rhs);
    lhs = std::make_shared<ExprAST>(std::move(expr));
  }
  return lhs;
}

//=================================================================================
// parseLogicalAnd() - left-associative '&&'.
//     logical_and_expr -> bitwise_or_expr ( '&&' bitwise_or_expr )*
//=================================================================================
static std::shared_ptr<ExprAST> parseLogicalAnd(iterator_t& it,
                                                iterator_t end) {
  auto lhs = parseBitwiseOr(it, end);
  while (true) {
    skipWS(it, end);
    if (!tryConsumeOp(it, end, Op::LogicalAnd)) {
      break;
    }
    auto rhs = parseBitwiseOr(it, end);
    BinaryExpr expr(Op::LogicalAnd, lhs, rhs);
    lhs = std::make_shared<ExprAST>(std::move(expr));
  }
  return lhs;
}

//=================================================================================
// parseBitwiseOr() - left-associative '|'.
//     bitwise_or_expr -> bitwise_xor_expr ( '|' bitwise_xor_expr )*
//=================================================================================
static std::shared_ptr<ExprAST> parseBitwiseOr(iterator_t& it, iterator_t end) {
  auto lhs = parseBitwiseXor(it, end);
  while (true) {
    skipWS(it, end);
    if (!tryConsumeOp(it, end, Op::BitOr)) {
      break;
    }
    auto rhs = parseBitwiseXor(it, end);
    BinaryExpr expr(Op::BitOr, lhs, rhs);
    lhs = std::make_shared<ExprAST>(std::move(expr));
  }
  return lhs;
}

//=================================================================================
// parseBitwiseXor() - left-associative '^'.
//     bitwise_xor_expr -> bitwise_and_expr ( '^' bitwise_and_expr )*
//=================================================================================
static std::shared_ptr<ExprAST> parseBitwiseXor(iterator_t& it,
                                                iterator_t end) {
  auto lhs = parseBitwiseAnd(it, end);
  while (true) {
    skipWS(it, end);
    if (!tryConsumeOp(it, end, Op::BitXor))
      break;

    auto rhs = parseBitwiseAnd(it, end);
    BinaryExpr expr(Op::BitXor, lhs, rhs);
    lhs = std::make_shared<ExprAST>(std::move(expr));
  }
  return lhs;
}

//=================================================================================
// parseBitwiseAnd() - left-associative '&'.
//     bitwise_and_expr -> equality_expr ( '&' equality_expr )*
//=================================================================================
static std::shared_ptr<ExprAST> parseBitwiseAnd(iterator_t& it,
                                                iterator_t end) {
  auto lhs = parseEquality(it, end);
  while (true) {
    skipWS(it, end);
    if (!tryConsumeOp(it, end, Op::BitAnd))
      break;

    auto rhs = parseEquality(it, end);
    BinaryExpr expr(Op::BitAnd, lhs, rhs);
    lhs = std::make_shared<ExprAST>(std::move(expr));
  }
  return lhs;
}

//=================================================================================
// parseEquality() - left-associative '==' and '!='.
//     equality_expr -> relational_expr ( ('=='|'!=') relational_expr )*
//=================================================================================
static std::shared_ptr<ExprAST> parseEquality(iterator_t& it, iterator_t end) {
  auto lhs = parseRelational(it, end);
  while (true) {
    skipWS(it, end);
    auto match = tryConsumeAny(it, end, {Op::Equal, Op::NotEqual});
    if (!match.has_value())
      break;

    auto op = match.value();
    auto rhs = parseRelational(it, end);
    BinaryExpr expr(op, lhs, rhs);
    lhs = std::make_shared<ExprAST>(std::move(expr));
  }
  return lhs;
}

//=================================================================================
// parseRelational() - left-associative '<', '<=', '>', '>='.
//     relational_expr -> shift_expr ( ('<'|'<='|'>'|'>=') shift_expr )*
//=================================================================================
static std::shared_ptr<ExprAST> parseRelational(iterator_t& it,
                                                iterator_t end) {
  auto lhs = parseShift(it, end);
  while (true) {
    skipWS(it, end);
    auto match = tryConsumeAny(
        it, end, {Op::Less, Op::LessEqual, Op::Greater, Op::GreaterEqual});
    if (!match.has_value())
      break;

    auto op = match.value();
    auto rhs = parseShift(it, end);
    BinaryExpr expr(op, lhs, rhs);
    lhs = std::make_shared<ExprAST>(std::move(expr));
  }
  return lhs;
}

//=================================================================================
// parseShift() - left-associative '<<', '>>', '>>>'.
//     shift_expr -> additive_expr ( ('<<'|'>>'|'>>>') additive_expr )*
//=================================================================================
static std::shared_ptr<ExprAST> parseShift(iterator_t& it, iterator_t end) {
  auto lhs = parseAdditive(it, end);
  while (true) {
    skipWS(it, end);
    auto match = tryConsumeAny(
        it, end, {Op::ShiftLeft, Op::ShiftRight, Op::ShiftUnsignedRight});
    if (!match.has_value())
      break;

    auto op = match.value();
    auto rhs = parseAdditive(it, end);
    BinaryExpr expr(op, lhs, rhs);
    lhs = std::make_shared<ExprAST>(std::move(expr));
  }
  return lhs;
}

//=================================================================================
// parseAdditive() - left-associative '+' and '-'.
//     additive_expr -> multiplicative_expr ( ('+'|'-') multiplicative_expr )*
//=================================================================================
static std::shared_ptr<ExprAST> parseAdditive(iterator_t& it, iterator_t end) {
  auto lhs = parseMultiplicative(it, end);
  while (true) {
    skipWS(it, end);
    auto match = tryConsumeAny(it, end, {Op::Add, Op::Sub});
    if (!match.has_value())
      break;

    auto op = match.value();
    auto rhs = parseMultiplicative(it, end);
    BinaryExpr expr(op, lhs, rhs);
    lhs = std::make_shared<ExprAST>(std::move(expr));
  }
  return lhs;
}

//=================================================================================
// parseMultiplicative() - left-associative '*', '/', '%'.
//     multiplicative_expr -> unary_expr ( ('*'|'/'|'%') unary_expr )*
//=================================================================================
static std::shared_ptr<ExprAST> parseMultiplicative(iterator_t& it,
                                                    iterator_t end) {
  auto lhs = parseUnary(it, end);
  while (true) {
    skipWS(it, end);
    auto match = tryConsumeAny(it, end, {Op::Mul, Op::Div, Op::Mod});
    if (!match.has_value())
      break;

    auto op = match.value();
    auto rhs = parseUnary(it, end);
    BinaryExpr expr(op, lhs, rhs);
    lhs = std::make_shared<ExprAST>(std::move(expr));
  }
  return lhs;
}

//=================================================================================
// parseUnary() - handles unary ops like '+', '-', '~' (prefix).
//     unary_expr -> ( ('+'|'-'|'~') )* postfix_expr
// We accumulate multiple unary ops and apply them in right-to-left order.
//=================================================================================
static std::shared_ptr<ExprAST> parseUnary(iterator_t& it, iterator_t end) {
  // Collect all unary operators first.
  std::vector<Op> ops;
  while (true) {
    skipWS(it, end);
    auto match = tryConsumeAny(it, end, {Op::Add, Op::Sub, Op::Tilde});
    if (!match.has_value())
      break;

    ops.push_back(match.value());
  }

  // Parse the postfix expression that these unary ops apply to.
  auto node = parsePostfix(it, end);

  // Apply unary ops in reverse order (right-to-left).
  for (auto itOp = ops.rbegin(); itOp != ops.rend(); ++itOp) {
    UnaryExpr expr(*itOp, node);
    node = std::make_shared<ExprAST>(std::move(expr));
  }

  return node;
}

//=================================================================================
// parsePostfix() - handles function calls, member access, array subscripts.
//     postfix_expr -> primary_expr postfix_suffix*
//     postfix_suffix ->
//         '(' [expression] ')'        (function invocation)
//       | '.' id_token                (member access)
//       | '[' expression ']'          (subscript)
//=================================================================================
static std::shared_ptr<ExprAST> parsePostfix(iterator_t& it, iterator_t end) {
  auto lhs = parsePrimary(it, end);

  // Repeatedly parse postfix elements.
  while (true) {
    skipWS(it, end);

    // 1) function call: '(' [expression] ')'
    if (tryConsumeToken<tok::ParenthesisL>(it, end)) {
      // parse optional expression
      skipWS(it, end);
      std::shared_ptr<ExprAST> argExpr = nullptr;
      if (it != end && !std::holds_alternative<tok::ParenthesisR>(*it)) {
        // there's something inside, parse expression
        argExpr = parseExpression(it, end);
      }
      skipWS(it, end);
      if (!tryConsumeToken<tok::ParenthesisR>(it, end)) {
        throw ParsingError("Expected ')' after function call.");
      }
      lhs = std::make_shared<ExprAST>(InvokeExpr(lhs, argExpr));
      continue;
    }

    // 2) member access: '.' <identifier>
    if (tryConsumeOp(it, end, Op::Dot)) {
      skipWS(it, end);
      if (it == end || !std::holds_alternative<tok::ID>(*it)) {
        throw ParsingError("Expected identifier after '.'");
      }
      auto p = std::get_if<tok::ID>(&*it);
      IdExpr memberName(p->id);
      ++it;  // consume the ID token
      auto memberNode = std::make_shared<ExprAST>(std::move(memberName));
      lhs = std::make_shared<ExprAST>(MemberExpr(lhs, memberNode));
      continue;
    }

    // 3) subscript: '[' expression ']'
    if (tryConsumeToken<tok::SquareL>(it, end)) {
      auto indexExpr = parseExpression(it, end);
      if (!tryConsumeToken<tok::SquareR>(it, end)) {
        throw ParsingError("Expected ']' after subscript expression.");
      }
      lhs = std::make_shared<ExprAST>(SubscriptExpr(lhs, indexExpr));
      continue;
    }

    // if none of the above matched, we're done with postfix.
    break;
  }

  return lhs;
}

//=================================================================================
// parsePrimary() - int, string, id, or parenthesized expression.
//     primary_expr -> Int
//                   | Literal
//                   | ID
//                   | '(' expression ')'
//=================================================================================
static std::shared_ptr<ExprAST> parsePrimary(iterator_t& it, iterator_t end) {
  skipWS(it, end);
  if (it == end)
    throw ParsingError("Unexpected end of tokens in parsePrimary.");

  // Try integer
  if (auto p = std::get_if<tok::Int>(&*it)) {
    auto val = p->value;
    ++it;
    return std::make_shared<ExprAST>(val);
  }

  // Try string
  if (auto p = std::get_if<tok::Literal>(&*it)) {
    auto strval = p->str;
    ++it;
    return std::make_shared<ExprAST>(strval);
  }

  // Try identifier
  if (auto p = std::get_if<tok::ID>(&*it)) {
    auto idval = p->id;
    ++it;
    return std::make_shared<ExprAST>(IdExpr(idval));
  }

  // Try '(' expression ')'
  if (tryConsumeToken<tok::ParenthesisL>(it, end)) {
    auto exprNode = parseExpression(it, end);
    skipWS(it, end);
    if (!tryConsumeToken<tok::ParenthesisR>(it, end))
      throw ParsingError("Missing closing ')' in parenthesized expression.");

    return std::make_shared<ExprAST>(ParenExpr(exprNode));
  }

  // If none matched, it's an error.
  throw ParsingError("Unknown token type in parsePrimary.");
}

//=================================================================================
// Public parse function entry point.
//=================================================================================
std::shared_ptr<ExprAST> ParseExpression(std::span<Token> input) {
  auto it = input.begin();
  auto end = input.end();

  // parse the expression.
  auto result = parseExpression(it, end);

  // Skip any trailing whitespace.
  skipWS(it, end);

  if (it != end) {
    std::ptrdiff_t index = std::distance(input.begin(), it);
    std::ostringstream oss;
    oss << "Parsing did not consume all tokens. Leftover begins at index "
        << index << " with token "
        << std::visit(tok::DebugStringVisitor(), *it);
    throw ParsingError(oss.str());
  }

  return result;
}

}  // namespace m6
