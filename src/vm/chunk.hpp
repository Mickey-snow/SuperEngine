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

#include "vm/instruction.hpp"
#include "vm/value.hpp"

#include <bit>
#include <cstddef>
#include <vector>

namespace serilang {

struct Chunk {
  std::vector<std::byte> code;
  std::vector<Value> const_pool;

  [[nodiscard]] std::byte operator[](const std::size_t idx) const {
    return code[idx];
  }

  // helpers ------------------------------------------------------
  // Encode *one* concrete instruction type and push its bytes.
  template <typename T>
    requires(std::constructible_from<Instruction, T> &&
             std::is_trivially_copyable_v<T>)
  void Append(T v) {
    // opcode byte
    code.emplace_back(std::bit_cast<std::byte>(GetOpcode<T>()));

    // payload
    if constexpr (sizeof(T) > 0) {
      auto* p = reinterpret_cast<const std::byte*>(&v);
      code.insert(code.end(), p, p + sizeof(T));
    }
  }

  // Variant-aware overload – forwards to the typed version above.
  void Append(const Instruction& ins) {
    std::visit([this](auto&& op) { Append(op); }, ins);
  }

  template <typename T>
    requires std::is_trivially_copyable_v<T>
  void Write(const std::size_t idx, T v) {
    auto* p = reinterpret_cast<std::byte*>(&v);
    std::copy(p, p + sizeof(T), code.begin() + idx);
  }

  template <typename T>
  [[nodiscard]] T Read(std::size_t ip) const {
    using ptr_t = std::add_pointer_t<std::add_const_t<T>>;
    auto* p = reinterpret_cast<ptr_t>(code.data() + ip);
    return *p;
  }
};

}  // namespace serilang
