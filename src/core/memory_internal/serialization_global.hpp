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

#include <stdexcept>
#include <string>
#include <type_traits>

struct GlobalMemory {
  IntBankStorage G, Z;
  StrBankStorage M, global_names;

  // boost::serialization support
  template <class Archive>
  void save(Archive& ar, unsigned int version) const {
    const size_t bank_count = 4;
    ar & bank_count;

    SerializeBankTag(ar, IntBank::G);
    ar & G;
    SerializeBankTag(ar, IntBank::Z);
    ar & Z;
    SerializeBankTag(ar, StrBank::M);
    ar & M;
    SerializeBankTag(ar, StrBank::global_name);
    ar & global_names;
  }

  template <class Archive>
  void load(Archive& ar, unsigned int version) {
    size_t bank_count;
    ar & bank_count;

    if (bank_count != 4)
      throw std::runtime_error(
          "GlobalMemory::load: expected 4 global memory banks, got " +
          std::to_string(bank_count));

    bool seen_g = false;
    bool seen_z = false;
    bool seen_m = false;
    bool seen_global_names = false;

    for (int i = 0; i < 4; ++i) {
      auto tag = DeserializeBankTag(ar);
      std::visit(
          [&](auto tag) {
            using T = std::decay_t<decltype(tag)>;
            if constexpr (std::is_same_v<T, IntBank>) {
              if (tag == IntBank::G) {
                if (seen_g)
                  throw std::runtime_error(
                      "GlobalMemory::load: duplicate bank intG");
                seen_g = true;
                ar & G;
              } else if (tag == IntBank::Z) {
                if (seen_z)
                  throw std::runtime_error(
                      "GlobalMemory::load: duplicate bank intZ");
                seen_z = true;
                ar & Z;
              } else {
                throw std::runtime_error(
                    "GlobalMemory::load: unexpected bank " + ToString(tag));
              }
            }
            if constexpr (std::is_same_v<T, StrBank>) {
              if (tag == StrBank::M) {
                if (seen_m)
                  throw std::runtime_error(
                      "GlobalMemory::load: duplicate bank strM");
                seen_m = true;
                ar & M;
              } else if (tag == StrBank::global_name) {
                if (seen_global_names)
                  throw std::runtime_error(
                      "GlobalMemory::load: duplicate bank GlobalName");
                seen_global_names = true;
                ar & global_names;
              } else {
                throw std::runtime_error(
                    "GlobalMemory::load: unexpected bank " + ToString(tag));
              }
            }
          },
          tag);
    }

    if (!seen_g || !seen_z || !seen_m || !seen_global_names)
      throw std::runtime_error("GlobalMemory::load: missing required bank");
  }

  BOOST_SERIALIZATION_SPLIT_MEMBER();
};
