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
#include "m6/error.hpp"
#include "m6/tokenizer.hpp"
#include "machine/op.hpp"

#include <iterator>
#include <optional>
#include <sstream>
#include <unordered_set>

namespace m6 {

namespace {
// helper
inline static SourceLocation LocRange(Token* begin, Token* end) {
  return begin->loc_.Combine(end->loc_);
}
}  // namespace

//--------------------------------------------------------------------
//  Parser class
//--------------------------------------------------------------------

// ── PUBLIC ENTRY POINTS ─────────────────────────────────────────────────────
std::shared_ptr<ExprAST> Parser::ParseExpression() { return parseLogicalOr(); }

std::shared_ptr<AST> Parser::ParseStatement(bool requireSemi) {
  if (auto reserved = it_->GetIf<tok::Reserved>()) {
    ++it_;

    switch (reserved->type) {
      case tok::Reserved::_if: {
        require<tok::ParenthesisL>("expected '(' after if");
        auto cond = ParseExpression();
        require<tok::ParenthesisR>("expected ')'");
        auto thenStmt = ParseStatement();
        std::shared_ptr<AST> elseStmt = nullptr;
        if (tryConsume<tok::Reserved>(tok::Reserved::_else))
          elseStmt = ParseStatement();
        return std::make_shared<AST>(IfStmt(cond, thenStmt, elseStmt));
      }

      case tok::Reserved::_while: {
        require<tok::ParenthesisL>("expected '(' after while");
        auto cond = ParseExpression();
        require<tok::ParenthesisR>("expected ')'");
        auto body = ParseStatement();
        return std::make_shared<AST>(WhileStmt(cond, body));
      }

      case tok::Reserved::_for: {
        require<tok::ParenthesisL>("expected '(' after for");

        std::shared_ptr<AST> init = nullptr, inc = nullptr;
        std::shared_ptr<ExprAST> cond = nullptr;

        if (!tryConsume<tok::Semicol>()) {
          init = ParseStatement(false);
          require<tok::Semicol>("expected ';' after for‑init");
        }
        if (!tryConsume<tok::Semicol>()) {
          cond = ParseExpression();
          require<tok::Semicol>("Expected ';' after for‑cond.");
        }
        if (!tryConsume<tok::ParenthesisR>()) {
          inc = ParseStatement(false);
          require<tok::ParenthesisR>("Expected ')' after for‑inc.");
        }
        auto body = ParseStatement();
        return std::make_shared<AST>(ForStmt(init, cond, inc, body));
      }

      case tok::Reserved::_class: {
        auto clsNameTok = *it_;
        auto clsNameLoc = it_->loc_;
        if (!require<tok::ID>("expected identifier")) {
          Synchronize();
          return nullptr;
        }

        std::string id = clsNameTok.GetIf<tok::ID>()->id;
        std::vector<FuncDecl> members;

        require<tok::CurlyL>("expected '{' after class name");
        while (it_ != end_ && !tryConsume<tok::CurlyR>()) {
          // fn
          if (tryConsume<tok::Reserved>(tok::Reserved::_fn)) {
            auto fn = parseFuncDecl(/*alreadyConsumedFn=*/true);
            if (fn && fn->HoldsAlternative<FuncDecl>()) {
              members.emplace_back(std::move(*fn->Get_if<FuncDecl>()));
              continue;
            } else {
              Synchronize();
              return nullptr;
            }
          }

          AddError("only function declarations allowed in class body", it_);
          Synchronize();
          return nullptr;
        }

        ClassDecl cd{std::move(id), std::move(members), clsNameLoc};
        return std::make_shared<AST>(std::move(cd));
      }

      case tok::Reserved::_fn:
        return parseFuncDecl(true);

      case tok::Reserved::_return: {
        auto kwLoc = (it_ - 1)->loc_;
        std::shared_ptr<ExprAST> val = nullptr;
        if (!tryConsume<tok::Semicol>()) {
          val = ParseExpression();
          require<tok::Semicol>("expected ';' after return");
        }
        return std::make_shared<AST>(ReturnStmt(val, kwLoc));
      }

      case tok::Reserved::_yield: {
        auto kwLoc = (it_ - 1)->loc_;
        std::shared_ptr<ExprAST> expr = nullptr;
        if (!tryConsume<tok::Semicol>()) {
          expr = ParseExpression();
          require<tok::Semicol>("expected ';' after yield");
        }

        return std::make_shared<AST>(YieldStmt(expr, kwLoc));
      }

      case tok::Reserved::_spawn: {
        auto kwLoc = (it_ - 1)->loc_;
        require<tok::ParenthesisL>("expected '(' after spawn");

        auto fnNameTok = *it_;
        auto fnNameLoc = it_->loc_;
        if (!require<tok::ID>("expected identifier")) {
          Synchronize();
          return nullptr;
        }
        std::string fn = fnNameTok.GetIf<tok::ID>()->id;

        std::vector<std::shared_ptr<ExprAST>> args;
        while (it_ != end_ && tryConsume<tok::Operator>(Op::Comma))
          args.emplace_back(ParseExpression());

        require<tok::ParenthesisR>("expected ')'");
        require<tok::Semicol>("expected ';'");

        return std::make_shared<AST>(
            SpawnStmt(std::move(fn), std::move(args), kwLoc));
      }

      default: {
        --it_;
        AddError("unexpected reserved keyword", it_);
        Synchronize();
        return nullptr;
      }
    }
  }

  // block
  if (tryConsume<tok::CurlyL>()) {
    std::vector<std::shared_ptr<AST>> body;
    while (!tryConsume<tok::CurlyR>() && it_ != end_) {
      body.emplace_back(ParseStatement());
    }
    return std::make_shared<AST>(BlockStmt(std::move(body)));
  }

  {
    // expression / assignment statement
    auto stmt = parseAssignment();
    if (requireSemi)
      require<tok::Semicol>("Expected ';'.");
    return stmt;
  }
}

std::vector<std::shared_ptr<AST>> Parser::ParseAll() {
  std::vector<std::shared_ptr<AST>> out;
  while (it_ != end_ && errors_.empty()) {
    if (it_->HoldsAlternative<tok::Eof>())
      break;

    auto stmt = ParseStatement();
    if (stmt)
      out.emplace_back(stmt);
    else if (it_ == end_)
      break;  // fatal: could not advance
  }
  return out;
}

bool Parser::Ok() const { return errors_.empty(); }

std::span<const Error> Parser::GetErrors() const { return errors_; }

void Parser::ClearErrors() { errors_.clear(); }

void Parser::AddError(std::string m, iterator_t loc) {
  errors_.emplace_back(std::move(m), loc->loc_);
}
void Parser::AddError(std::string m, SourceLocation loc) {
  errors_.emplace_back(std::move(m), std::move(loc));
}

// Panic‑mode: skip to next ; or } or EOF
// -------------------------------------
void Parser::Synchronize() {
  while (it_ != end_) {
    if (it_->template HoldsAlternative<tok::Semicol>() ||
        it_->template HoldsAlternative<tok::CurlyR>()) {
      return;
    }
    ++it_;
  }
}

// --- RECURSIVE‑DESCENT PRODUCTIONS -----------------------------------------
std::shared_ptr<AST> Parser::parseAssignment() {
  auto lhs_begin = it_;
  auto lhs = parseLogicalOr();
  if (!lhs)
    return nullptr;
  auto lhs_end = it_;

  auto op_it = it_;
  auto match =
      tryConsumeAny({Op::Assign, Op::AddAssign, Op::SubAssign, Op::MulAssign,
                     Op::DivAssign, Op::ModAssign, Op::BitAndAssign,
                     Op::BitOrAssign, Op::BitXorAssign, Op::ShiftLeftAssign,
                     Op::ShiftRightAssign, Op::ShiftUnsignedRightAssign});
  if (!match.has_value())  // expression statement
    return std::make_shared<AST>(lhs);
  Op assignmentOp = match.value();

  auto rhs_begin = it_;
  auto rhs = ParseExpression();
  auto rhs_end = it_;
  if (rhs == nullptr)
    return nullptr;

  if (assignmentOp == Op::Assign) {
    return std::make_shared<AST>(
        AssignStmt(lhs, rhs, LocRange(lhs_begin, lhs_end), op_it->loc_,
                   LocRange(rhs_begin, rhs_end)));
  } else {
    return std::make_shared<AST>(
        AugStmt(lhs, assignmentOp, rhs, LocRange(lhs_begin, lhs_end),
                op_it->loc_, LocRange(rhs_begin, rhs_end)));
  }
}

// -----------------------------------------------------------------------------
//  Helper:  scanParameterList()
//  Consumes a Python-like parameter list:
//        ( a, b, c=42, *args, d, e=0, **kw )
//
//  and fills the five output collections that live inside FuncDecl.
//  On success the lexer cursor is positioned **after** the closing ')'.
//
bool Parser::ScanParameterList(
    std::vector<std::string>& required,
    std::vector<SourceLocation>& requiredLocs,
    std::vector<std::pair<std::string, std::shared_ptr<ExprAST>>>& defaulted,
    std::vector<SourceLocation>& defaultedLocs,
    std::string& varArg,
    SourceLocation& varArgLoc,
    std::string& kwArg,
    SourceLocation& kwArgLoc) {
  // We enter with '(' already consumed.
  bool passedStar = false;          // “*” sentinel or *args seen?
  bool seenDefaultEarlier = false;  // once we saw "x=..." every further
                                    // *positional* must also have default.

  if (it_ != end_ && !it_->HoldsAlternative<tok::ParenthesisR>()) {
    do {
      // ------------------------------------------------------------
      //  1.  **kw-only
      // ------------------------------------------------------------
      if (tryConsume<tok::Operator>(Op::Pow)) {
        if (!kwArg.empty()) {
          AddError("duplicate **kwargs parameter", it_);
          return false;
        }

        auto* idTok = it_->GetIf<tok::ID>();
        if (!idTok)
          AddError("identifier required after '**'", it_);

        kwArg = std::string(idTok->id);
        kwArgLoc = it_->loc_;
        ++it_;
        break;
      }

      // ------------------------------------------------------------
      //  2.  *args   OR   bare '*' (keyword-only sentinel)
      // ------------------------------------------------------------
      if (tryConsume<tok::Operator>(Op::Mul)) {  // '*'
        if (it_ != end_ && it_->HoldsAlternative<tok::ID>()) {
          // real “*args”
          if (!varArg.empty()) {
            AddError("duplicate *args parameter", it_);
            return false;
          }

          auto* idTok = it_->GetIf<tok::ID>();
          varArg = std::string(idTok->id);
          varArgLoc = it_->loc_;
          ++it_;
        }
        passedStar = true;
        // If it was the bare '*', we stay on the same token and continue.
        continue;  // ',' handled at the bottom
      }

      // ------------------------------------------------------------
      //  3.  normal parameter  (might have default)
      // ------------------------------------------------------------
      auto* idTok = it_->GetIf<tok::ID>();
      if (!idTok) {
        AddError("expected parameter name", it_);
        return false;
      }
      std::string paramName = std::string(idTok->id);
      SourceLocation paramLoc = it_->loc_;
      ++it_;

      if (tryConsume<tok::Operator>(Op::Assign)) {  // '='
        auto defExpr = ParseExpression();
        defaulted.emplace_back(paramName, defExpr);
        defaultedLocs.emplace_back(paramLoc);
        seenDefaultEarlier = true;
      } else {
        if (passedStar) {
          AddError("keyword argument after var_args must have default",
                   paramLoc);
          return false;
        }
        if (seenDefaultEarlier) {
          AddError("non-default positional argument follows default argument",
                   paramLoc);
          return false;
        }

        required.emplace_back(paramName);
        requiredLocs.emplace_back(paramLoc);
      }
    } while (tryConsume<tok::Operator>(Op::Comma));
  }

  require<tok::ParenthesisR>("expected ')' after parameter list");
  return true;
}

std::shared_ptr<AST> Parser::parseFuncDecl(bool consumedfn) {
  if (!consumedfn)
    require<tok::Reserved>("expected fn", tok::Reserved::_fn);
  auto nameTok = it_;
  require<tok::ID>("expected identifier");

  FuncDecl fn;
  fn.name = nameTok->GetIf<tok::ID>()->id;
  fn.name_loc = nameTok->loc_;

  require<tok::ParenthesisL>("expected '(' after function name");

  if (!ScanParameterList(fn.params, fn.param_locs, fn.default_params,
                         fn.def_params_loc, fn.var_arg, fn.var_arg_loc,
                         fn.kw_arg, fn.kw_arg_loc))
    return nullptr;

  if (!tryConsume<tok::CurlyL>()) {
    AddError("function body must be a block", it_);
    Synchronize();
    return nullptr;
  }

  --it_;  // hand the '{' back to statement parser
  fn.body = ParseStatement(false);

  return std::make_shared<AST>(std::move(fn));
}

std::shared_ptr<ExprAST> Parser::parseLogicalOr() {
  auto lhs_begin = it_;
  auto lhs = parseLogicalAnd();
  if (!lhs)
    return nullptr;
  auto lhs_end = it_;

  while (true) {
    auto op_it = it_;
    if (!tryConsume<tok::Operator>(Op::LogicalOr))
      break;
    auto rhs_begin = it_;
    auto rhs = parseLogicalAnd();
    if (!rhs)
      return lhs;  // give up chaining on error
    auto rhs_end = it_;

    BinaryExpr be(Op::LogicalOr, lhs, rhs, op_it->loc_,
                  LocRange(lhs_begin, lhs_end), LocRange(rhs_begin, rhs_end));
    lhs = std::make_shared<ExprAST>(std::move(be));
  }
  return lhs;
}

std::shared_ptr<ExprAST> Parser::parseLogicalAnd() {
  auto lhs_begin = it_;
  auto lhs = parseBitwiseOr();
  if (!lhs)
    return nullptr;
  auto lhs_end = it_;

  while (true) {
    auto op_it = it_;
    if (!tryConsume<tok::Operator>(Op::LogicalAnd))
      break;
    auto rhs_begin = it_;
    auto rhs = parseBitwiseOr();
    if (!rhs)
      return lhs;
    auto rhs_end = it_;

    BinaryExpr be(Op::LogicalAnd, lhs, rhs, op_it->loc_,
                  LocRange(lhs_begin, lhs_end), LocRange(rhs_begin, rhs_end));
    lhs = std::make_shared<ExprAST>(std::move(be));
  }
  return lhs;
}

std::shared_ptr<ExprAST> Parser::parseBitwiseOr() {
  auto lhs_begin = it_;
  auto lhs = parseBitwiseXor();
  if (!lhs)
    return nullptr;
  auto lhs_end = it_;

  while (true) {
    auto op_it = it_;
    if (!tryConsume<tok::Operator>(Op::BitOr))
      break;
    auto rhs_begin = it_;
    auto rhs = parseBitwiseXor();
    if (!rhs)
      return lhs;
    auto rhs_end = it_;

    BinaryExpr be(Op::BitOr, lhs, rhs, op_it->loc_,
                  LocRange(lhs_begin, lhs_end), LocRange(rhs_begin, rhs_end));
    lhs = std::make_shared<ExprAST>(std::move(be));
  }
  return lhs;
}

std::shared_ptr<ExprAST> Parser::parseBitwiseXor() {
  auto lhs_begin = it_;
  auto lhs = parseBitwiseAnd();
  if (!lhs)
    return nullptr;
  auto lhs_end = it_;

  while (true) {
    auto op_it = it_;
    if (!tryConsume<tok::Operator>(Op::BitXor))
      break;
    auto rhs_begin = it_;
    auto rhs = parseBitwiseAnd();
    if (!rhs)
      return lhs;
    auto rhs_end = it_;

    BinaryExpr be(Op::BitXor, lhs, rhs, op_it->loc_,
                  LocRange(lhs_begin, lhs_end), LocRange(rhs_begin, rhs_end));
    lhs = std::make_shared<ExprAST>(std::move(be));
  }
  return lhs;
}

std::shared_ptr<ExprAST> Parser::parseBitwiseAnd() {
  auto lhs_begin = it_;
  auto lhs = parseEquality();
  if (!lhs)
    return nullptr;
  auto lhs_end = it_;

  while (true) {
    auto op_it = it_;
    if (!tryConsume<tok::Operator>(Op::BitAnd))
      break;
    auto rhs_begin = it_;
    auto rhs = parseEquality();
    if (!rhs)
      return lhs;
    auto rhs_end = it_;

    BinaryExpr be(Op::BitAnd, lhs, rhs, op_it->loc_,
                  LocRange(lhs_begin, lhs_end), LocRange(rhs_begin, rhs_end));
    lhs = std::make_shared<ExprAST>(std::move(be));
  }
  return lhs;
}

std::shared_ptr<ExprAST> Parser::parseEquality() {
  auto lhs_begin = it_;
  auto lhs = parseRelational();
  if (!lhs)
    return nullptr;
  auto lhs_end = it_;

  while (true) {
    auto op_it = it_;
    auto op = tryConsumeAny({Op::Equal, Op::NotEqual});
    if (!op.has_value())
      break;
    auto rhs_begin = it_;
    auto rhs = parseRelational();
    if (!rhs)
      return lhs;
    auto rhs_end = it_;

    BinaryExpr be(op.value(), lhs, rhs, op_it->loc_,
                  LocRange(lhs_begin, lhs_end), LocRange(rhs_begin, rhs_end));
    lhs = std::make_shared<ExprAST>(std::move(be));
  }
  return lhs;
}

std::shared_ptr<ExprAST> Parser::parseRelational() {
  auto lhs_begin = it_;
  auto lhs = parseShift();
  if (!lhs)
    return nullptr;
  auto lhs_end = it_;

  while (true) {
    auto op_it = it_;
    auto op =
        tryConsumeAny({Op::Less, Op::LessEqual, Op::Greater, Op::GreaterEqual});
    if (!op.has_value())
      break;
    auto rhs_begin = it_;
    auto rhs = parseShift();
    if (!rhs)
      return lhs;
    auto rhs_end = it_;

    BinaryExpr be(op.value(), lhs, rhs, op_it->loc_,
                  LocRange(lhs_begin, lhs_end), LocRange(rhs_begin, rhs_end));
    lhs = std::make_shared<ExprAST>(std::move(be));
  }
  return lhs;
}

std::shared_ptr<ExprAST> Parser::parseShift() {
  auto lhs_begin = it_;
  auto lhs = parseAdditive();
  if (!lhs)
    return nullptr;
  auto lhs_end = it_;

  while (true) {
    auto op_it = it_;
    auto op =
        tryConsumeAny({Op::ShiftLeft, Op::ShiftRight, Op::ShiftUnsignedRight});
    if (!op.has_value())
      break;
    auto rhs_begin = it_;
    auto rhs = parseAdditive();
    if (!rhs)
      return lhs;
    auto rhs_end = it_;

    BinaryExpr be(op.value(), lhs, rhs, op_it->loc_,
                  LocRange(lhs_begin, lhs_end), LocRange(rhs_begin, rhs_end));
    lhs = std::make_shared<ExprAST>(std::move(be));
  }
  return lhs;
}

std::shared_ptr<ExprAST> Parser::parseAdditive() {
  auto lhs_begin = it_;
  auto lhs = parseMultiplicative();
  if (!lhs)
    return nullptr;
  auto lhs_end = it_;

  while (true) {
    auto op_it = it_;
    auto op = tryConsumeAny({Op::Add, Op::Sub});
    if (!op.has_value())
      break;
    auto rhs_begin = it_;
    auto rhs = parseMultiplicative();
    if (!rhs)
      return lhs;
    auto rhs_end = it_;

    BinaryExpr be(op.value(), lhs, rhs, op_it->loc_,
                  LocRange(lhs_begin, lhs_end), LocRange(rhs_begin, rhs_end));
    lhs = std::make_shared<ExprAST>(std::move(be));
  }
  return lhs;
}

std::shared_ptr<ExprAST> Parser::parseMultiplicative() {
  auto lhs_begin = it_;
  auto lhs = parseUnary();
  if (!lhs)
    return nullptr;
  auto lhs_end = it_;

  while (true) {
    auto op_it = it_;
    auto op = tryConsumeAny({Op::Mul, Op::Div, Op::Mod});
    if (!op.has_value())
      break;
    auto rhs_begin = it_;
    auto rhs = parseUnary();
    if (!rhs)
      return lhs;
    auto rhs_end = it_;

    BinaryExpr be(op.value(), lhs, rhs, op_it->loc_,
                  LocRange(lhs_begin, lhs_end), LocRange(rhs_begin, rhs_end));
    lhs = std::make_shared<ExprAST>(std::move(be));
  }
  return lhs;
}

std::shared_ptr<ExprAST> Parser::parseUnary() {
  std::vector<Op> ops;
  std::vector<iterator_t> opLocs;
  while (true) {
    auto loc = it_;
    auto op = tryConsumeAny({Op::Add, Op::Sub, Op::Tilde});
    if (!op.has_value())
      break;
    ops.push_back(op.value());
    opLocs.push_back(loc);
  }
  auto sub_begin = it_;
  auto node = parseExponentiation();
  if (!node)
    return nullptr;
  auto sub_end = it_;

  for (size_t i = ops.size(); i-- > 0;) {
    UnaryExpr ue(ops[i], node, opLocs[i]->loc_, LocRange(sub_begin, sub_end));
    node = std::make_shared<ExprAST>(std::move(ue));
    sub_begin = opLocs[i];
  }
  return node;
}

std::shared_ptr<ExprAST> Parser::parseExponentiation() {
  auto lhs_begin = it_;
  auto lhs = parsePostfix();
  if (!lhs)
    return nullptr;
  auto lhs_end = it_;

  while (true) {
    auto op_it = it_;
    auto op = tryConsumeAny({Op::Pow});
    if (!op.has_value())
      break;
    auto rhs_begin = it_;
    auto rhs = parsePostfix();
    if (!rhs)
      return lhs;
    auto rhs_end = it_;

    BinaryExpr be(op.value(), lhs, rhs, op_it->loc_,
                  LocRange(lhs_begin, lhs_end), LocRange(rhs_begin, rhs_end));
    lhs = std::make_shared<ExprAST>(std::move(be));
  }
  return lhs;
}

std::shared_ptr<ExprAST> Parser::parsePostfix() {
  auto primary_begin = it_;
  auto lhs = parsePrimary();
  if (!lhs)
    return nullptr;
  auto primary_end = it_;

  while (true) {
    // function call
    // -----------------------------------------------------------
    if (tryConsume<tok::ParenthesisL>()) {
      std::vector<std::shared_ptr<ExprAST>> args;
      std::vector<SourceLocation> argLocs;
      std::vector<std::pair<std::string_view, std::shared_ptr<ExprAST>>> kwargs;
      std::vector<SourceLocation> kwargsLocs;

      if (it_ != end_ && !it_->template HoldsAlternative<tok::ParenthesisR>()) {
        do {
          // Peek for keyword: <ID> '=' <Expression>
          if (it_->HoldsAlternative<tok::ID>() &&
              (std::next(it_) != end_ &&
               std::next(it_)->HoldsAlternative<tok::Operator>() &&
               std::next(it_)->GetIf<tok::Operator>()->op == Op::Assign)) {
            // keyword name
            std::string_view nameTok = it_->GetIf<tok::ID>()->id;
            auto nameLoc = it_->loc_;
            ++it_;  // consume ID
            require<tok::Operator>("expected '=' after keyword", Op::Assign);
            // parse value expression
            auto val = ParseExpression();
            if (val)
              kwargs.emplace_back(nameTok, val);
            kwargsLocs.emplace_back(nameLoc);
          } else {
            // positional argument
            auto argBegin = it_;
            auto expr = ParseExpression();
            if (expr)
              args.emplace_back(expr);
            argLocs.emplace_back(LocRange(argBegin, it_));
          }
        } while (tryConsume<tok::Operator>(Op::Comma));
      }
      require<tok::ParenthesisR>("Expected ')' after function call.");
      lhs = std::make_shared<ExprAST>(
          InvokeExpr{lhs, std::move(args), std::move(kwargs),
                     LocRange(primary_begin, primary_end), std::move(argLocs),
                     std::move(kwargsLocs)});
      continue;
    }

    // member access
    // -----------------------------------------------------------
    if (tryConsume<tok::Operator>(Op::Dot)) {
      if (it_ == end_ || !it_->template HoldsAlternative<tok::ID>()) {
        AddError("expected identifier after '.'", it_);
        Synchronize();
        return lhs;
      }
      tok::ID const* id_tok = it_->GetIf<tok::ID>();
      auto memberLoc = it_->loc_;
      ++it_;

      lhs = std::make_shared<ExprAST>(MemberExpr(
          lhs, id_tok->id, LocRange(primary_begin, primary_end), memberLoc));
      continue;
    }

    // subscript
    // ---------------------------------------------------------------
    if (tryConsume<tok::SquareL>()) {
      auto idx_begin = it_;
      auto idxExpr = ParseExpression();
      require<tok::SquareR>("Expected ']' after subscript.");
      auto idx_end = it_;
      lhs = std::make_shared<ExprAST>(
          SubscriptExpr(lhs, idxExpr, LocRange(primary_begin, primary_end),
                        LocRange(idx_begin, idx_end)));
      continue;
    }
    break;  // no postfix matched
  }
  return lhs;
}

std::shared_ptr<ExprAST> Parser::parsePrimary() {
  auto startLoc = it_;

  if (it_ == end_) {
    AddError("expected primary expression", it_);
    return nullptr;
  }
  // int literal
  // --------------------------------------------------------------
  if (auto s = it_->template GetIf<tok::Int>()) {
    auto node = IntLiteral(s->value, it_->loc_);
    ++it_;
    return std::make_shared<ExprAST>(std::move(node));
  }
  // string literal
  // -----------------------------------------------------------
  if (auto s = it_->template GetIf<tok::Literal>()) {
    auto node = StrLiteral(s->str, it_->loc_);
    ++it_;
    return std::make_shared<ExprAST>(std::move(node));
  }
  // identifier
  // ---------------------------------------------------------------
  if (auto s = it_->template GetIf<tok::ID>()) {
    auto node = Identifier(s->id, it_->loc_);
    ++it_;
    return std::make_shared<ExprAST>(std::move(node));
  }
  // parenthesised expression
  // --------------------------------------------------
  if (tryConsume<tok::ParenthesisL>()) {
    auto subBegin = it_;
    auto expr = ParseExpression();
    require<tok::ParenthesisR>("missing ')' in expression");
    auto subEnd = it_;
    return std::make_shared<ExprAST>(
        ParenExpr(expr, LocRange(subBegin, subEnd)));
  }
  // ListLiteral
  // --------------------------------------------------
  if (tryConsume<tok::SquareL>()) {
    std::vector<std::shared_ptr<ExprAST>> elems;

    if (!tryConsume<tok::SquareR>()) {
      // parse first sub-expression
      auto first = ParseExpression();
      // TODO: comprehension?

      elems = {first};
      while (tryConsume<tok::Operator>(Op::Comma))
        elems.emplace_back(ParseExpression());

      require<tok::SquareR>("expected ']'");
    }
    return std::make_shared<ExprAST>(
        ListLiteral{std::move(elems), LocRange(startLoc, it_)});
  }
  // DictLiteral
  // --------------------------------------------------
  if (tryConsume<tok::CurlyL>()) {
    std::vector<std::pair<std::shared_ptr<ExprAST>, std::shared_ptr<ExprAST>>>
        elms;

    if (!tryConsume<tok::CurlyR>()) {
      auto key = ParseExpression();
      require<tok::Colon>("expected ':'");
      auto val = ParseExpression();

      elms = {std::make_pair(key, val)};
      while (tryConsume<tok::Operator>(Op::Comma)) {
        key = ParseExpression();
        require<tok::Colon>("expected ':'");
        val = ParseExpression();
        elms.emplace_back(std::make_pair(key, val));
      }
      require<tok::CurlyR>("expected '}'");
    }

    return std::make_shared<ExprAST>(
        DictLiteral{std::move(elms), LocRange(startLoc, it_)});
  }

  AddError("expected primary expression", it_);
  Synchronize();
  return nullptr;
}

}  // namespace m6
