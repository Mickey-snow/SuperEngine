// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2007 Elliot Glaysher
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

#include "machine/rlmachine.hpp"
#include "machine/rloperation.hpp"

template <typename... Args>
class RLStoreOpcode : public RLNormalOpcode<Args...> {
 public:
  RLStoreOpcode();
  virtual ~RLStoreOpcode();

  virtual void Dispatch(RLMachine& machine,
                        const libreallive::ExpressionPiecesVector& parameters);

  virtual int operator()(RLMachine&, typename Args::type...) = 0;

 private:
  template <std::size_t... I>
  void DispatchImpl(RLMachine& machine,
                    const std::tuple<typename Args::type...>& args,
                    std::index_sequence<I...> /*unused*/) {
    const int store = operator()(machine, std::get<I>(args)...);
    machine.set_store_register(store);
  }
};

template <typename... Args>
RLStoreOpcode<Args...>::RLStoreOpcode() {}

template <typename... Args>
RLStoreOpcode<Args...>::~RLStoreOpcode() {}

template <typename... Args>
void RLStoreOpcode<Args...>::Dispatch(
    RLMachine& machine,
    const libreallive::ExpressionPiecesVector& params) {
  unsigned int position = 0;
  const auto args = std::tuple<typename Args::type...>{
      Args::getData(machine, params, position)...};
  DispatchImpl(machine, args, std::make_index_sequence<sizeof...(Args)>());
}

template <>
void RLStoreOpcode<>::Dispatch(
    RLMachine& machine,
    const libreallive::ExpressionPiecesVector& parameters);
