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
#include "m6/ast.hpp"
#include "m6/exception.hpp"
#include "machine/op.hpp"

#include <iterator>
#include <optional>
#include <sstream>
#include <unordered_set>

namespace m6 {

// using iterator_t = std::span<Token>::iterator;
using iterator_t = Token*;

// Helper to skip whitespace tokens in-place.
static void skipWS(iterator_t& it, iterator_t end) {
  while (it != end && it->HoldsAlternative<tok::WS>()) {
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

  if (auto p = it->GetIf<tok::Operator>()) {
    for (auto candidate : ops)
      if (candidate == p->op) {
        ++it;
        return p->op;
      }
  }
  return std::nullopt;
}

// Handy checker function for matching a token type T.
template <typename T>
static bool tryConsume(iterator_t& it, iterator_t end) {
  skipWS(it, end);
  if (it == end) {
    return false;
  }
  if (it->HoldsAlternative<T>()) {
    ++it;
    return true;
  }
  return false;
}

// Handy checker function for maching a token with specified type and field
template <typename T, typename... Ts>
static bool tryConsume(iterator_t& it, iterator_t end, Ts&&... params) {
  skipWS(it, end);
  if (it == end)
    return false;

  if (auto t = it->GetIf<T>()) {
    if (*t == T(std::forward<Ts>(params)...)) {
      ++it;
      return true;
    }
  }
  return false;
}

// Forward declarations of recursive parsing functions.
static std::shared_ptr<AST> parseAssignment(iterator_t& it, iterator_t end);
static std::shared_ptr<ExprAST> parseExpression(iterator_t& it, iterator_t end);
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
static std::shared_ptr<AST> parseStmt(iterator_t& it, iterator_t end);

//=================================================================================
// parseExpression() - top-level parse for expressions.
//     expr -> logical_or_expr
//=================================================================================
static std::shared_ptr<ExprAST> parseExpression(iterator_t& it,
                                                iterator_t end) {
  return parseLogicalOr(it, end);
}

//=================================================================================
// parseAssignment() - right-associative assignment ops.
//     assignment_expr -> logical_or_expr ( ASSIGN_OP assignment_expr )?
//     where ASSIGN_OP is one of =, +=, -=, etc.
//=================================================================================
static std::shared_ptr<AST> parseAssignment(iterator_t& it, iterator_t end) {
  skipWS(it, end);
  iterator_t lhs_begin = it;
  auto lhs = parseLogicalOr(it, end);
  iterator_t lhs_end = it;

  skipWS(it, end);
  Token* op_tok = it;
  auto matched =
      tryConsumeAny(it, end,
                    {Op::Assign, Op::AddAssign, Op::SubAssign, Op::MulAssign,
                     Op::DivAssign, Op::ModAssign, Op::BitAndAssign,
                     Op::BitOrAssign, Op::BitXorAssign, Op::ShiftLeftAssign,
                     Op::ShiftRightAssign, Op::ShiftUnsignedRightAssign});
  if (!matched.has_value())
    return std::make_shared<AST>(lhs);

  auto assignmentOp = matched.value();
  auto* id_node = lhs->Apply([](auto& x) -> Identifier* {
    if constexpr (std::same_as<std::decay_t<decltype(x)>, Identifier>)
      return &x;  // for now, only support assign to identifier
    return nullptr;
  });
  if (id_node == nullptr) {
    throw SyntaxError("Expected identifier.",
                      std::make_optional<SourceLocation>(*lhs_begin, *lhs_end));
  }

  auto rhs = parseExpression(it, end);
  if (assignmentOp == Op::Assign) {  // simple assignment
    return std::make_shared<AST>(AssignStmt(lhs, rhs));
  } else {  // compound assignment
    return std::make_shared<AST>(AugStmt(lhs, op_tok, rhs));
  }
}

//=================================================================================
// parseLogicalOr() - left-associative '||'.
//     logical_or_expr -> logical_and_expr ( '||' logical_and_expr )*
//=================================================================================
static std::shared_ptr<ExprAST> parseLogicalOr(iterator_t& it, iterator_t end) {
  auto lhs = parseLogicalAnd(it, end);
  while (true) {
    skipWS(it, end);
    if (!tryConsume<tok::Operator>(it, end, Op::LogicalOr)) {
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
    if (!tryConsume<tok::Operator>(it, end, Op::LogicalAnd)) {
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
    if (!tryConsume<tok::Operator>(it, end, Op::BitOr)) {
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
    if (!tryConsume<tok::Operator>(it, end, Op::BitXor))
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
    if (!tryConsume<tok::Operator>(it, end, Op::BitAnd))
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
//         '(' [expression (,expression)*] ')'        (function invocation)
//       | '.' id_token                               (member access)
//       | '[' expr ']'                               (subscript)
//=================================================================================
static std::shared_ptr<ExprAST> parsePostfix(iterator_t& it, iterator_t end) {
  auto lhs = parsePrimary(it, end);

  // Repeatedly parse postfix elements.
  while (true) {
    skipWS(it, end);

    // 1) function call
    if (tryConsume<tok::ParenthesisL>(it, end)) {
      // parse optional expression
      skipWS(it, end);

      std::vector<std::shared_ptr<ExprAST>> arglist;
      if (it != end && !it->HoldsAlternative<tok::ParenthesisR>()) {
        arglist.emplace_back(parseExpression(it, end));

        while (tryConsume<tok::Operator>(it, end, Op::Comma))
          arglist.emplace_back(parseExpression(it, end));
      }

      if (!tryConsume<tok::ParenthesisR>(it, end)) {
        throw SyntaxError("Expected ')' after function call.", *it);
      }
      lhs = std::make_shared<ExprAST>(InvokeExpr(lhs, std::move(arglist)));
      continue;
    }

    // 2) member access: '.' <identifier>
    if (tryConsume<tok::Operator>(it, end, Op::Dot)) {
      skipWS(it, end);
      if (it == end || !it->HoldsAlternative<tok::ID>()) {
        throw SyntaxError("Expected identifier after '.'", *it);
      }
      Identifier memberName(it);
      ++it;  // consume the ID token
      auto memberNode = std::make_shared<ExprAST>(std::move(memberName));
      lhs = std::make_shared<ExprAST>(MemberExpr(lhs, memberNode));
      continue;
    }

    // 3) subscript: '[' expression ']'
    if (tryConsume<tok::SquareL>(it, end)) {
      auto indexExpr = parseExpression(it, end);
      if (!tryConsume<tok::SquareR>(it, end)) {
        throw SyntaxError("Expected ']' after subscript expression.", *it);
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
//                   | '(' expr ')'
//=================================================================================
static std::shared_ptr<ExprAST> parsePrimary(iterator_t& it, iterator_t end) {
  skipWS(it, end);
  if (it == end)
    throw SyntaxError("Expected primary expression.", *it);

  // Try integer
  if (it->HoldsAlternative<tok::Int>())
    return std::make_shared<ExprAST>(IntLiteral(it++));

  // Try string
  if (it->HoldsAlternative<tok::Literal>())
    return std::make_shared<ExprAST>(StrLiteral(it++));

  // Try identifier
  if (it->HoldsAlternative<tok::ID>())
    return std::make_shared<ExprAST>(Identifier(it++));

  // Try '(' expr ')'
  if (tryConsume<tok::ParenthesisL>(it, end)) {
    auto exprNode = parseExpression(it, end);
    skipWS(it, end);
    if (!tryConsume<tok::ParenthesisR>(it, end))
      throw SyntaxError("Missing closing ')' in parenthesized expression.",
                        *it);

    return std::make_shared<ExprAST>(ParenExpr(exprNode));
  }

  // If none matched, it's an error.
  throw SyntaxError("Expected primary expression.", *it);
}

//=================================================================================
// parseStmt() - statement
//     stmt -> assignment_expr
//           | "if" "(" expr; ")" stmt ("else" stmt)?
//=================================================================================
static std::shared_ptr<AST> parseStmt(iterator_t& it, iterator_t end) {
  // if statement
  if (tryConsume<tok::Reserved>(it, end, "if")) {
    if (!tryConsume<tok::ParenthesisL>(it, end))
      throw SyntaxError("Expected '('.",
                        std::make_optional<SourceLocation>(*it));

    auto cond = parseExpression(it, end);

    if (!tryConsume<tok::ParenthesisR>(it, end))
      throw SyntaxError("Expected ')'.",
                        std::make_optional<SourceLocation>(*it));

    std::shared_ptr<AST> then = parseStmt(it, end), els = nullptr;
    if (tryConsume<tok::Reserved>(it, end, "else"))
      els = parseStmt(it, end);

    return std::make_shared<AST>(IfStmt(cond, then, els));
  }

  // assignment or expression statement
  auto stmt = parseAssignment(it, end);
  if (!tryConsume<tok::Semicol>(it, end))
    throw SyntaxError("Expected ';'.", std::make_optional<SourceLocation>(*it));
  return stmt;
}

//=================================================================================
// Public parse function entry point.
//=================================================================================
std::shared_ptr<ExprAST> ParseExpression(Token*& it, Token* end) {
  return parseLogicalOr(it, end);
}
std::shared_ptr<ExprAST> ParseExpression(std::span<Token> input) {
  auto begin = input.data(), end = input.data() + input.size();
  return ParseExpression(begin, end);
}

std::shared_ptr<AST> ParseStmt(Token*& begin, Token* end) {
  return parseStmt(begin, end);
}
std::shared_ptr<AST> ParseStmt(std::span<Token> input) {
  auto begin = input.data(), end = input.data() + input.size();
  return ParseStmt(begin, end);
}

}  // namespace m6
