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

#include "m6/argparse.hpp"
#include "m6/value.hpp"
#include "utilities/mpl.hpp"

#include <functional>

namespace m6 {

class Function : public IValue {
 public:
  virtual std::type_index Type() const noexcept override {
    return typeid(
        std::function<Value(std::vector<Value>, std::map<std::string, Value>)>);
  }

  virtual Value Invoke(std::vector<Value> args,
                       std::map<std::string, Value> kwargs) override = 0;
};

namespace internal {
template <typename T>
struct function_traits;

// Specialization for function types
template <typename R, typename... Args>
struct function_traits<R(Args...)> {
  using result_type = R;
  using argument_types = TypeList<Args...>;
};

// Specialization for function pointers
template <typename R, typename... Args>
struct function_traits<R (*)(Args...)> : function_traits<R(Args...)> {};

// Specialization for member function pointers (non-const)
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...)> : function_traits<R(Args...)> {};

// Specialization for member function pointers (const) -- this catches lambda's
// operator()
template <typename C, typename R, typename... Args>
struct function_traits<R (C::*)(Args...) const> : function_traits<R(Args...)> {
};

template <typename F>
struct function_traits
    : function_traits<decltype(&std::remove_reference_t<F>::operator())> {};
}  // namespace internal

Value make_fn_value(std::string name, auto&& fn) {
  using fn_type = std::decay_t<decltype(fn)>;
  using R = typename internal::function_traits<fn_type>::result_type;
  using Argt = typename internal::function_traits<fn_type>::argument_types;

  class BasicFunction : public m6::Function {
   public:
    BasicFunction(std::string name, fn_type fn) : name_(name), fn_(fn) {}

    std::string Str() const override {
      return "<built-in function " + name_ + '>';
    }

    std::string Desc() const override {
      return "<wrapper 'basic function' of object " + name_ + '>';
    }

    Value Duplicate() override {
      return std::make_shared<BasicFunction>(*this);
    }

    Value Invoke(std::vector<Value> args,
                 std::map<std::string, Value> kwargs) override {
      auto arg = ParseArgs(Argt{}, std::move(args));
      if constexpr (std::same_as<R, void>) {
        std::apply(fn_, std::move(arg));
        return {};
      } else {
        const auto result = std::apply(fn_, std::move(arg));

        if constexpr (std::same_as<R, Value>)
          return result;
        else
          return make_value(result);
      }
    }

   private:
    std::string name_;
    fn_type fn_;
  };

  return std::make_shared<BasicFunction>(std::move(name),
                                         std::forward<fn_type>(fn));
}

}  // namespace m6
