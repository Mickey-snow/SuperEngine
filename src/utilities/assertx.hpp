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

#pragma once

#include <cassert>
#include <charconv>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <iomanip>
#include <iostream>
#include <optional>
#include <source_location>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <vector>

#include <boost/stacktrace.hpp>

#ifndef ASSERTX_ENABLE
// Always-on by default. Define ASSERTX_ENABLE=0 to compile out.
#define ASSERTX_ENABLE 1
#endif

#ifndef ASSERTX_TERMINATE_ON_FAIL
#define ASSERTX_TERMINATE_ON_FAIL 1
#endif

namespace assertx {

struct AssertionFailure : std::runtime_error {
  using std::runtime_error::runtime_error;
};

namespace detail {

// Try to stream a value; if that fails, fall back to a type name placeholder.
template <class T>
auto to_string_fallback(const T& v) -> std::string {
  std::ostringstream oss;
  if constexpr (requires(std::ostream& os, const T& x) { os << x; }) {
    oss << v;
    return oss.str();
  } else {
#if defined(__GNUC__) || defined(__clang__)
    return std::string("{") + typeid(T).name() + "}";
#else
    return "{value}";
#endif
  }
}

inline std::string build_header(std::string_view kind,
                                std::string_view expr,
                                std::source_location loc) {
  std::ostringstream oss;
  oss << "[assertx] " << kind << " failed\n"
      << "  expr : " << expr << "\n"
      << "  file : " << loc.file_name() << ":" << loc.line() << " ("
      << loc.function_name() << ")\n";
  return oss.str();
}

[[noreturn]] inline void fail_now(std::string&& full, bool should_terminate) {
  std::cerr << full;
  if (should_terminate) {
    std::abort();
  }
  throw AssertionFailure(std::move(full));
}

}  // namespace detail

template <class L, class R>
inline void eq(const L& lhs,
               const R& rhs,
               std::string_view expr = {},
               std::source_location loc = std::source_location::current()) {
#if ASSERTX_ENABLE
  if (!(lhs == rhs)) {
    std::ostringstream oss;
    oss << detail::build_header("eq", expr, loc)
        << "  left : " << detail::to_string_fallback(lhs) << "\n"
        << "  right: " << detail::to_string_fallback(rhs) << "\n"
        << "--- stacktrace ---\n"
        << boost::stacktrace::stacktrace();
    detail::fail_now(std::move(oss).str(), ASSERTX_TERMINATE_ON_FAIL);
  }
#endif
}

template <class L, class R>
inline void ne(const L& lhs,
               const R& rhs,
               std::string_view expr = {},
               std::source_location loc = std::source_location::current()) {
#if ASSERTX_ENABLE
  if (!(lhs != rhs)) {
    std::ostringstream oss;
    oss << detail::build_header("ne", expr, loc)
        << "  left : " << detail::to_string_fallback(lhs) << "\n"
        << "  right: " << detail::to_string_fallback(rhs) << "\n"
        << "--- stacktrace ---\n"
        << boost::stacktrace::stacktrace();
    detail::fail_now(std::move(oss).str(), ASSERTX_TERMINATE_ON_FAIL);
  }
#endif
}

template <class L, class R>
inline void lt(const L& lhs,
               const R& rhs,
               std::string_view expr = {},
               std::source_location loc = std::source_location::current()) {
#if ASSERTX_ENABLE
  if (!(lhs < rhs)) {
    std::ostringstream oss;
    oss << detail::build_header("lt", expr, loc)
        << "  left : " << detail::to_string_fallback(lhs) << "\n"
        << "  right: " << detail::to_string_fallback(rhs) << "\n"
        << "--- stacktrace ---\n"
        << boost::stacktrace::stacktrace();
    detail::fail_now(std::move(oss).str(), ASSERTX_TERMINATE_ON_FAIL);
  }
#endif
}

template <class L, class R>
inline void le(const L& lhs,
               const R& rhs,
               std::string_view expr = {},
               std::source_location loc = std::source_location::current()) {
#if ASSERTX_ENABLE
  if (!(lhs <= rhs)) {
    std::ostringstream oss;
    oss << detail::build_header("le", expr, loc)
        << "  left : " << detail::to_string_fallback(lhs) << "\n"
        << "  right: " << detail::to_string_fallback(rhs) << "\n"
        << "--- stacktrace ---\n"
        << boost::stacktrace::stacktrace();
    detail::fail_now(std::move(oss).str(), ASSERTX_TERMINATE_ON_FAIL);
  }
#endif
}

template <class L, class R>
inline void gt(const L& lhs,
               const R& rhs,
               std::string_view expr = {},
               std::source_location loc = std::source_location::current()) {
#if ASSERTX_ENABLE
  if (!(lhs > rhs)) {
    std::ostringstream oss;
    oss << detail::build_header("gt", expr, loc)
        << "  left : " << detail::to_string_fallback(lhs) << "\n"
        << "  right: " << detail::to_string_fallback(rhs) << "\n"
        << "--- stacktrace ---\n"
        << boost::stacktrace::stacktrace();
    detail::fail_now(std::move(oss).str(), ASSERTX_TERMINATE_ON_FAIL);
  }
#endif
}

template <class L, class R>
inline void ge(const L& lhs,
               const R& rhs,
               std::string_view expr = {},
               std::source_location loc = std::source_location::current()) {
#if ASSERTX_ENABLE
  if (!(lhs >= rhs)) {
    std::ostringstream oss;
    oss << detail::build_header("ge", expr, loc)
        << "  left : " << detail::to_string_fallback(lhs) << "\n"
        << "  right: " << detail::to_string_fallback(rhs) << "\n"
        << "--- stacktrace ---\n"
        << boost::stacktrace::stacktrace();
    detail::fail_now(std::move(oss).str(), ASSERTX_TERMINATE_ON_FAIL);
  }
#endif
}

inline void is_true(
    bool cond,
    std::string_view expr = {},
    std::source_location loc = std::source_location::current()) {
#if ASSERTX_ENABLE
  if (!cond) {
    std::ostringstream oss;
    oss << detail::build_header("true", expr, loc) << "  value: false\n"
        << "--- stacktrace ---\n"
        << boost::stacktrace::stacktrace();
    detail::fail_now(std::move(oss).str(), ASSERTX_TERMINATE_ON_FAIL);
  }
#endif
}

inline void is_false(
    bool cond,
    std::string_view expr = {},
    std::source_location loc = std::source_location::current()) {
#if ASSERTX_ENABLE
  if (cond) {
    std::ostringstream oss;
    oss << detail::build_header("false", expr, loc) << "  value: true\n"
        << "--- stacktrace ---\n"
        << boost::stacktrace::stacktrace();
    detail::fail_now(std::move(oss).str(), ASSERTX_TERMINATE_ON_FAIL);
  }
#endif
}

#if ASSERTX_ENABLE

#define ASSERTX_TRUE(expr) ::assertx::is_true((expr), #expr)
#define ASSERTX_FALSE(expr) ::assertx::is_false((expr), #expr)

#define ASSERTX_EQ(a, b) ::assertx::eq((a), (b), #a " == " #b)
#define ASSERTX_NE(a, b) ::assertx::ne((a), (b), #a " != " #b)
#define ASSERTX_LT(a, b) ::assertx::lt((a), (b), #a " < " #b)
#define ASSERTX_LE(a, b) ::assertx::le((a), (b), #a " <= " #b)
#define ASSERTX_GT(a, b) ::assertx::gt((a), (b), #a " > " #b)
#define ASSERTX_GE(a, b) ::assertx::ge((a), (b), #a " >= " #b)

#else  // disabled: compile to no-ops

#define ASSERTX_TRUE(expr) ((void)0)
#define ASSERTX_FALSE(expr) ((void)0)
#define ASSERTX_EQ(a, b) ((void)0)
#define ASSERTX_NE(a, b) ((void)0)
#define ASSERTX_LT(a, b) ((void)0)
#define ASSERTX_LE(a, b) ((void)0)
#define ASSERTX_GT(a, b) ((void)0)
#define ASSERTX_GE(a, b) ((void)0)

#endif

}  // namespace assertx
