// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2025 Serina Sakurai
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
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

template <typename T>
class flat_map {
 public:
  using key_type = int;
  using mapped_type = T;

  flat_map(int min_key, int max_key)
      : min_key_(min_key),
        max_key_(max_key),
        N_(std::size_t(max_key - min_key + 1)) {
    if (min_key_ > max_key_)
      throw std::invalid_argument("flat_map: min_key > max_key");
    data_.resize(N_);
  }

  void clear() noexcept {
    for (auto& e : data_)
      e.reset();
  }

  void insert(int key, const T& value) { data_[index(key)] = value; }

  void insert(int key, T&& value) { data_[index(key)] = std::move(value); }

  template <typename... Args>
  void emplace(int key, Args&&... args) {
    data_[index(key)].emplace(std::forward<Args>(args)...);
  }

  bool contains(int key) const noexcept {
    if (key < min_key_ || key > max_key_)
      return false;
    return data_[key - min_key_].has_value();
  }

  T& at(int key) {
    auto& opt = data_[index(key)];
    if (!opt)
      throw std::out_of_range("flat_map: no value at key " +
                              std::to_string(key));
    return *opt;
  }

  const T& at(int key) const {
    auto& opt = data_[index(key)];
    if (!opt)
      throw std::out_of_range("flat_map: no value at key " +
                              std::to_string(key));
    return *opt;
  }

  std::optional<T>& operator[](int key) { return data_[index(key)]; }

  const std::optional<T>& operator[](int key) const {
    return data_[index(key)];
  }

  int min_key() const noexcept { return min_key_; }
  int max_key() const noexcept { return max_key_; }
  std::size_t capacity() const noexcept { return N_; }

 private:
  std::size_t index(int key) const {
    if (key < min_key_ || key > max_key_)
      throw std::out_of_range("flat_map: key out of range");
    return std::size_t(key - min_key_);
  }

  int min_key_, max_key_;
  std::size_t N_;
  std::vector<std::optional<T>> data_;
};

template <typename T>
flat_map<T> make_flatmap(std::initializer_list<std::pair<int, T>> init) {
  int lo = std::min_element(init.begin(), init.end(), [](auto& a, auto& b) {
             return a.first < b.first;
           })->first;
  int hi = std::max_element(init.begin(), init.end(), [](auto& a, auto& b) {
             return a.first < b.first;
           })->first;
  flat_map<T> fm(lo, hi);
  for (auto& p : init)
    fm.insert(p.first, std::move(p.second));
  return fm;
}
