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

#include "core/memory_internal/bank.hpp"
#include "core/memory_internal/location.hpp"

#include <array>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

struct LocalMemory {
  LocalMemory();
  ~LocalMemory();

  IntBankStorage A, B, C, D, E, F, X, H, I, J;
  StrBankStorage S, local_names;

  // boost::serialization support
  BOOST_SERIALIZATION_SPLIT_MEMBER();

  template <class Archive>
  void save(Archive& ar, unsigned int version) const {
    const size_t bank_count = 12;
    ar & bank_count;

    // save integer memory banks
    const std::array<std::pair<IntBank, IntBankStorage const*>, 10> bank{{
        {IntBank::A, &A}, {IntBank::B, &B}, {IntBank::C, &C}, {IntBank::D, &D},
        {IntBank::E, &E}, {IntBank::F, &F}, {IntBank::X, &X}, {IntBank::H, &H},
        {IntBank::I, &I}, {IntBank::J, &J},
    }};
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
          "LocalMemory::load: expected 12 local memory banks, but got " +
          std::to_string(bank_count));

    std::array<bool, static_cast<size_t>(IntBank::CNT)> seen_int{};
    std::array<bool, static_cast<size_t>(StrBank::CNT)> seen_str{};

    const auto intbank = [&](IntBank bank) -> IntBankStorage* {
      switch (bank) {
        case IntBank::A:
          return &A;
        case IntBank::B:
          return &B;
        case IntBank::C:
          return &C;
        case IntBank::D:
          return &D;
        case IntBank::E:
          return &E;
        case IntBank::F:
          return &F;
        case IntBank::X:
          return &X;
        case IntBank::H:
          return &H;
        case IntBank::I:
          return &I;
        case IntBank::J:
          return &J;
        default:
          throw std::runtime_error("LocalMemory::load: unexpected bank " +
                                   ToString(bank));
      }
    };

    const auto strbank = [&](StrBank bank) -> StrBankStorage* {
      switch (bank) {
        case StrBank::S:
          return &S;
        case StrBank::local_name:
          return &local_names;
        default:
          throw std::runtime_error("LocalMemory::load: unexpected bank " +
                                   ToString(bank));
      }
    };

    for (int i = 0; i < bank_count; ++i) {
      auto tag = DeserializeBankTag(ar);
      std::visit(
          [&](auto tag) {
            using T = std::decay_t<decltype(tag)>;
            if constexpr (std::is_same_v<T, IntBank>) {
              const auto index = static_cast<size_t>(tag);
              if (seen_int[index])
                throw std::runtime_error("LocalMemory::load: duplicate bank " +
                                         ToString(tag));
              auto* bank = intbank(tag);
              seen_int[index] = true;
              ar&* bank;
            } else if constexpr (std::is_same_v<T, StrBank>) {
              const auto index = static_cast<size_t>(tag);
              if (seen_str[index])
                throw std::runtime_error("LocalMemory::load: duplicate bank " +
                                         ToString(tag));
              auto* bank = strbank(tag);
              seen_str[index] = true;
              ar&* bank;
            }
          },
          tag);
    }

    const std::array<IntBank, 10> required_int{
        IntBank::A, IntBank::B, IntBank::C, IntBank::D, IntBank::E,
        IntBank::F, IntBank::X, IntBank::H, IntBank::I, IntBank::J};
    for (const auto bank : required_int) {
      if (!seen_int[static_cast<size_t>(bank)])
        throw std::runtime_error("LocalMemory::load: missing bank " +
                                 ToString(bank));
    }
    if (!seen_str[static_cast<size_t>(StrBank::S)])
      throw std::runtime_error("LocalMemory::load: missing bank strS");
    if (!seen_str[static_cast<size_t>(StrBank::local_name)])
      throw std::runtime_error("LocalMemory::load: missing bank LocalName");
  }
};
