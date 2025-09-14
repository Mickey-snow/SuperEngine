// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Centralized primitive operator tables and helpers.
//
// -----------------------------------------------------------------------

#pragma once

#include "machine/op.hpp"
#include "vm/value.hpp"

namespace serilang::primops {

enum class Kind : uint8_t { Nil = 0, Bool, Int, Double, Str, Object, Count };

inline Kind kind_of(const Value& v) {
  // Keep in sync with Value::value_t order
  return v.Apply<Kind>([](auto const& x) -> Kind {
    using T = std::decay_t<decltype(x)>;
    if constexpr (std::same_as<T, std::monostate>)
      return Kind::Nil;
    else if constexpr (std::same_as<T, bool>)
      return Kind::Bool;
    else if constexpr (std::same_as<T, int>)
      return Kind::Int;
    else if constexpr (std::same_as<T, double>)
      return Kind::Double;
    else if constexpr (std::same_as<T, std::string>)
      return Kind::Str;
    else
      return Kind::Object;
  });
}

using BinFn = Value (*)(Op, const Value&, const Value&);
using UnFn = Value (*)(Op, const Value&);

// Evaluate primitive binary op. Returns true and sets out if handled.
bool EvaluateBinary(Op op, const Value& lhs, const Value& rhs, Value& out);

// Evaluate primitive unary op. Returns true and sets out if handled.
bool EvaluateUnary(Op op, const Value& v, Value& out);

}  // namespace serilang::primops

