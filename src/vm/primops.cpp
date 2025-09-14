// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------

#include "vm/primops.hpp"

#include "vm/exception.hpp"

#include <bit>
#include <cmath>

namespace serilang::primops {

namespace {

static const Value True(true);
static const Value False(false);

Value handleIntIntOp(Op op, int lhs, int rhs) {
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

Value handleDoubleDoubleOp(Op op, double lhs, double rhs) {
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

Value handleBoolBoolOp(Op op, bool lhs, bool rhs) {
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

Value handleStringStringOp(Op op,
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

Value handleStringIntOp(Op op, std::string lhs, int rhs) {
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
Value unaryInt(Op op, int x) {
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

Value unaryDouble(Op op, double x) {
  switch (op) {
    case Op::Add:
      return Value(x);
    case Op::Sub:
      return Value(-x);
    default:
      throw UndefinedOperator(op, {std::to_string(x)});
  }
}

Value unaryBool(Op op, bool b) {
  switch (op) {
    case Op::Tilde:
      return Value(!b);
    default:
      throw UndefinedOperator(op, {b ? "true" : "false"});
  }
}

// ---- dispatch tables ----
constexpr size_t K = static_cast<size_t>(Kind::Count);

BinFn BIN[K][K] = {};
UnFn UN[static_cast<size_t>(Kind::Count)] = {};

bool inited = false;

void ensure_init() {
  if (inited)
    return;
  inited = true;

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
        return handleDoubleDoubleOp(op, l.Get<double>(), r.Get<bool>() ? 1.0
                                                                       : 0.0);
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
}

}  // namespace

bool EvaluateBinary(Op op, const Value& lhs, const Value& rhs, Value& out) {
  ensure_init();
  const auto lk = kind_of(lhs);
  const auto rk = kind_of(rhs);

  const auto f = BIN[static_cast<size_t>(lk)][static_cast<size_t>(rk)];
  if (!f)
    return false;
  out = f(op, lhs, rhs);
  return true;
}

bool EvaluateUnary(Op op, const Value& v, Value& out) {
  ensure_init();
  const auto k = kind_of(v);
  const auto f = UN[static_cast<size_t>(k)];
  if (!f)
    return false;
  out = f(op, v);
  return true;
}

}  // namespace serilang::primops

