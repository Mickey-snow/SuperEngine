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
#include "utilities/assertx.hpp"
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

std::size_t CheckedEnd(std::size_t begin,
                       std::size_t count,
                       std::string_view where) {
  if (count > std::numeric_limits<std::size_t>::max() - begin)
    throw RuntimeError(std::format("{} index overflow", where));
  return begin + count;
}

std::size_t CheckExistingIndex(int idx,
                               std::size_t size,
                               std::string_view where) {
  const std::size_t index = CheckIndex(idx);
  if (index >= size) {
    throw RuntimeError(std::format("{} index {} out of range for size {}",
                                   where, index, size));
  }
  return index;
}

std::size_t CheckBitIndex(int idx,
                          std::size_t words,
                          std::uint8_t bits,
                          std::string_view where) {
  const std::size_t index = CheckIndex(idx);
  const std::size_t per_word = 32 / bits;
  if (words > std::numeric_limits<std::size_t>::max() / per_word)
    throw RuntimeError(std::format("{} size overflow", where));
  const std::size_t logical_size = words * per_word;
  if (index >= logical_size) {
    throw RuntimeError(std::format("{} index {} out of range for size {}",
                                   where, index, logical_size));
  }
  return index;
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

SiglusIntBank::SiglusIntBank(Memory& memory, IntBank bank, uint8_t bits)
    : memory_(&memory), bank_(bank), bits_(bits) {
  ASSERTX_TRUE(bits == 32 || bits == 1 || bits == 2 || bits == 4 || bits == 8 ||
               bits == 16);
}

int SiglusIntBank::get(int idx) {
  const std::size_t index = CheckIndex(idx);
  EnsureSize(index + 1);
  return memory_->Read(IntMemoryLocation(bank_, index, bits_));
}

void SiglusIntBank::set(int idx, int value) {
  const std::size_t index = CheckIndex(idx);
  EnsureSize(index + 1);
  memory_->Write(IntMemoryLocation(bank_, index, bits_), value);
}

void SiglusIntBank::Set(int idx, std::vector<Value> values) {
  const std::size_t begin = CheckIndex(idx);
  if (values.empty())
    return;

  EnsureSize(CheckedEnd(begin, values.size(), "integer bank Set"));
  for (std::size_t i = 0; i < values.size(); ++i) {
    memory_->Write(IntMemoryLocation(bank_, begin + i, bits_),
                   RequireInt(values[i], "integer bank Set"));
  }
}

int SiglusIntBank::b1(int idx) { return get_bits(idx, 1); }
void SiglusIntBank::write_b1(int idx, int value) { set_bits(idx, value, 1); }

int SiglusIntBank::b2(int idx) { return get_bits(idx, 2); }
void SiglusIntBank::write_b2(int idx, int value) { set_bits(idx, value, 2); }

int SiglusIntBank::b4(int idx) { return get_bits(idx, 4); }
void SiglusIntBank::write_b4(int idx, int value) { set_bits(idx, value, 4); }

int SiglusIntBank::b8(int idx) { return get_bits(idx, 8); }
void SiglusIntBank::write_b8(int idx, int value) { set_bits(idx, value, 8); }

int SiglusIntBank::b16(int idx) { return get_bits(idx, 16); }
void SiglusIntBank::write_b16(int idx, int value) { set_bits(idx, value, 16); }

void SiglusIntBank::resize(int size) {
  memory_->Resize(bank_, RequiredIntWords(CheckSize(size), bits_));
}

int SiglusIntBank::size() const {
  const std::size_t words = memory_->Size(bank_);
  if (bits_ == 32)
    return CheckedIntSize(words);
  return CheckedIntSize(words * (32 / bits_));
}

void SiglusIntBank::fill(int begin, int end, int value) {
  const std::size_t begin_index = CheckIndex(begin);
  const std::size_t end_index = CheckIndex(end);
  if (begin_index > end_index)
    throw RuntimeError("invalid memory fill range");
  if (begin_index == end_index)
    return;

  EnsureSize(end_index);
  if (bits_ == 32) {
    memory_->Fill(bank_, begin_index, end_index, value);
    return;
  }

  for (std::size_t i = begin_index; i < end_index; ++i)
    memory_->Write(IntMemoryLocation(bank_, i, bits_), value);
}

void SiglusIntBank::init(int value) { fill(0, size(), value); }

int SiglusIntBank::get_bits(int idx, std::uint8_t bits) {
  const std::size_t index = CheckIndex(idx);
  EnsureSize(index + 1, bits);
  return memory_->Read(IntMemoryLocation(bank_, index, bits));
}

void SiglusIntBank::set_bits(int idx, int value, std::uint8_t bits) {
  const std::size_t index = CheckIndex(idx);
  EnsureSize(index + 1, bits);
  memory_->Write(IntMemoryLocation(bank_, index, bits), value);
}

void SiglusIntBank::EnsureSize(std::size_t logical_size) {
  EnsureSize(logical_size, bits_);
}

void SiglusIntBank::EnsureSize(std::size_t logical_size, std::uint8_t bits) {
  const std::size_t required = RequiredIntWords(logical_size, bits);
  if (memory_->Size(bank_) < required)
    memory_->Resize(bank_, required);
}

SiglusStrBank::SiglusStrBank(Memory& memory, StrBank bank)
    : memory_(&memory), bank_(bank) {}

std::string SiglusStrBank::get(int idx) {
  const std::size_t index = CheckIndex(idx);
  EnsureSize(index + 1);
  return memory_->Read(bank_, index);
}

void SiglusStrBank::set(int idx, std::string value) {
  const std::size_t index = CheckIndex(idx);
  EnsureSize(index + 1);
  memory_->Write(bank_, index, value);
}

void SiglusStrBank::Set(int idx, std::string value) {
  set(idx, std::move(value));
}

void SiglusStrBank::resize(int size) {
  memory_->Resize(bank_, CheckSize(size));
}

int SiglusStrBank::size() const { return CheckedIntSize(memory_->Size(bank_)); }

void SiglusStrBank::fill(int begin, int end, std::string value) {
  const std::size_t begin_index = CheckIndex(begin);
  const std::size_t end_index = CheckIndex(end);
  if (begin_index > end_index)
    throw RuntimeError("invalid memory fill range");
  if (begin_index == end_index)
    return;

  EnsureSize(end_index);
  memory_->Fill(bank_, begin_index, end_index, value);
}

void SiglusStrBank::init(std::string value) {
  fill(0, size(), std::move(value));
}

void SiglusStrBank::EnsureSize(std::size_t size) {
  if (memory_->Size(bank_) < size)
    memory_->Resize(bank_, size);
}

SiglusIntList::SiglusIntList(int size)
    : storage_(CheckSize(size)), default_size_(CheckSize(size)) {}

int SiglusIntList::get(int idx) {
  const std::size_t index =
      CheckExistingIndex(idx, storage_.GetSize(), "integer list");
  return storage_.Get(index);
}

void SiglusIntList::set(int idx, int value) {
  const std::size_t index =
      CheckExistingIndex(idx, storage_.GetSize(), "integer list");
  storage_.Set(index, value);
}

void SiglusIntList::Set(int idx, std::vector<Value> values) {
  const std::size_t begin = CheckIndex(idx);
  if (values.empty())
    return;

  const std::size_t end = CheckedEnd(begin, values.size(), "integer list Set");
  if (end > storage_.GetSize()) {
    throw RuntimeError(
        std::format("integer list Set range [{}, {}) out of range for size {}",
                    begin, end, storage_.GetSize()));
  }

  for (std::size_t i = 0; i < values.size(); ++i)
    storage_.Set(begin + i, RequireInt(values[i], "integer list Set"));
}

void SiglusIntList::resize(int size) { storage_.Resize(CheckSize(size)); }

int SiglusIntList::size() const { return CheckedIntSize(storage_.GetSize()); }

void SiglusIntList::fill(int begin, int end, int value) {
  const auto [begin_index, end_index] =
      CheckFillRange(begin, end, storage_.GetSize(), "integer list");
  storage_.Fill(begin_index, end_index, value);
}

void SiglusIntList::init() {
  storage_.Resize(default_size_);
  storage_.Fill(0, default_size_, 0);
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
  const std::size_t index =
      CheckBitIndex(idx, storage_.GetSize(), bits, "integer list bit access");
  return storage_.Get(index, bits);
}

void SiglusIntList::set_bits(int idx, int value, std::uint8_t bits) {
  const std::size_t index =
      CheckBitIndex(idx, storage_.GetSize(), bits, "integer list bit access");
  storage_.Set(index, value, bits);
}

SiglusStrList::SiglusStrList(int size)
    : storage_(CheckSize(size)), default_size_(CheckSize(size)) {}

std::string SiglusStrList::get(int idx) {
  const std::size_t index =
      CheckExistingIndex(idx, storage_.GetSize(), "string list");
  return storage_.Get(index);
}

void SiglusStrList::set(int idx, std::string value) {
  const std::size_t index =
      CheckExistingIndex(idx, storage_.GetSize(), "string list");
  storage_.Set(index, value);
}

void SiglusStrList::resize(int size) { storage_.Resize(CheckSize(size)); }

int SiglusStrList::size() const { return CheckedIntSize(storage_.GetSize()); }

void SiglusStrList::fill(int begin, int end, std::string value) {
  const auto [begin_index, end_index] =
      CheckFillRange(begin, end, storage_.GetSize(), "string list");
  storage_.Fill(begin_index, end_index, value);
}

void SiglusStrList::init() {
  storage_.Resize(default_size_);
  storage_.Fill(0, default_size_, "");
}

}  // namespace libsiglus::binding
