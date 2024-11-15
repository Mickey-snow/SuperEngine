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

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <type_traits>
#include <vector>

/**
 * @class ByteInserter
 *
 * @brief A custom output iterator for inserting arbitrary trivially copyable
 * data types into a byte buffer.
 */
class ByteInserter {
 public:
  using ByteBuffer_t = std::vector<uint8_t>;

  /* Iterator traits */
  using iterator_category = std::output_iterator_tag;
  using value_type = void;
  using difference_type = std::ptrdiff_t;
  using pointer = void;
  using reference = void;

  ByteInserter(ByteBuffer_t& buf) : back_(std::back_inserter(buf)) {}

  /**
   * @brief Inserts a trivially copyable value or a string into the byte buffer.
   *
   * @tparam T The type of data to be inserted.
   * @param val The value to be inserted into the byte buffer.
   * @return A reference to this `ByteInserter` instance.
   */
  template <typename T>
  ByteInserter& operator=(const T& val) {
    if constexpr (std::is_same_v<T, std::string> ||
                  std::is_same_v<T, std::string_view>) {
      std::copy(val.cbegin(), val.cend(), back_);

    } else if constexpr (std::is_same_v<std::decay_t<T>, char*>) {
      std::copy_n(val, std::strlen(val), back_);
    } else {
      static_assert(std::is_trivially_copyable<T>::value,
                    "T must be string or trivially copyable");

      auto ptr = reinterpret_cast<const uint8_t*>(&val);
      std::copy(ptr, ptr + sizeof(T), back_);
    }
    return *this;
  }

  ByteInserter& operator*() { return *this; }
  ByteInserter& operator++() { return *this; }
  ByteInserter& operator++(int) { return *this; }

 private:
  std::back_insert_iterator<ByteBuffer_t> back_;
};
