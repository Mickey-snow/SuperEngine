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

#ifndef SRC_LIBSIGLUS_STACK_HPP_
#define SRC_LIBSIGLUS_STACK_HPP_

#include "libsiglus/types.hpp"
#include "libsiglus/value.hpp"

#include <stack>
#include <stdexcept>
#include <string>
#include <vector>

namespace libsiglus {

using ElementCode = std::vector<int>;

class StackUnderflow : public std::exception {
 public:
  const char* what() const noexcept override {
    return "Stack underflow: Attempted to access an element from an empty "
           "stack.";
  }
};

class Stack {
 public:
  Stack() = default;
  ~Stack() = default;

  void Clearint() {
    intstk_.clear();
    elm_point_ = decltype(elm_point_){};
  }
  void Clearstr() { strstk_.clear(); }
  void Clear() {
    Clearint();
    Clearstr();
  }

  Stack& Push(int value) {
    intstk_.push_back(value);
    return *this;
  }
  Stack& Push(std::string value) {
    strstk_.emplace_back(std::move(value));
    return *this;
  }
  Stack& PushMarker() {
    elm_point_.push(intstk_.size());
    return *this;
  }
  Stack& Push(const ElementCode& elm) {
    PushMarker();
    for (const auto& it : elm)
      intstk_.push_back(it);
    return *this;
  }

  int Backint() const {
    if (intstk_.empty()) {
      throw StackUnderflow();
    }
    return intstk_.back();
  }
  int& Backint() {
    if (intstk_.empty()) {
      throw StackUnderflow();
    }
    return intstk_.back();
  }
  int Popint() {
    if (intstk_.empty()) {
      throw StackUnderflow();
    }
    int result = intstk_.back();
    intstk_.pop_back();
    if (!elm_point_.empty() && elm_point_.top() >= intstk_.size())
      elm_point_.pop();
    return result;
  }

  std::string const& Backstr() const {
    if (strstk_.empty()) {
      throw StackUnderflow();
    }
    return strstk_.back();
  }
  std::string& Backstr() {
    if (strstk_.empty()) {
      throw StackUnderflow();
    }
    return strstk_.back();
  }
  std::string Popstr() {
    if (strstk_.empty()) {
      throw StackUnderflow();
    }
    std::string result = std::move(strstk_.back());
    strstk_.pop_back();
    return result;
  }

  Value Pop(Type type) {
    if (type == Type::Int)
      return Popint();
    else if (type == Type::String)
      return Popstr();
    else
      throw std::invalid_argument("Stack: unknown type " +
                                  std::to_string(static_cast<int>(type)));
  }

  ElementCode Backelm() const {
    if (elm_point_.empty())
      throw StackUnderflow();
    ElementCode result(intstk_.cbegin() + elm_point_.top(), intstk_.cend());
    return result;
  }
  ElementCode Popelm() {
    auto result = Backelm();
    intstk_.resize(elm_point_.top());
    elm_point_.pop();
    return result;
  }

 private:
  std::vector<int> intstk_;
  std::vector<std::string> strstk_;
  std::stack<size_t> elm_point_;
};

}  // namespace libsiglus

#endif
