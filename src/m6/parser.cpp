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
#include "m6/tokenizer.hpp"
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
static std::shared_ptr<AST> parseStmt(iterator_t& it, iterator_t end, bool);

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
  iterator_t lhs_begin_it = it;
  auto lhs = parseLogicalOr(it, end);
  iterator_t lhs_end_it = it;

  skipWS(it, end);
  Token* op_it = it;
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
                      SourceLocation(lhs_begin_it, lhs_end_it));
  }

  iterator_t rhs_begin_it = it;
  auto rhs = parseExpression(it, end);
  iterator_t rhs_end_it = it;
  if (assignmentOp == Op::Assign) {  // simple assignment
    return std::make_shared<AST>(AssignStmt(
        lhs, rhs, SourceLocation(lhs_begin_it, lhs_end_it),
        SourceLocation(op_it), SourceLocation(rhs_begin_it, rhs_end_it)));
  } else {  // compound assignment
    return std::make_shared<AST>(AugStmt(
        lhs, assignmentOp, rhs, SourceLocation(lhs_begin_it, lhs_end_it),
        SourceLocation(op_it, op_it + 1),
        SourceLocation(rhs_begin_it, rhs_end_it)));
  }
}

//=================================================================================
// parseLogicalOr() - left-associative '||'.
//     logical_or_expr -> logical_and_expr ( '||' logical_and_expr )*
//=================================================================================
static std::shared_ptr<ExprAST> parseLogicalOr(iterator_t& it, iterator_t end) {
  iterator_t lhs_begin_it = it;
  auto lhs = parseLogicalAnd(it, end);
  iterator_t lhs_end_it = it;

  while (true) {
    skipWS(it, end);
    iterator_t op_it = it;
    if (!tryConsume<tok::Operator>(it, end, Op::LogicalOr)) {
      break;
    }

    iterator_t rhs_begin_it = it;
    auto rhs = parseLogicalAnd(it, end);
    iterator_t rhs_end_it = it;

    // Note how we construct the BinaryExpr with SourceLocations:
    BinaryExpr expr(Op::LogicalOr, lhs, rhs, SourceLocation(op_it, op_it + 1),
                    SourceLocation(lhs_begin_it, lhs_end_it),
                    SourceLocation(rhs_begin_it, rhs_end_it));

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
  iterator_t lhs_begin_it = it;
  auto lhs = parseBitwiseOr(it, end);
  iterator_t lhs_end_it = it;

  while (true) {
    skipWS(it, end);
    iterator_t op_it = it;
    if (!tryConsume<tok::Operator>(it, end, Op::LogicalAnd)) {
      break;
    }

    iterator_t rhs_begin_it = it;
    auto rhs = parseBitwiseOr(it, end);
    iterator_t rhs_end_it = it;

    BinaryExpr expr(Op::LogicalAnd, lhs, rhs, SourceLocation(op_it, op_it + 1),
                    SourceLocation(lhs_begin_it, lhs_end_it),
                    SourceLocation(rhs_begin_it, rhs_end_it));

    lhs = std::make_shared<ExprAST>(std::move(expr));
  }
  return lhs;
}

//=================================================================================
// parseBitwiseOr() - left-associative '|'.
//     bitwise_or_expr -> bitwise_xor_expr ( '|' bitwise_xor_expr )*
//=================================================================================
static std::shared_ptr<ExprAST> parseBitwiseOr(iterator_t& it, iterator_t end) {
  iterator_t lhs_begin_it = it;
  auto lhs = parseBitwiseXor(it, end);
  iterator_t lhs_end_it = it;

  while (true) {
    skipWS(it, end);
    iterator_t op_it = it;
    if (!tryConsume<tok::Operator>(it, end, Op::BitOr)) {
      break;
    }

    iterator_t rhs_begin_it = it;
    auto rhs = parseBitwiseXor(it, end);
    iterator_t rhs_end_it = it;

    BinaryExpr expr(Op::BitOr, lhs, rhs, SourceLocation(op_it, op_it + 1),
                    SourceLocation(lhs_begin_it, lhs_end_it),
                    SourceLocation(rhs_begin_it, rhs_end_it));

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
  iterator_t lhs_begin_it = it;
  auto lhs = parseBitwiseAnd(it, end);
  iterator_t lhs_end_it = it;

  while (true) {
    skipWS(it, end);
    iterator_t op_it = it;
    if (!tryConsume<tok::Operator>(it, end, Op::BitXor)) {
      break;
    }

    iterator_t rhs_begin_it = it;
    auto rhs = parseBitwiseAnd(it, end);
    iterator_t rhs_end_it = it;

    BinaryExpr expr(Op::BitXor, lhs, rhs, SourceLocation(op_it, op_it + 1),
                    SourceLocation(lhs_begin_it, lhs_end_it),
                    SourceLocation(rhs_begin_it, rhs_end_it));

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
  iterator_t lhs_begin_it = it;
  auto lhs = parseEquality(it, end);
  iterator_t lhs_end_it = it;

  while (true) {
    skipWS(it, end);
    iterator_t op_it = it;
    if (!tryConsume<tok::Operator>(it, end, Op::BitAnd)) {
      break;
    }

    iterator_t rhs_begin_it = it;
    auto rhs = parseEquality(it, end);
    iterator_t rhs_end_it = it;

    BinaryExpr expr(Op::BitAnd, lhs, rhs, SourceLocation(op_it, op_it + 1),
                    SourceLocation(lhs_begin_it, lhs_end_it),
                    SourceLocation(rhs_begin_it, rhs_end_it));

    lhs = std::make_shared<ExprAST>(std::move(expr));
  }
  return lhs;
}

//=================================================================================
// parseEquality() - left-associative '==' and '!='.
//     equality_expr -> relational_expr ( ('=='|'!=') relational_expr )*
//=================================================================================
static std::shared_ptr<ExprAST> parseEquality(iterator_t& it, iterator_t end) {
  iterator_t lhs_begin_it = it;
  auto lhs = parseRelational(it, end);
  iterator_t lhs_end_it = it;

  while (true) {
    skipWS(it, end);
    iterator_t op_it = it;
    auto match = tryConsumeAny(it, end, {Op::Equal, Op::NotEqual});
    if (!match.has_value())
      break;

    auto op = match.value();

    iterator_t rhs_begin_it = it;
    auto rhs = parseRelational(it, end);
    iterator_t rhs_end_it = it;

    BinaryExpr expr(op, lhs, rhs, SourceLocation(op_it, op_it + 1),
                    SourceLocation(lhs_begin_it, lhs_end_it),
                    SourceLocation(rhs_begin_it, rhs_end_it));

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
  iterator_t lhs_begin_it = it;
  auto lhs = parseShift(it, end);
  iterator_t lhs_end_it = it;

  while (true) {
    skipWS(it, end);
    iterator_t op_it = it;
    auto match = tryConsumeAny(
        it, end, {Op::Less, Op::LessEqual, Op::Greater, Op::GreaterEqual});
    if (!match.has_value())
      break;

    auto op = match.value();

    iterator_t rhs_begin_it = it;
    auto rhs = parseShift(it, end);
    iterator_t rhs_end_it = it;

    BinaryExpr expr(op, lhs, rhs, SourceLocation(op_it, op_it + 1),
                    SourceLocation(lhs_begin_it, lhs_end_it),
                    SourceLocation(rhs_begin_it, rhs_end_it));

    lhs = std::make_shared<ExprAST>(std::move(expr));
  }
  return lhs;
}

//=================================================================================
// parseShift() - left-associative '<<', '>>', '>>>'.
//     shift_expr -> additive_expr ( ('<<'|'>>'|'>>>') additive_expr )*
//=================================================================================
static std::shared_ptr<ExprAST> parseShift(iterator_t& it, iterator_t end) {
  iterator_t lhs_begin_it = it;
  auto lhs = parseAdditive(it, end);
  iterator_t lhs_end_it = it;

  while (true) {
    skipWS(it, end);
    iterator_t op_it = it;
    auto match = tryConsumeAny(
        it, end, {Op::ShiftLeft, Op::ShiftRight, Op::ShiftUnsignedRight});
    if (!match.has_value())
      break;

    auto op = match.value();

    iterator_t rhs_begin_it = it;
    auto rhs = parseAdditive(it, end);
    iterator_t rhs_end_it = it;

    BinaryExpr expr(op, lhs, rhs, SourceLocation(op_it, op_it + 1),
                    SourceLocation(lhs_begin_it, lhs_end_it),
                    SourceLocation(rhs_begin_it, rhs_end_it));

    lhs = std::make_shared<ExprAST>(std::move(expr));
  }
  return lhs;
}

//=================================================================================
// parseAdditive() - left-associative '+' and '-'.
//     additive_expr -> multiplicative_expr ( ('+'|'-') multiplicative_expr )*
//=================================================================================
static std::shared_ptr<ExprAST> parseAdditive(iterator_t& it, iterator_t end) {
  iterator_t lhs_begin_it = it;
  auto lhs = parseMultiplicative(it, end);
  iterator_t lhs_end_it = it;

  while (true) {
    skipWS(it, end);
    iterator_t op_it = it;
    auto match = tryConsumeAny(it, end, {Op::Add, Op::Sub});
    if (!match.has_value())
      break;

    auto op = match.value();

    iterator_t rhs_begin_it = it;
    auto rhs = parseMultiplicative(it, end);
    iterator_t rhs_end_it = it;

    BinaryExpr expr(op, lhs, rhs, SourceLocation(op_it, op_it + 1),
                    SourceLocation(lhs_begin_it, lhs_end_it),
                    SourceLocation(rhs_begin_it, rhs_end_it));

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
  iterator_t lhs_begin_it = it;
  auto lhs = parseUnary(it, end);
  iterator_t lhs_end_it = it;

  while (true) {
    skipWS(it, end);
    iterator_t op_it = it;
    auto match = tryConsumeAny(it, end, {Op::Mul, Op::Div, Op::Mod});
    if (!match.has_value())
      break;

    auto op = match.value();

    iterator_t rhs_begin_it = it;
    auto rhs = parseUnary(it, end);
    iterator_t rhs_end_it = it;

    BinaryExpr expr(op, lhs, rhs, SourceLocation(op_it, op_it + 1),
                    SourceLocation(lhs_begin_it, lhs_end_it),
                    SourceLocation(rhs_begin_it, rhs_end_it));

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
  std::vector<iterator_t> op_locs;
  while (true) {
    skipWS(it, end);
    iterator_t op_loc = it;
    auto match = tryConsumeAny(it, end, {Op::Add, Op::Sub, Op::Tilde});
    if (!match.has_value())
      break;

    ops.push_back(match.value());
    op_locs.push_back(op_loc);
  }

  // Parse the postfix expression that these unary ops apply to.
  iterator_t sub_begin_it = it;
  auto node = parsePostfix(it, end);
  iterator_t sub_end_it = it;

  // Apply unary ops in reverse order (right-to-left).
  for (size_t i = ops.size(); i-- > 0;) {
    UnaryExpr expr(ops[i], node, SourceLocation(op_locs[i]),
                   SourceLocation(sub_begin_it, sub_end_it));
    node = std::make_shared<ExprAST>(std::move(expr));
    sub_begin_it = op_locs[i];
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
  iterator_t primary_begin_it = it;
  auto lhs = parsePrimary(it, end);
  iterator_t primary_end_it = it;

  // Repeatedly parse postfix elements.
  while (true) {
    skipWS(it, end);

    // 1) function call
    if (tryConsume<tok::ParenthesisL>(it, end)) {
      // parse optional expression
      skipWS(it, end);

      std::vector<std::shared_ptr<ExprAST>> arg_list;
      std::vector<SourceLocation> arg_locs;
      if (it != end && !it->HoldsAlternative<tok::ParenthesisR>()) {
        iterator_t begin_it = it;
        arg_list.emplace_back(parseExpression(it, end));
        arg_locs.emplace_back(begin_it, it);

        while (tryConsume<tok::Operator>(it, end, Op::Comma)) {
          begin_it = it;
          arg_list.emplace_back(parseExpression(it, end));
          arg_locs.emplace_back(begin_it, it);
        }
      }

      if (!tryConsume<tok::ParenthesisR>(it, end)) {
        throw SyntaxError("Expected ')' after function call.", it);
      }
      lhs = std::make_shared<ExprAST>(
          InvokeExpr(lhs, std::move(arg_list),
                     SourceLocation(primary_begin_it, primary_end_it),
                     std::move(arg_locs)));
      continue;
    }

    // 2) member access: '.' <identifier>
    if (tryConsume<tok::Operator>(it, end, Op::Dot)) {
      skipWS(it, end);
      if (it == end || !it->HoldsAlternative<tok::ID>()) {
        throw SyntaxError("Expected identifier after '.'", it);
      }
      auto memberLoc = SourceLocation(it);
      Identifier memberName(it->GetIf<tok::ID>()->id, memberLoc);
      ++it;  // consume the ID token
      auto memberNode = std::make_shared<ExprAST>(std::move(memberName));
      lhs = std::make_shared<ExprAST>(MemberExpr(
          lhs, memberNode, SourceLocation(primary_begin_it, primary_end_it),
          memberLoc));
      continue;
    }

    // 3) subscript: '[' expression ']'
    if (tryConsume<tok::SquareL>(it, end)) {
      iterator_t idx_begin_it = it;
      auto indexExpr = parseExpression(it, end);
      iterator_t idx_end_it = it;

      if (!tryConsume<tok::SquareR>(it, end)) {
        throw SyntaxError("Expected ']' after subscript expression.", it);
      }
      lhs = std::make_shared<ExprAST>(SubscriptExpr(
          lhs, indexExpr, SourceLocation(primary_begin_it, primary_end_it),
          SourceLocation(idx_begin_it, idx_end_it)));
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
    throw SyntaxError("Expected primary expression.", it);

  // Try integer
  if (auto s = it->GetIf<tok::Int>()) {
    auto node = IntLiteral(s->value, SourceLocation(it, it + 1));
    ++it;
    return std::make_shared<ExprAST>(std::move(node));
  }

  // Try string
  if (auto s = it->GetIf<tok::Literal>()) {
    auto node = StrLiteral(s->str, SourceLocation(it, it + 1));
    ++it;
    return std::make_shared<ExprAST>(std::move(node));
  }

  // Try identifier
  if (auto s = it->GetIf<tok::ID>()) {
    auto node = Identifier(s->id, SourceLocation(it, it + 1));
    ++it;
    return std::make_shared<ExprAST>(std::move(node));
  }

  // Try '(' expr ')'
  if (tryConsume<tok::ParenthesisL>(it, end)) {
    iterator_t sub_begin_it = it;
    auto exprNode = parseExpression(it, end);
    iterator_t sub_end_it = it;

    skipWS(it, end);
    if (!tryConsume<tok::ParenthesisR>(it, end))
      throw SyntaxError("Missing closing ')' in parenthesized expression.", it);

    return std::make_shared<ExprAST>(
        ParenExpr(exprNode, SourceLocation(sub_begin_it, sub_end_it)));
  }

  // If none matched, it's an error.
  throw SyntaxError("Expected primary expression.", it);
}

//=================================================================================
// parseStmt() - statement
//     stmt -> assignment_expr
//           | "if" "(" expr ")" stmt ("else" stmt)?
//           | "while" "(" expr ")" stmt
//           | "for" "(" stmt ";" expr ";" stmt ")" stmt
//           | "{" stmt* "}"
//=================================================================================
template <typename T>
inline static void Require(iterator_t& it, iterator_t end, char const* msg) {
  if (!tryConsume<T>(it, end))
    throw SyntaxError(msg, it);
}
static std::shared_ptr<AST> parseStmt(iterator_t& it,
                                      iterator_t end,
                                      bool require_semicol = true) {
  // if statement
  if (tryConsume<tok::Reserved>(it, end, "if")) {
    Require<tok::ParenthesisL>(it, end, "Expected '('.");
    auto cond = parseExpression(it, end);
    Require<tok::ParenthesisR>(it, end, "Expected ')'.");

    std::shared_ptr<AST> then = parseStmt(it, end), els = nullptr;
    if (tryConsume<tok::Reserved>(it, end, "else"))
      els = parseStmt(it, end);

    return std::make_shared<AST>(IfStmt(cond, then, els));
  }

  // while statement
  if (tryConsume<tok::Reserved>(it, end, "while")) {
    Require<tok::ParenthesisL>(it, end, "Expected '('.");
    auto cond = parseExpression(it, end);
    Require<tok::ParenthesisR>(it, end, "Expected ')'.");

    auto body = parseStmt(it, end);
    return std::make_shared<AST>(WhileStmt(cond, body));
  }

  // for statement
  if (tryConsume<tok::Reserved>(it, end, "for")) {
    std::shared_ptr<AST> init, inc;
    std::shared_ptr<ExprAST> cond;

    Require<tok::ParenthesisL>(it, end, "Expected '('.");
    if (tryConsume<tok::Semicol>(it, end))
      init = nullptr;
    else {
      init = parseStmt(it, end, false);
      Require<tok::Semicol>(it, end, "Expected ';'.");
    }
    if (tryConsume<tok::Semicol>(it, end))
      cond = nullptr;
    else {
      cond = parseExpression(it, end);
      Require<tok::Semicol>(it, end, "Expected ';'.");
    }
    if (tryConsume<tok::ParenthesisR>(it, end))
      inc = nullptr;
    else {
      inc = parseStmt(it, end, false);
      Require<tok::ParenthesisR>(it, end, "Expected ')'.");
    }

    auto body = parseStmt(it, end);
    return std::make_shared<AST>(ForStmt(init, cond, inc, body));
  }

  // block
  if (tryConsume<tok::CurlyL>(it, end)) {
    std::vector<std::shared_ptr<AST>> body;
    while (!tryConsume<tok::CurlyR>(it, end))
      body.emplace_back(parseStmt(it, end));
    return std::make_shared<AST>(BlockStmt(std::move(body)));
  }

  // assignment or expression statement
  auto stmt = parseAssignment(it, end);
  if (require_semicol)
    Require<tok::Semicol>(it, end, "Expected ';'.");
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

//=================================================================================
// class Parser
//=================================================================================
Parser::Parser() = default;

std::span<std::shared_ptr<AST>> Parser::Parse(std::string input) {
  error_.clear();
  const size_t src_n = src_.length(), token_n = tokens_.size(),
               program_n = program_.size();

  src_ += std::move(input);
  std::string_view src = src_;
  src = src.substr(src_n);

  Tokenizer tokenizer(tokens_);
  tokenizer.Parse(src);

  for (auto begin = tokens_.data(), end = tokens_.data() + tokens_.size();
       begin < end;) {
    try {
      auto stmt = parseStmt(begin, end);
      program_.emplace_back(std::move(stmt));
    } catch (const SyntaxError& err) {
      error_.push_back(err);

      while (begin < end) {
        if (begin->HoldsAlternative<tok::Semicol>() ||
            begin->HoldsAlternative<tok::CurlyR>()) {
          ++begin;
          break;
        }
        ++begin;
      }
    }
  }

  if (error_.empty()) {
    return std::span(program_.begin() + program_n, program_.end());
  } else {
    src_.resize(src_n);
    tokens_.resize(token_n);
    program_.resize(program_n);
    return {};
  }
}

}  // namespace m6
