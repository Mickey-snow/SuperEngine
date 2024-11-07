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

#ifndef SRC_MEMORY_LOCATION_HPP_
#define SRC_MEMORY_LOCATION_HPP_

#include <cstddef>
#include <cstdint>
#include <string>
#include <variant>

namespace libreallive {
class IntMemRef;
}

/*
 * @brief Defines classes to represent memory locations in the virtual machine.
 */

enum class IntBank : uint8_t { A = 0, B, C, D, E, F, X, G, Z, H, I, J, L, CNT };

enum class StrBank : uint8_t { S = 0, M, K, local_name, global_name, CNT };

std::string ToString(IntBank bank, uint8_t bits = 32);
std::string ToString(StrBank bank);

/**
 * @class IntMemoryLocation
 * @brief Represents an integer memory location in the virtual machine.
 *
 * An integer memory location is specified by the name of the memory bank, the
 * memory index, and the bit width.
 */
class IntMemoryLocation {
 public:
  IntMemoryLocation(IntBank bank, size_t location, uint8_t bits = 32);

  /**
   * @brief Constructs an IntMemoryLocation from a `libreallive::IntMemRef`
   * object.
   * @param rlint The libreallive integer memory reference.
   *
   * This constructor is compatible with libreallive and allows for easy
   * conversion from libreallive's `IntMemRef`.
   */
  IntMemoryLocation(libreallive::IntMemRef);

  IntBank Bank() const;
  size_t Index() const;
  uint8_t Bitwidth() const;

  // For debug
  friend std::ostream& operator<<(std::ostream& os,
                                  const IntMemoryLocation& memref);
  explicit operator std::string() const;

  bool operator<(const IntMemoryLocation&) const;
  bool operator==(const IntMemoryLocation&) const;

 private:
  IntBank bank_;
  size_t location_;
  uint8_t bits_ = 32;  ///< Access type, specifies the bit width for each
                       ///< element in the array.
};

class StrMemoryLocation {
 public:
  StrMemoryLocation(StrBank bank, size_t location);

  /**
   * @brief Constructs an StrMemorylocation from libreallive enum and index
   * @param bank The libreallive representation of a string bank.
   *
   * This constructor is compatible with libreallive.
   */
  StrMemoryLocation(int bank, size_t location);

  StrBank Bank() const;
  size_t Index() const;

  // For debug
  friend std::ostream& operator<<(std::ostream& os,
                                  const StrMemoryLocation& memref);
  explicit operator std::string() const;

  bool operator<(const StrMemoryLocation&) const;
  bool operator==(const StrMemoryLocation&) const;

 private:
  StrBank bank_;
  size_t location_;
};

using MemoryLocation = std::variant<IntMemoryLocation, StrMemoryLocation>;

/**
 * @brief Parses a memory location string and returns a `MemoryLocation` object.
 * @param location_str The string representing the memory location.
 * @return A `MemoryLocation` object parsed from the string.
 *
 * Parses a string representation of a memory location and constructs the
 * corresponding `IntMemoryLocation` or `StrMemoryLocation` object.
 *
 * @note not implemented yet.
 */
MemoryLocation ParseMemoryLocation(std::string);

#endif  // SRC_MEMORY_LOCATION_HPP_
