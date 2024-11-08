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

#ifndef SRC_MEMORY_SERIALIZATION_GLOBAL_HPP_
#define SRC_MEMORY_SERIALIZATION_GLOBAL_HPP_

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/split_member.hpp>

#include "memory/bank.hpp"
#include "memory/location.hpp"

#include <stdexcept>
#include <string>
#include <type_traits>

struct _GlobalMemory {
  MemoryBank<int> G, Z;
  MemoryBank<std::string> M, global_names;

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

    for (int i = 0; i < 4; ++i) {
      auto tag = DeserializeBankTag(ar);
      std::visit(
          [&](auto tag) {
            using T = std::decay_t<decltype(tag)>;
            if constexpr (std::is_same_v<T, IntBank>) {
              if (tag == IntBank::G)
                ar & G;
              else if (tag == IntBank::Z)
                ar & Z;
            }
            if constexpr (std::is_same_v<T, StrBank>) {
              if (tag == StrBank::M)
                ar & M;
              else if (tag == StrBank::global_name)
                ar & global_names;
            }
          },
          tag);
    }
  }

  BOOST_SERIALIZATION_SPLIT_MEMBER();
};

#endif
