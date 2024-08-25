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

#ifndef SRC_UTILITIES_BITSTREAM_H_
#define SRC_UTILITIES_BITSTREAM_H_

#include <cstddef>
#include <cstdint>
#include <iterator>

/**
 * @class BitStream
 * @brief A class to interpret and manipulate a byte array as a stream of bits.
 */
class BitStream {
 public:
  /**
   * @brief Constructor that initializes the BitStream with a byte array.
   * @param data Pointer to the byte array.
   * @param length Length of the byte array in bytes.
   */
  BitStream(uint8_t* data, size_t length);

  /**
   * @brief Templated constructor that initializes the BitStream with a range of
   * elements.
   * @tparam T Type of the elements in the range.
   * @param begin Pointer to the beginning of the range.
   * @param end Pointer to the end of the range.
   */
  template <typename T>
  BitStream(T* begin, T* end)
      : BitStream(reinterpret_cast<uint8_t*>(begin),
                  std::distance(reinterpret_cast<uint8_t*>(begin),
                                reinterpret_cast<uint8_t*>(end))) {}

  /**
   * @brief Reads a specified number of bits from the current position without
   * removing them.
   * @param bitwidth Number of bits to read.
   * @return The bits read as an unsigned 64-bit integer.
   * @throw std::invalid_argument if bitwidth is outside the range [0, 64].
   */
  uint64_t Readbits(const int& bitwidth);

  /**
   * @brief Pops (reads and removes) a specified number of bits from the current
   * position.
   * @param bitwidth Number of bits to pop.
   * @return The bits popped as an unsigned 64-bit integer.
   * @throw std::invalid_argument if bitwidth is outside the range [0, 64].
   */
  uint64_t Popbits(const int& bitwidth);

 private:
  uint8_t* data_;   /**< Pointer to the byte array. */
  size_t length_;   /**< Length of the bit stream in bits. */
  uint64_t number_; /**< Current working number holding up to 64 bits. */
  size_t tail_pos_; /**< Current position in the bit stream. */
};

#endif
