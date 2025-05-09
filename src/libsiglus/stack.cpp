// -----------------------------------------------------------------------
//
// This file is part of RLVM
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2024 Serina Sakurai
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
// -----------------------------------------------------------------------

#include "libsiglus/stack.hpp"

#include "utilities/string_utilities.hpp"

#include <format>
#include <stdexcept>

namespace libsiglus {

const char* StackUnderflow::what() const noexcept {
  return "Stack underflow: Attempted to access an element from an empty stack.";
}

// -----------------------------------------------------------------------
// class libsiglus::Stack
Stack::Stack() = default;
Stack::~Stack() = default;

bool Stack::Empty() const {
  return intstk_.empty() && strstk_.empty() && elm_point_.empty();
}

void Stack::Clear() {
  intstk_.clear();
  strstk_.clear();
  elm_point_.clear();
}

Stack& Stack::Push(Value v) {
  switch (Typeof(v)) {
    case Type::Int:
      intstk_.emplace_back(std::move(v));
      break;
    case Type::String:
      strstk_.emplace_back(std::move(v));
      break;
    default:
      // ignore
      break;
  }
  return *this;
}

Stack& Stack::PushMarker() {
  elm_point_.push_back(intstk_.size());
  return *this;
}

Stack& Stack::Push(const ElementCode& elm) {
  PushMarker();
  for (const auto& it : elm)
    intstk_.push_back(Integer(it));
  return *this;
}

Value Stack::Backint() const {
  if (intstk_.empty()) {
    throw StackUnderflow();
  }
  return intstk_.back();
}

Value& Stack::Backint() {
  if (intstk_.empty()) {
    throw StackUnderflow();
  }
  return intstk_.back();
}

Value Stack::Popint() {
  if (intstk_.empty()) {
    throw StackUnderflow();
  }
  auto result = intstk_.back();
  intstk_.pop_back();
  if (!elm_point_.empty() && elm_point_.back() >= intstk_.size())
    elm_point_.pop_back();
  return result;
}

const Value& Stack::Backstr() const {
  if (strstk_.empty()) {
    throw StackUnderflow();
  }
  return strstk_.back();
}

Value& Stack::Backstr() {
  if (strstk_.empty()) {
    throw StackUnderflow();
  }
  return strstk_.back();
}

Value Stack::Popstr() {
  if (strstk_.empty()) {
    throw StackUnderflow();
  }
  auto result = std::move(strstk_.back());
  strstk_.pop_back();
  return result;
}

Value Stack::Pop(Type type) {
  if (type == Type::Int)
    return Popint();
  else if (type == Type::String)
    return Popstr();
  else {
    throw std::invalid_argument("Stack: unknown type " +
                                std::to_string(static_cast<int>(type)));
  }
}

ElementCode Stack::Backelm() const {
  if (elm_point_.empty())
    throw StackUnderflow();
  ElementCode result  // (intstk_.cbegin() + elm_point_.back(), intstk_.cend())
      ;

  result.reserve(intstk_.size() - elm_point_.back());
  std::transform(intstk_.cbegin() + elm_point_.back(), intstk_.cend(),
                 std::back_inserter(result), [](const Value& v) -> int {
                   if (auto i = std::get_if<Integer>(&v))
                     return i->val_;
                   else
                     throw std::runtime_error("Stack: cannot convert " +
                                              ToString(v) + " to integer.");
                 });
  return result;
}

ElementCode Stack::Popelm() {
  auto result = Backelm();
  intstk_.resize(elm_point_.back());
  elm_point_.pop_back();
  return result;
}

std::string Stack::ToDebugString() const {
  auto to_string = [](const std::vector<Value>& vec) {
    return std::views::all(vec) |
           std::views::transform([](const Value& v) { return ToString(v); });
  };

  std::string result;
  result += std::format("int: {}\n", Join(",", to_string(intstk_)));
  result += std::format("str: {}\n", Join(",", to_string(strstk_)));
  result += std::format("elm: {}\n", Join(",", view_to_string(elm_point_)));
  return result;
}

}  // namespace libsiglus
