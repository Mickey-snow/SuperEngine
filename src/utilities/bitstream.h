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
#include <cstring>
#include <iterator>
#include <type_traits>

/**
 * @class BitStream
 * @brief A class to interpret and manipulate a byte array as a stream of bits.
 *
 * This class provides functionality to read bits within a byte array.
 * It acts as a view to bit data, meaning it does not manage the memory of the
 * underlying data array. It is the caller's responsibility to ensure the byte
 * array remains valid for the lifetime of the BitStream object.
 */
class BitStream {
 public:
  /**
   * @brief Constructor that initializes the BitStream with a byte array.
   * @param data Pointer to the byte array.
   * @param length Length of the byte array in bytes.
   */
  BitStream(const uint8_t* data, size_t length);

  /**
   * @brief Templated constructor that initializes the BitStream with an array
   * of elements.
   * @tparam T Type of the elements in the array.
   * @param data Pointer to the array of elements.
   * @param length Number of elements in the array.
   */
  template <typename T>
  BitStream(const T* data, size_t length) : BitStream(data, data + length) {}

  /**
   * @brief Templated constructor that initializes the BitStream with a range of
   * elements.
   * @tparam T Type of the elements in the range.
   * @param begin Pointer to the beginning of the range.
   * @param end Pointer to the end of the range.
   */
  template <typename T>
  BitStream(const T* begin, const T* end)
      : BitStream(reinterpret_cast<data_t>(begin),
                  std::distance(reinterpret_cast<data_t>(begin),
                                reinterpret_cast<data_t>(end))) {}

  /**
   * @brief Reads a specified number of bits from the current position without
   * removing them.
   * @param bitwidth Number of bits to read.
   * @return The bits read as an unsigned 64-bit integer.
   * @throw std::invalid_argument if bitwidth is outside the range [0, 64].
   */
  uint64_t Readbits(const int& bitwidth);

  /**
   * @brief Reads a specified number of bits from the current position and casts
   * them to the specified numerical type.
   * @tparam T The numerical type to cast the read bits to. Must be an integral
   * or floating-point type.
   * @param bitwidth Number of bits to read and cast.
   * @return The bits read, cast to the specified type `T`.
   * @throw std::invalid_argument if `bitwidth` is outside the range [0, 8 *
   * sizeof(T)].
   * @throw std::logic_error if the type `T` is unsupported (non-numerical).
   */
  template <typename T>
  T ReadAs(const int& bitwidth) {
    static_assert(std::is_arithmetic_v<T>,
                  "Template type T must be a numerical type.");

    if (bitwidth < 0 || bitwidth > 8 * sizeof(T)) {
      throw std::invalid_argument(
          "Bit width out of range for specified type T.");
    }

    uint64_t bits = Readbits(bitwidth);
    if constexpr (std::is_integral_v<T>) {
      return static_cast<T>(bits);
    } else if constexpr (std::is_floating_point_v<T>) {
      T value;
      std::memcpy(&value, &bits, sizeof(T));
      return value;
    } else {
      throw std::logic_error("Unsupported type.");
    }
  }

  /**
   * @brief Pops (reads and removes) a specified number of bits from the current
   * position.
   * @param bitwidth Number of bits to pop.
   * @return The bits popped as an unsigned 64-bit integer.
   * @throw std::invalid_argument if bitwidth is outside the range [0, 64].
   */
  uint64_t Popbits(const int& bitwidth);

  /**
   * @brief Pops (reads and removes) a specified number of bits from the current
   * position and casts them to the specified numerical type.
   * @tparam T The numerical type to cast the popped bits to. Must be an
   * integral or floating-point type.
   * @param bitwidth Number of bits to pop and cast.
   * @return The bits popped, cast to the specified type `T`.
   * @throw std::invalid_argument if `bitwidth` is outside the range [0, 8 *
   * sizeof(T)].
   * @throw std::logic_error if the type `T` is unsupported (non-numerical).
   */
  template <typename T>
  T PopAs(const int& bitwidth) {
    T ret = ReadAs<T>(bitwidth);
    Proceed(bitwidth);
    return ret;
  }

  /**
   * @brief Returns the current position in the bit stream.
   * @return The current position in the bit stream, in number of bits.
   */
  size_t Position(void) const;

  /**
   * @brief Returns the size of the bit stream in bits.
   * @return The size of the bit stream in bits.
   */
  size_t Size(void) const;

  /**
   * @brief Returns the length of the bit stream in bits.
   * @return The length of the bit stream in bits.
   */
  size_t Length(void) const;

 private:
  /**
   * @brief Advances the current position in the bit stream by a specified
   * number of bits.
   * @param bitcount Number of bits to advance the current position.
   */
  void Proceed(const int& bitcount);

  using data_t = uint8_t const* const;
  data_t data_;     /**< Pointer to the byte array. */
  size_t length_;   /**< Length of the bit stream in bits. */
  uint64_t number_; /**< Current working number holding up to 64 bits. */
  size_t pos_;      /**< Current position in the bit stream. */
};

#endif
