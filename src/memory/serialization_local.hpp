// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
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
//
// -----------------------------------------------------------------------

#pragma once

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/split_member.hpp>

#include "memory/bank.hpp"
#include "memory/location.hpp"

#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>

struct LocalMemory {
  LocalMemory();
  ~LocalMemory();

  MemoryBank<int> A, B, C, D, E, F, X, H, I, J;
  MemoryBank<std::string> S, local_names;

  // boost::serialization support
  BOOST_SERIALIZATION_SPLIT_MEMBER();

  template <class Archive>
  void save(Archive& ar, unsigned int version) const {
    const size_t bank_count = 12;
    ar & bank_count;

    // save integer memory banks
    const std::unordered_map<IntBank, MemoryBank<int> const*> bank{
        {IntBank::A, &A}, {IntBank::B, &B}, {IntBank::C, &C}, {IntBank::D, &D},
        {IntBank::E, &E}, {IntBank::F, &F}, {IntBank::X, &X}, {IntBank::H, &H},
        {IntBank::I, &I}, {IntBank::J, &J},
    };
    for (const auto [tag, ptr] : bank) {
      SerializeBankTag(ar, tag);
      ar&(*ptr);
    }

    // save string memory banks
    SerializeBankTag(ar, StrBank::S);
    ar & S;
    SerializeBankTag(ar, StrBank::local_name);
    ar & local_names;
  }

  template <class Archive>
  void load(Archive& ar, unsigned int version) {
    size_t bank_count;
    ar & bank_count;

    if (bank_count != 12)
      throw std::runtime_error(
          "GlobalMemory::load: expected 12 local memory banks, but got " +
          std::to_string(bank_count));

    const std::unordered_map<IntBank, MemoryBank<int>*> intbank{
        {IntBank::A, &A}, {IntBank::B, &B}, {IntBank::C, &C}, {IntBank::D, &D},
        {IntBank::E, &E}, {IntBank::F, &F}, {IntBank::X, &X}, {IntBank::H, &H},
        {IntBank::I, &I}, {IntBank::J, &J},
    };
    const std::unordered_map<StrBank, MemoryBank<std::string>*> strbank{
        {StrBank::S, &S}, {StrBank::local_name, &local_names}};

    for (int i = 0; i < bank_count; ++i) {
      auto tag = DeserializeBankTag(ar);
      std::visit(
          [&](auto tag) {
            using T = std::decay_t<decltype(tag)>;
            if constexpr (std::is_same_v<T, IntBank>)
              ar&* intbank.at(tag);
            else if constexpr (std::is_same_v<T, StrBank>)
              ar&* strbank.at(tag);
          },
          tag);
    }
  }
};
