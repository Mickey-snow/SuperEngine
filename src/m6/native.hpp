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
#include "machine/value.hpp"

namespace m6 {

Value make_fn_value(std::string name, auto&& fn) {
  using fn_type = std::decay_t<decltype(fn)>;
  using R = typename function_traits<fn_type>::result_type;
  using Argt = typename function_traits<fn_type>::argument_types;

  class NativeImpl : public NativeFunction {
   public:
    NativeImpl(std::string name, fn_type fn)
        : NativeFunction(std::move(name)), fn_(fn) {}

    virtual Value Invoke(RLMachine* /*unused*/,
                         std::vector<Value> args) override {
      auto arg = ParseArgs(Argt{}, args.begin(), args.end());

      if constexpr (std::same_as<R, void>) {
        std::apply(fn_, std::move(arg));
        return Value(std::monostate());
      } else {
        auto result = std::apply(fn_, std::move(arg));
        if constexpr (std::same_as<R, Value>)
          return result;
        else
          return Value(std::move(result));
      }
    }

   private:
    fn_type fn_;
  };

  std::shared_ptr<IObject> fn_obj =
      std::make_shared<NativeImpl>(std::move(name), std::forward<fn_type>(fn));
  return Value(std::move(fn_obj));
}

}  // namespace m6
