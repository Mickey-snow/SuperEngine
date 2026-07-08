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

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>

class IntBankStorage {
  static void CheckBitWidth(std::uint8_t bits) {
    if (!(bits == 1 || bits == 2 || bits == 4 || bits == 8 || bits == 16 ||
          bits == 32)) {
      throw std::invalid_argument("IntBankStorage: access type " +
                                  std::to_string(bits) + "b not supported.");
    }
  }
  static std::uint32_t BitMask(std::uint8_t bits) {
    return (std::uint32_t{1} << bits) - 1;
  }

 public:
  IntBankStorage() = default;
  explicit IntBankStorage(std::size_t size) : data_(size) {}

  int Get(std::size_t index, std::uint8_t bits = 32) {
    CheckBitWidth(bits);

    if (bits == 32) {
      EnsureSizeForIndex(index);
      return data_[index];
    }

    const auto subwords_per_int = 32 / bits;
    const std::size_t index32 = index / subwords_per_int;
    EnsureSizeForIndex(index32);

    const auto val32 = static_cast<std::uint32_t>(data_[index32]);
    const auto shiftbits = (index % subwords_per_int) * bits;
    return static_cast<int>((val32 >> shiftbits) & BitMask(bits));
  }

  void Set(std::size_t index, int value, std::uint8_t bits = 32) {
    CheckBitWidth(bits);

    if (bits == 32) {
      EnsureSizeForIndex(index);
      data_[index] = value;
      return;
    }

    const auto mask = BitMask(bits);
    if (value < 0 || static_cast<std::uint32_t>(value) > mask) {
      throw std::overflow_error(
          "IntBankStorage: value " + std::to_string(value) +
          " overflow when casting to " + std::to_string(bits) + " bit int.");
    }

    const auto subwords_per_int = 32 / bits;
    const auto index32 = index / subwords_per_int;
    EnsureSizeForIndex(index32);

    auto val32 = static_cast<std::uint32_t>(data_[index32]);
    const auto shiftbits = (index % subwords_per_int) * bits;
    const auto shifted_mask = mask << shiftbits;
    val32 &= ~shifted_mask;
    val32 |= static_cast<std::uint32_t>(value) << shiftbits;
    data_[index32] = static_cast<int>(val32);
  }

  void Resize(std::size_t size) { data_.resize(size); }

  std::size_t GetSize() const { return data_.size(); }

  void Fill(std::size_t begin, std::size_t end, const int& value) {
    if (begin > end) {
      throw std::invalid_argument("IntBankStorage: invalid fill range [" +
                                  std::to_string(begin) + ',' +
                                  std::to_string(end) + ").");
    }
    if (begin == end)
      return;
    EnsureSize(end);

    std::fill(data_.begin() + begin, data_.begin() + end, value);
  }

  inline std::vector<int>& Data() { return data_; }

 private:
  void EnsureSizeForIndex(std::size_t index) {
    if (index == std::numeric_limits<std::size_t>::max()) {
      throw std::out_of_range("IntBankStorage: index " + std::to_string(index) +
                              " is too large.");
    }
    EnsureSize(index + 1);
  }

  void EnsureSize(std::size_t size) {
    if (size > data_.max_size()) {
      throw std::out_of_range("IntBankStorage: requested size " +
                              std::to_string(size) + " exceeds max size.");
    }
    if (size > data_.size())
      data_.resize(size);
  }

  std::vector<int> data_;

  friend class boost::serialization::access;
  BOOST_SERIALIZATION_SPLIT_MEMBER();

  template <class Archive>
  void save(Archive& ar, unsigned int version) const {
    const std::size_t size = data_.size();
    ar & size;
    for (const int& it : data_)
      ar & it;
  }

  template <class Archive>
  void load(Archive& ar, unsigned int version) {
    std::size_t size;
    ar & size;
    data_.assign(size, 0);
    for (int& it : data_)
      ar & it;
  }
};

class StrBankStorage {
 public:
  StrBankStorage() = default;
  explicit StrBankStorage(std::size_t size) : data_(size) {}

  std::string Get(std::size_t index) {
    EnsureSizeForIndex(index);
    return data_[index];
  }

  void Set(std::size_t index, const std::string& value) {
    EnsureSizeForIndex(index);
    data_[index] = value;
  }

  void Resize(std::size_t size) { data_.resize(size); }

  std::size_t GetSize() const { return data_.size(); }

  void Fill(std::size_t begin, std::size_t end, const std::string& value) {
    if (begin > end) {
      throw std::invalid_argument("StrBankStorage: invalid fill range [" +
                                  std::to_string(begin) + ',' +
                                  std::to_string(end) + ").");
    }
    if (begin == end)
      return;
    EnsureSize(end);

    std::fill(data_.begin() + begin, data_.begin() + end, value);
  }

  inline std::vector<std::string>& Data() { return data_; }

 private:
  void EnsureSizeForIndex(std::size_t index) {
    if (index == std::numeric_limits<std::size_t>::max()) {
      throw std::out_of_range("StrBankStorage: index " + std::to_string(index) +
                              " is too large.");
    }
    EnsureSize(index + 1);
  }

  void EnsureSize(std::size_t size) {
    if (size > data_.max_size()) {
      throw std::out_of_range("StrBankStorage: requested size " +
                              std::to_string(size) + " exceeds max size.");
    }
    if (size > data_.size())
      data_.resize(size);
  }

  std::vector<std::string> data_;

  friend class boost::serialization::access;
  BOOST_SERIALIZATION_SPLIT_MEMBER();

  template <class Archive>
  void save(Archive& ar, unsigned int version) const {
    const std::size_t size = data_.size();
    ar & size;
    for (const std::string& it : data_)
      ar & it;
  }

  template <class Archive>
  void load(Archive& ar, unsigned int version) {
    std::size_t size;
    ar & size;
    data_.assign(size, {});
    for (std::string& it : data_)
      ar & it;
  }
};
