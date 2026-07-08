// -----------------------------------------------------------------------
//
// This file is part of RLVM
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
// -----------------------------------------------------------------------

#include "libsiglus/bindings/memory_facades.hpp"

#include "libsiglus/bindings/util.hpp"
#include "vm/exception.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <format>
#include <limits>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace libsiglus::binding {

using serilang::RuntimeError;
using serilang::Value;

namespace {

std::size_t CheckIndex(int idx) {
  if (idx < 0)
    throw RuntimeError("negative memory bank index: " + std::to_string(idx));
  return static_cast<std::size_t>(idx);
}

std::size_t CheckSize(int size) {
  if (size < 0)
    throw RuntimeError("negative memory bank size: " + std::to_string(size));
  return static_cast<std::size_t>(size);
}

int CheckedIntSize(std::size_t size) {
  if (size > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    throw RuntimeError("memory bank size exceeds script integer range");
  return static_cast<int>(size);
}

std::size_t RequiredIntWords(std::size_t logical_size, uint8_t bits) {
  if (bits == 32)
    return logical_size;

  const std::size_t max_bits = std::numeric_limits<std::size_t>::max() - 31;
  if (logical_size > max_bits / bits)
    throw RuntimeError("memory bank size overflow");

  return (logical_size * bits + 31) / 32;
}

std::uint32_t BitMask(std::uint8_t bits) {
  return (std::uint32_t{1} << bits) - 1;
}

void CheckSubwordWidth(std::uint8_t bits) {
  if (!(bits == 1 || bits == 2 || bits == 4 || bits == 8 || bits == 16)) {
    throw RuntimeError("IntList: access type " + std::to_string(bits) +
                       "b not supported.");
  }
}

std::size_t CheckedEnd(std::size_t begin,
                       std::size_t count,
                       std::string_view where) {
  if (count > std::numeric_limits<std::size_t>::max() - begin)
    throw RuntimeError(std::format("{} index overflow", where));
  return begin + count;
}

std::pair<std::size_t, std::size_t> CheckFillRange(int begin,
                                                   int end,
                                                   std::size_t size,
                                                   std::string_view where) {
  const std::size_t begin_index = CheckIndex(begin);
  const std::size_t end_index = CheckIndex(end);
  if (begin_index > end_index)
    throw RuntimeError(std::format("{} has invalid fill range", where));
  if (end_index > size) {
    throw RuntimeError(std::format("{} fill end {} out of range for size {}",
                                   where, end_index, size));
  }
  return {begin_index, end_index};
}

}  // namespace

SiglusIntList::SiglusIntList(getter_t getter, int size)
    : getter_(std::move(getter)), default_size_(CheckSize(size)) {
  if (!getter_)
    throw RuntimeError("IntList: getter is empty.");
  if (default_size_ > 0)
    getter_().resize(default_size_);
}

std::size_t SiglusIntList::CheckExistingIndex(int idx) {
  if (idx < 0)
    throw RuntimeError("negative memory bank index: " + std::to_string(idx));

  auto& storage = getter_();
  const std::size_t index = idx;
  if (const std::size_t size = storage.size(); index >= size) {
    if (autoresize_) {
      storage.resize(index + 1);
    } else
      throw RuntimeError(std::format(
          "IntList: index {} out of range for size {}", index, size));
  }
  return index;
}

int SiglusIntList::get(int idx) {
  const std::size_t index = CheckExistingIndex(idx);
  return getter_()[index];
}

void SiglusIntList::set(int idx, int value) {
  const std::size_t index = CheckExistingIndex(idx);
  getter_()[index] = value;
}

void SiglusIntList::Set(int idx, std::vector<Value> values) {
  const std::size_t begin = CheckIndex(idx);
  if (values.empty())
    return;

  auto& storage = getter_();
  const std::size_t end = CheckedEnd(begin, values.size(), "integer list Set");
  if (end > storage.size()) {
    throw RuntimeError(
        std::format("integer list Set range [{}, {}) out of range for size {}",
                    begin, end, storage.size()));
  }

  for (std::size_t i = 0; i < values.size(); ++i)
    storage[begin + i] = RequireInt(values[i], "integer list Set");
}

void SiglusIntList::resize(int size) { getter_().resize(CheckSize(size)); }

int SiglusIntList::size() const { return CheckedIntSize(getter_().size()); }

void SiglusIntList::fill(int begin, int end, int value) {
  auto& storage = getter_();
  const auto [begin_index, end_index] =
      CheckFillRange(begin, end, storage.size(), "integer list");
  std::fill(storage.begin() + begin_index, storage.begin() + end_index, value);
}

void SiglusIntList::init() {
  auto& storage = getter_();
  storage.assign(default_size_, 0);
}

int SiglusIntList::b1(int idx) { return get_bits(idx, 1); }
void SiglusIntList::write_b1(int idx, int value) { set_bits(idx, value, 1); }

int SiglusIntList::b2(int idx) { return get_bits(idx, 2); }
void SiglusIntList::write_b2(int idx, int value) { set_bits(idx, value, 2); }

int SiglusIntList::b4(int idx) { return get_bits(idx, 4); }
void SiglusIntList::write_b4(int idx, int value) { set_bits(idx, value, 4); }

int SiglusIntList::b8(int idx) { return get_bits(idx, 8); }
void SiglusIntList::write_b8(int idx, int value) { set_bits(idx, value, 8); }

int SiglusIntList::b16(int idx) { return get_bits(idx, 16); }
void SiglusIntList::write_b16(int idx, int value) { set_bits(idx, value, 16); }

int SiglusIntList::get_bits(int idx, std::uint8_t bits) {
  CheckSubwordWidth(bits);
  const std::size_t index = CheckIndex(idx);
  const std::size_t logical_size =
      CheckedEnd(index, 1, "integer list bit access");

  auto& storage = getter_();
  storage.resize(
      std::max(storage.size(), RequiredIntWords(logical_size, bits)));

  const std::size_t per_word = 32 / bits;
  const std::size_t word_index = index / per_word;
  const std::uint8_t shiftbits = (index % per_word) * bits;
  const auto word = static_cast<std::uint32_t>(storage[word_index]);
  return static_cast<int>((word >> shiftbits) & BitMask(bits));
}

void SiglusIntList::set_bits(int idx, int value, std::uint8_t bits) {
  CheckSubwordWidth(bits);
  const std::size_t index = CheckIndex(idx);
  const auto mask = BitMask(bits);
  if (value < 0 || static_cast<std::uint32_t>(value) > mask) {
    throw RuntimeError("IntList: value " + std::to_string(value) +
                       " overflow when casting to " + std::to_string(bits) +
                       " bit int.");
  }
  const std::size_t logical_size =
      CheckedEnd(index, 1, "integer list bit access");

  auto& storage = getter_();
  storage.resize(
      std::max(storage.size(), RequiredIntWords(logical_size, bits)));

  const std::size_t per_word = 32 / bits;
  const std::size_t word_index = index / per_word;
  const std::uint8_t shiftbits = (index % per_word) * bits;
  const auto shifted_mask = mask << shiftbits;

  auto word = static_cast<std::uint32_t>(storage[word_index]);
  word &= ~shifted_mask;
  word |= static_cast<std::uint32_t>(value) << shiftbits;
  storage[word_index] = static_cast<int>(word);
}

SiglusStrList::SiglusStrList(getter_t getter, int size)
    : getter_(std::move(getter)), default_size_(CheckSize(size)) {
  if (!getter_)
    throw RuntimeError("StrList: getter is empty.");
  if (default_size_ > 0)
    getter_().resize(default_size_);
}

std::size_t SiglusStrList::CheckExistingIndex(int idx) {
  if (idx < 0)
    throw RuntimeError("negative memory bank index: " + std::to_string(idx));

  auto& storage = getter_();
  const std::size_t index = idx;
  if (const std::size_t size = storage.size(); index >= size) {
    if (autoresize_) {
      storage.resize(index + 1);
    } else
      throw RuntimeError(std::format(
          "StrList: index {} out of range for size {}", index, size));
  }
  return index;
}

std::string SiglusStrList::get(int idx) {
  const std::size_t index = CheckExistingIndex(idx);
  return getter_()[index];
}

void SiglusStrList::set(int idx, std::string value) {
  const std::size_t index = CheckExistingIndex(idx);
  getter_()[index] = std::move(value);
}

void SiglusStrList::resize(int size) { getter_().resize(CheckSize(size)); }

int SiglusStrList::size() const { return CheckedIntSize(getter_().size()); }

void SiglusStrList::fill(int begin, int end, std::string value) {
  auto& storage = getter_();
  const auto [begin_index, end_index] =
      CheckFillRange(begin, end, storage.size(), "string list");
  std::fill(storage.begin() + begin_index, storage.begin() + end_index, value);
}

void SiglusStrList::init() {
  auto& storage = getter_();
  storage.assign(default_size_, "");
}

}  // namespace libsiglus::binding
