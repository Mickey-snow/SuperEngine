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

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>

/**
 * @brief A class to interpret a raw byte array as a read-only stream.
 *
 * This class provides a read-only view of a byte array. It does not perform
 * memory management or attempt to modify the array.
 */
class ByteReader {
 public:
  /**
   * @brief Constructs a ByteReader from a beginning and ending pointer.
   *
   * @param begin Pointer to the beginning of the byte array.
   * @param end Pointer to the end of the byte array.
   * @throws std::invalid_argument if `end` is before `begin`.
   */
  ByteReader(const char* begin, const char* end);

  /**
   * @brief Constructs a ByteReader from a beginning pointer and length.
   *
   * @param begin Pointer to the beginning of the byte array.
   * @param N Length of the byte array.
   * @throws std::invalid_argument if `N` is greater than zero and `begin` is a
   * null pointer.
   */
  ByteReader(const char* begin, size_t N);

  /**
   * @brief Constructs a ByteReader from a std::string_view.
   *
   * @param sv A std::string_view containing the byte data to interpret.
   */
  ByteReader(std::string_view sv);

  /**
   * @brief Reads a specified number of bytes without advancing the current
   * pointer.
   *
   * @param count Number of bytes to read (must be between 0 and 8).
   * @return The read bytes as a 64-bit unsigned integer.
   * @throws std::invalid_argument if `count` is not between 0 and 8.
   * @throws std::out_of_range if there are not enough bytes left in the array.
   */
  uint64_t ReadBytes(int count);

  /**
   * @brief Reads bytes as a specified arithmetic type without advancing the
   * current pointer.
   *
   * @tparam T The arithmetic type to read as.
   * @param count Number of bytes to read (must be between 0 and sizeof(T)).
   * @return The interpreted value as type T.
   * @throws std::invalid_argument if `count` is out of range for type T.
   * @throws std::logic_error if type T is unsupported.
   */
  template <typename T>
  T ReadAs(int count) {
    if constexpr (std::is_arithmetic_v<T>) {  // numerical type
      if (count < 0 || count > sizeof(T)) {
        throw std::invalid_argument(
            "Byte count is out of range for the specified type T.");
      }

      uint64_t value = ReadBytes(count);
      if constexpr (std::is_integral_v<T>) {
        return static_cast<T>(value);
      } else if constexpr (std::is_floating_point_v<T>) {
        T ret;
        std::memcpy(&ret, &value, sizeof(T));
        return ret;
      } else {
        static_assert(false, "Unsupported arithmetic type provided to ReadAs.");
      }
    } else {  // string type
      if (count < 0 || current_ + count > end_)
        throw std::out_of_range(
            "Attempt to read beyond the end of the byte array.");

      if constexpr (std::is_same_v<T, std::string_view>)
        return std::string_view(current_, count);
      else if constexpr (std::is_same_v<T, std::string>)
        return std::string(current_, count);
      else
        static_assert(
            false, "Template type T must be a numerical type or string type.");
    }
  }

  /**
   * @brief Reads a specified number of bytes and advances the current pointer.
   *
   * @param count Number of bytes to read (must be between 0 and 8).
   * @return The read bytes as a 64-bit unsigned integer.
   * @throws std::invalid_argument if `count` is not between 0 and 8.
   * @throws std::out_of_range if there are not enough bytes left in the array.
   */
  uint64_t PopBytes(int count);

  /**
   * @brief Reads bytes as a specified arithmetic type and advances the current
   * pointer.
   *
   * @tparam T The arithmetic type to read as.
   * @param count Number of bytes to read (must be between 0 and sizeof(T)).
   * @return The interpreted value as type T.
   * @throws std::invalid_argument if `count` is out of range for type T.
   * @throws std::logic_error if type T is unsupported.
   */
  template <typename T>
  T PopAs(int count) {
    T ret = ReadAs<T>(count);
    Proceed(count);
    return ret;
  }

  /**
   * @brief Overloaded extraction operator for reading a value of type T.
   *
   * @tparam T The arithmetic type to read as.
   * @param other Reference to the variable where the read value will be stored.
   * @return Reference to the current ByteReader object.
   */
  template <typename T>
  ByteReader& operator>>(T& other) {
    other = PopAs<T>(sizeof(T));
    return *this;
  }

  /**
   * @brief Advances the current pointer by a specified number of bytes.
   *
   * @param count Number of bytes to advance.
   * @throws std::out_of_range if advancing moves the pointer out of the array
   * bounds.
   */
  void Proceed(int count);

  /**
   * @brief Gets the total size of the byte array.
   *
   * @return The size of the byte array in bytes.
   */
  size_t Size() const noexcept;

  /**
   * @brief Gets the current position in the byte array.
   *
   * @return The position in the byte array relative to the beginning.
   */
  size_t Position() const noexcept;

  /**
   * @brief Moves the current pointer to a specified location in the byte
   * array.
   *
   * @param loc The new position for the current pointer (must be within the
   * array bounds).
   * @throws std::out_of_range if `loc` is outside the array bounds.
   */
  void Seek(int loc);

  /**
   * @brief Gets the current pointer.
   *
   * @return The pointer to the current position in the byte array.
   */
  const char* Ptr() const { return current_; }

 private:
  const char* begin_; /**< Pointer to the beginning of the byte array. */
  const char*
      current_;     /**< Pointer to the current position in the byte array. */
  const char* end_; /**< Pointer to the end of the byte array. */
};
