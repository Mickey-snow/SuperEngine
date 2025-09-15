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

#include "vm/primops.hpp"

#include "vm/exception.hpp"

#include <bit>
#include <cmath>
#include <mutex>  // for std::call_once

namespace serilang::primops {

namespace {
enum class Kind : uint8_t { Nil = 0, Bool, Int, Double, Str, Object, Count };
inline static Kind kind_of(const Value& v) {
  switch (v.Type()) {
    case ObjType::Nil:
      return Kind::Nil;
    case ObjType::Bool:
      return Kind::Bool;
    case ObjType::Int:
      return Kind::Int;
    case ObjType::Double:
      return Kind::Double;
    case ObjType::Str:
      return Kind::Str;
    default:
      return Kind::Object;
  }
}

static const Value True(true);
static const Value False(false);

static Value handleIntIntOp(Op op, int lhs, int rhs) {
  switch (op) {
    // Arithmetic
    case Op::Add:
      return Value(lhs + rhs);
    case Op::Sub:
      return Value(lhs - rhs);
    case Op::Mul:
      return Value(lhs * rhs);
    case Op::Div:
      return (rhs == 0) ? Value(0) : Value(lhs / rhs);
    case Op::Mod:
      return (rhs == 0) ? Value(0) : Value(lhs % rhs);
    case Op::Pow: {
      // integer power via std::pow then cast
      double r = std::pow(static_cast<double>(lhs), static_cast<double>(rhs));
      return Value(static_cast<int>(r));
    }

    // Bitwise
    case Op::BitAnd:
      return Value(lhs & rhs);
    case Op::BitOr:
      return Value(lhs | rhs);
    case Op::BitXor:
      return Value(lhs ^ rhs);
    case Op::ShiftLeft: {
      if (rhs < 0)
        throw ValueError("negative shift count: " + std::to_string(rhs));
      int shifted = lhs << rhs;
      return Value(shifted);
    }
    case Op::ShiftRight: {
      if (rhs < 0)
        throw ValueError("negative shift count: " + std::to_string(rhs));
      int shifted = lhs >> rhs;
      return Value(shifted);
    }
    case Op::ShiftUnsignedRight: {
      if (rhs < 0)
        throw ValueError("negative shift count: " + std::to_string(rhs));
      uint32_t uval = std::bit_cast<uint32_t>(lhs);
      uint32_t shifted = uval >> rhs;
      return Value(static_cast<int>(shifted));
    }

    // Comparisons
    case Op::Equal:
      return (lhs == rhs) ? True : False;
    case Op::NotEqual:
      return (lhs != rhs) ? True : False;
    case Op::LessEqual:
      return (lhs <= rhs) ? True : False;
    case Op::Less:
      return (lhs < rhs) ? True : False;
    case Op::GreaterEqual:
      return (lhs >= rhs) ? True : False;
    case Op::Greater:
      return (lhs > rhs) ? True : False;

    // Logical (non short-circuit)
    case Op::LogicalAnd:
      return (lhs && rhs) ? True : False;
    case Op::LogicalOr:
      return (lhs || rhs) ? True : False;

    default:
      throw UndefinedOperator(op, {std::to_string(lhs), std::to_string(rhs)});
  }
}

static Value handleDoubleDoubleOp(Op op, double lhs, double rhs) {
  switch (op) {
    // Arithmetic
    case Op::Add:
      return Value(lhs + rhs);
    case Op::Sub:
      return Value(lhs - rhs);
    case Op::Mul:
      return Value(lhs * rhs);
    case Op::Div:
      return (rhs == 0.0) ? Value(0.0) : Value(lhs / rhs);
    case Op::Mod:
      return Value(std::fmod(lhs, rhs));
    case Op::Pow:
      return Value(std::pow(lhs, rhs));

    // Comparisons
    case Op::Equal:
      return (lhs == rhs) ? True : False;
    case Op::NotEqual:
      return (lhs != rhs) ? True : False;
    case Op::LessEqual:
      return (lhs <= rhs) ? True : False;
    case Op::Less:
      return (lhs < rhs) ? True : False;
    case Op::GreaterEqual:
      return (lhs >= rhs) ? True : False;
    case Op::Greater:
      return (lhs > rhs) ? True : False;

    // Logical
    case Op::LogicalAnd:
      return (lhs && rhs) ? True : False;
    case Op::LogicalOr:
      return (lhs || rhs) ? True : False;

    default:
      throw UndefinedOperator(op, {std::to_string(lhs), std::to_string(rhs)});
  }
}

static Value handleBoolBoolOp(Op op, bool lhs, bool rhs) {
  switch (op) {
    case Op::LogicalAnd:
      return (lhs && rhs) ? True : False;
    case Op::LogicalOr:
      return (lhs || rhs) ? True : False;
    case Op::Equal:
      return (lhs == rhs) ? True : False;
    case Op::NotEqual:
      return (lhs != rhs) ? True : False;
    default:
      throw UndefinedOperator(op,
                              {lhs ? "true" : "false", rhs ? "true" : "false"});
  }
}

static Value handleStringStringOp(Op op,
                                  const std::string& lhs,
                                  const std::string& rhs) {
  switch (op) {
    case Op::Equal:
      return (lhs == rhs) ? True : False;
    case Op::NotEqual:
      return (lhs != rhs) ? True : False;
    case Op::Add:
      return Value(lhs + rhs);
    default:
      throw UndefinedOperator(op, {lhs, rhs});
  }
}

static Value handleStringIntOp(Op op, std::string lhs, int rhs) {
  if (op == Op::Mul && rhs >= 0) {
    std::string result;
    result.reserve(lhs.size() * static_cast<size_t>(rhs));
    lhs.reserve(lhs.size() * static_cast<size_t>(rhs));
    while (rhs > 0) {
      if (rhs & 1) {
        result += lhs;
      }
      lhs += lhs;
      rhs >>= 1;
    }
    return Value(std::move(result));
  }

  throw UndefinedOperator(op, {"<str>", std::to_string(rhs)});
}

// ---- unary helpers ----
static Value unaryInt(Op op, int x) {
  switch (op) {
    case Op::Add:
      return Value(x);
    case Op::Sub:
      return Value(-x);
    case Op::Tilde:
      return Value(~x);
    default:
      throw UndefinedOperator(op, {std::to_string(x)});
  }
}

static Value unaryDouble(Op op, double x) {
  switch (op) {
    case Op::Add:
      return Value(x);
    case Op::Sub:
      return Value(-x);
    default:
      throw UndefinedOperator(op, {std::to_string(x)});
  }
}

static Value unaryBool(Op op, bool b) {
  switch (op) {
    case Op::Tilde:
      return Value(!b);
    default:
      throw UndefinedOperator(op, {b ? "true" : "false"});
  }
}

// dispatch table
using BinFn = Value (*)(Op, const Value&, const Value&);
using UnFn = Value (*)(Op, const Value&);
constexpr inline size_t K = static_cast<size_t>(Kind::Count);
static BinFn BIN[K][K] = {};
static UnFn UN[K] = {};

static std::once_flag is_init;
static void ensure_init() {
  std::call_once(is_init, []() {
    // Binary table
    BIN[static_cast<size_t>(Kind::Int)][static_cast<size_t>(Kind::Int)] =
        +[](Op op, const Value& l, const Value& r) {
          return handleIntIntOp(op, l.Get<int>(), r.Get<int>());
        };
    BIN[static_cast<size_t>(Kind::Int)][static_cast<size_t>(Kind::Double)] =
        +[](Op op, const Value& l, const Value& r) {
          return handleDoubleDoubleOp(op, static_cast<double>(l.Get<int>()),
                                      r.Get<double>());
        };
    BIN[static_cast<size_t>(Kind::Double)][static_cast<size_t>(Kind::Int)] =
        +[](Op op, const Value& l, const Value& r) {
          return handleDoubleDoubleOp(op, l.Get<double>(),
                                      static_cast<double>(r.Get<int>()));
        };
    BIN[static_cast<size_t>(Kind::Double)][static_cast<size_t>(Kind::Double)] =
        +[](Op op, const Value& l, const Value& r) {
          return handleDoubleDoubleOp(op, l.Get<double>(), r.Get<double>());
        };
    BIN[static_cast<size_t>(Kind::Bool)][static_cast<size_t>(Kind::Bool)] =
        +[](Op op, const Value& l, const Value& r) {
          return handleBoolBoolOp(op, l.Get<bool>(), r.Get<bool>());
        };
    BIN[static_cast<size_t>(Kind::Int)][static_cast<size_t>(Kind::Bool)] =
        +[](Op op, const Value& l, const Value& r) {
          return handleIntIntOp(op, l.Get<int>(), r.Get<bool>() ? 1 : 0);
        };
    BIN[static_cast<size_t>(Kind::Bool)][static_cast<size_t>(Kind::Int)] =
        +[](Op op, const Value& l, const Value& r) {
          return handleIntIntOp(op, l.Get<bool>() ? 1 : 0, r.Get<int>());
        };
    BIN[static_cast<size_t>(Kind::Double)][static_cast<size_t>(Kind::Bool)] =
        +[](Op op, const Value& l, const Value& r) {
          return handleDoubleDoubleOp(op, l.Get<double>(),
                                      r.Get<bool>() ? 1.0 : 0.0);
        };
    BIN[static_cast<size_t>(Kind::Bool)][static_cast<size_t>(Kind::Double)] =
        +[](Op op, const Value& l, const Value& r) {
          return handleDoubleDoubleOp(op, l.Get<bool>() ? 1.0 : 0.0,
                                      r.Get<double>());
        };
    BIN[static_cast<size_t>(Kind::Str)][static_cast<size_t>(Kind::Int)] =
        +[](Op op, const Value& l, const Value& r) {
          return handleStringIntOp(op, l.Get<std::string>(), r.Get<int>());
        };
    BIN[static_cast<size_t>(Kind::Str)][static_cast<size_t>(Kind::Str)] =
        +[](Op op, const Value& l, const Value& r) {
          return handleStringStringOp(op, l.Get<std::string>(),
                                      r.Get<std::string>());
        };

    // Unary table
    UN[static_cast<size_t>(Kind::Int)] =
        +[](Op op, const Value& v) { return unaryInt(op, v.Get<int>()); };
    UN[static_cast<size_t>(Kind::Double)] =
        +[](Op op, const Value& v) { return unaryDouble(op, v.Get<double>()); };
    UN[static_cast<size_t>(Kind::Bool)] =
        +[](Op op, const Value& v) { return unaryBool(op, v.Get<bool>()); };
  });
}

}  // namespace

// public API

std::optional<Value> EvaluateBinary(Op op, const Value& lhs, const Value& rhs) {
  ensure_init();

  const auto lk = kind_of(lhs);
  const auto rk = kind_of(rhs);

  const auto f = BIN[static_cast<size_t>(lk)][static_cast<size_t>(rk)];
  if (!f)
    return std::nullopt;
  return f(op, lhs, rhs);
}

std::optional<Value> EvaluateUnary(Op op, const Value& v) {
  ensure_init();

  const auto k = kind_of(v);
  const auto f = UN[static_cast<size_t>(k)];
  if (!f)
    return std::nullopt;

  return f(op, v);
}

}  // namespace serilang::primops
