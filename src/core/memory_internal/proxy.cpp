// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2026 Serina Sakurai
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

#include "core/memory_internal/proxy.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace {

void CheckBitWidth(std::uint8_t bits) {
  if (!(bits == 1 || bits == 2 || bits == 4 || bits == 8 || bits == 16 ||
        bits == 32)) {
    throw std::invalid_argument("IntListProxy: access type " +
                                std::to_string(bits) + "b not supported.");
  }
}

std::uint32_t BitMask(std::uint8_t bits) {
  if (bits == 32)
    return std::numeric_limits<std::uint32_t>::max();
  return (std::uint32_t{1} << bits) - 1;
}

}  // namespace

IntListProxy::IntListProxy(std::vector<int>& data) : data_(data) {}

int IntListProxy::Get(std::size_t index, std::uint8_t bits) {
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

void IntListProxy::Set(std::size_t index, int value, std::uint8_t bits) {
  CheckBitWidth(bits);

  if (bits == 32) {
    EnsureSizeForIndex(index);
    data_[index] = value;
    return;
  }

  const auto mask = BitMask(bits);
  if (value < 0 || static_cast<std::uint32_t>(value) > mask) {
    throw std::overflow_error(
        "IntListProxy: value " + std::to_string(value) +
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

void IntListProxy::Resize(std::size_t size) { data_.resize(size); }

std::size_t IntListProxy::GetSize() const { return data_.size(); }

void IntListProxy::Fill(std::size_t begin, std::size_t end, int value) {
  if (begin > end) {
    throw std::invalid_argument("IntListProxy: invalid fill range [" +
                                std::to_string(begin) + ',' +
                                std::to_string(end) + ").");
  }
  if (begin == end)
    return;
  EnsureSize(end);

  std::fill(data_.begin() + begin, data_.begin() + end, value);
}

void IntListProxy::EnsureSizeForIndex(std::size_t index) {
  if (index == std::numeric_limits<std::size_t>::max()) {
    throw std::out_of_range("IntListProxy: index " + std::to_string(index) +
                            " is too large.");
  }
  EnsureSize(index + 1);
}

void IntListProxy::EnsureSize(std::size_t size) {
  if (size > data_.max_size()) {
    throw std::out_of_range("IntListProxy: requested size " +
                            std::to_string(size) + " exceeds max size.");
  }
  if (size > data_.size())
    data_.resize(size);
}

StrListProxy::StrListProxy(std::vector<std::string>& data) : data_(data) {}

std::string StrListProxy::Get(std::size_t index) {
  EnsureSizeForIndex(index);
  return data_[index];
}

void StrListProxy::Set(std::size_t index, const std::string& value) {
  EnsureSizeForIndex(index);
  data_[index] = value;
}

void StrListProxy::Resize(std::size_t size) { data_.resize(size); }

std::size_t StrListProxy::GetSize() const { return data_.size(); }

void StrListProxy::Fill(std::size_t begin,
                        std::size_t end,
                        const std::string& value) {
  if (begin > end) {
    throw std::invalid_argument("StrListProxy: invalid fill range [" +
                                std::to_string(begin) + ',' +
                                std::to_string(end) + ").");
  }
  if (begin == end)
    return;
  EnsureSize(end);

  std::fill(data_.begin() + begin, data_.begin() + end, value);
}

void StrListProxy::EnsureSizeForIndex(std::size_t index) {
  if (index == std::numeric_limits<std::size_t>::max()) {
    throw std::out_of_range("StrListProxy: index " + std::to_string(index) +
                            " is too large.");
  }
  EnsureSize(index + 1);
}

void StrListProxy::EnsureSize(std::size_t size) {
  if (size > data_.max_size()) {
    throw std::out_of_range("StrListProxy: requested size " +
                            std::to_string(size) + " exceeds max size.");
  }
  if (size > data_.size())
    data_.resize(size);
}
