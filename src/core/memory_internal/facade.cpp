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

#include "core/memory_internal/facade.hpp"

#include "core/memory_internal/proxy.hpp"

#include <algorithm>
#include <format>
#include <limits>
#include <stdexcept>
#include <utility>

namespace {

std::size_t CheckIndex(int idx) {
  if (idx < 0)
    throw std::out_of_range("negative memory bank index: " +
                            std::to_string(idx));
  return static_cast<std::size_t>(idx);
}

std::size_t CheckSize(int size) {
  if (size < 0)
    throw std::invalid_argument("negative memory bank size: " +
                                std::to_string(size));
  return static_cast<std::size_t>(size);
}

int CheckedIntSize(std::size_t size) {
  if (size > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    throw std::overflow_error("memory bank size exceeds script integer range");
  return static_cast<int>(size);
}

std::size_t CheckedEnd(std::size_t begin,
                       std::size_t count,
                       std::string_view where) {
  if (count > std::numeric_limits<std::size_t>::max() - begin)
    throw std::out_of_range(std::format("{} index overflow", where));
  return begin + count;
}

std::pair<std::size_t, std::size_t> CheckFillRange(int begin,
                                                   int end,
                                                   std::size_t size,
                                                   std::string_view where) {
  const std::size_t begin_index = CheckIndex(begin);
  const std::size_t end_index = CheckIndex(end);
  if (begin_index > end_index)
    throw std::invalid_argument(std::format("{} has invalid fill range", where));
  if (end_index > size) {
    throw std::out_of_range(
        std::format("{} fill end {} out of range for size {}", where, end_index,
                    size));
  }
  return {begin_index, end_index};
}

}  // namespace

IntListFacade::IntListFacade(getter_t getter, int size)
    : getter_(std::move(getter)), default_size_(CheckSize(size)) {
  if (!getter_)
    throw std::invalid_argument("IntListFacade: getter is empty.");
  if (default_size_ > 0)
    storage().resize(default_size_);
}

std::vector<int>& IntListFacade::storage() const { return getter_(); }

int IntListFacade::get(int idx) {
  auto& data = storage();
  return IntListProxy(data).Get(CheckIndex(idx));
}

void IntListFacade::set(int idx, int value) {
  auto& data = storage();
  IntListProxy(data).Set(CheckIndex(idx), value);
}

void IntListFacade::Set(int idx, std::vector<int> values) {
  const std::size_t begin = CheckIndex(idx);
  if (values.empty())
    return;

  auto& data = storage();
  const std::size_t end = CheckedEnd(begin, values.size(), "integer list Set");
  if (end > data.size()) {
    throw std::out_of_range(std::format(
        "integer list Set range [{}, {}) out of range for size {}", begin, end,
        data.size()));
  }

  std::move(values.begin(), values.end(), data.begin() + begin);
}

void IntListFacade::resize(int size) { storage().resize(CheckSize(size)); }

int IntListFacade::size() const { return CheckedIntSize(storage().size()); }

void IntListFacade::fill(int begin, int end, int value) {
  auto& data = storage();
  const auto [begin_index, end_index] =
      CheckFillRange(begin, end, data.size(), "integer list");
  std::fill(data.begin() + begin_index, data.begin() + end_index, value);
}

void IntListFacade::init() { storage().assign(default_size_, 0); }

int IntListFacade::b1(int idx) {
  auto& data = storage();
  return IntListProxy(data).Get(CheckIndex(idx), 1);
}

void IntListFacade::write_b1(int idx, int value) {
  auto& data = storage();
  IntListProxy(data).Set(CheckIndex(idx), value, 1);
}

int IntListFacade::b2(int idx) {
  auto& data = storage();
  return IntListProxy(data).Get(CheckIndex(idx), 2);
}

void IntListFacade::write_b2(int idx, int value) {
  auto& data = storage();
  IntListProxy(data).Set(CheckIndex(idx), value, 2);
}

int IntListFacade::b4(int idx) {
  auto& data = storage();
  return IntListProxy(data).Get(CheckIndex(idx), 4);
}

void IntListFacade::write_b4(int idx, int value) {
  auto& data = storage();
  IntListProxy(data).Set(CheckIndex(idx), value, 4);
}

int IntListFacade::b8(int idx) {
  auto& data = storage();
  return IntListProxy(data).Get(CheckIndex(idx), 8);
}

void IntListFacade::write_b8(int idx, int value) {
  auto& data = storage();
  IntListProxy(data).Set(CheckIndex(idx), value, 8);
}

int IntListFacade::b16(int idx) {
  auto& data = storage();
  return IntListProxy(data).Get(CheckIndex(idx), 16);
}

void IntListFacade::write_b16(int idx, int value) {
  auto& data = storage();
  IntListProxy(data).Set(CheckIndex(idx), value, 16);
}

StrListFacade::StrListFacade(getter_t getter, int size)
    : getter_(std::move(getter)), default_size_(CheckSize(size)) {
  if (!getter_)
    throw std::invalid_argument("StrListFacade: getter is empty.");
  if (default_size_ > 0)
    storage().resize(default_size_);
}

std::vector<std::string>& StrListFacade::storage() const { return getter_(); }

std::string StrListFacade::get(int idx) {
  auto& data = storage();
  return StrListProxy(data).Get(CheckIndex(idx));
}

void StrListFacade::set(int idx, std::string value) {
  auto& data = storage();
  StrListProxy(data).Set(CheckIndex(idx), value);
}

void StrListFacade::resize(int size) { storage().resize(CheckSize(size)); }

int StrListFacade::size() const { return CheckedIntSize(storage().size()); }

void StrListFacade::fill(int begin, int end, std::string value) {
  auto& data = storage();
  const auto [begin_index, end_index] =
      CheckFillRange(begin, end, data.size(), "string list");
  std::fill(data.begin() + begin_index, data.begin() + end_index, value);
}

void StrListFacade::init() { storage().assign(default_size_, ""); }
