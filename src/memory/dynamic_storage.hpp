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

#include "memory/storage_policy.hpp"

#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>

template <typename T>
class DynamicStorage : public StoragePolicy<T> {
 public:
  DynamicStorage() : root_(nullptr), size_(0) {}

  T Get(size_t index) const override {
    if (index >= size_)
      throw std::out_of_range("DynamicStorage: invalid access to index " +
                              std::to_string(index));
    return Get_impl(index, root_);
  }
  void Set(size_t index, T const& value) override {
    if (index >= size_)
      throw std::out_of_range("DynamicStorage: invalid write to index " +
                              std::to_string(index));
    Set_impl(index, index, value, root_);
  };

  void Resize(size_t size) override {
    size_ = size;
    const auto current_size = root_ == nullptr ? 0 : root_->to + 1;

    if (current_size < size) {
      size_t newsize = current_size;
      if (newsize == 0)
        newsize = 32;
      static_assert(std::numeric_limits<size_t>::radix == 2);
      for (int i = 0; newsize < size && i < std::numeric_limits<size_t>::digits;
           ++i) {
        newsize <<= 1;
      }
      if (newsize < size) {
        // special case: expanding the size by exponent of 2 causing overflow
        newsize = std::numeric_limits<size_t>::max();
      }

      Rebuild(newsize);
    } else {
      size_t newsize = current_size;
      while (newsize / 4 >= size) {
        newsize >>= 1;
        if (newsize <= 32)
          break;
      }
      if (newsize < current_size / 4)
        Rebuild(newsize);
    }
  };
  size_t GetSize() const override { return size_; }

  void Fill(size_t begin, size_t end, T const& value) override {
    // the implementation code uses ranges [begin,end] everywhere.
    Set_impl(begin, end - 1, value, root_);
  }

  std::shared_ptr<StoragePolicy<T>> Clone() const override {
    return std::make_shared<DynamicStorage<T>>(*this);
  }

  using SerializedStorage = typename StoragePolicy<T>::Serialized;
  SerializedStorage Save() const override {
    std::vector<std::tuple<size_t, size_t, T>> serialized;
    Apply(root_, [&serialized](size_t fr, size_t to, T value) {
      serialized.emplace_back(std::make_tuple(fr, to + 1, std::move(value)));
    });
    return SerializedStorage{.size = size_, .data = std::move(serialized)};
  }

  void Load(SerializedStorage serialized) override {
    Resize(serialized.size);
    Fill(0, serialized.size, T{});
    for (const auto& [fr, to, val] : serialized.data) {
      Fill(fr, to, val);
    }
  }

 private:
  struct Node {
    size_t fr, to;
    std::optional<T> tag;
    std::shared_ptr<Node> lch, rch;

    Node(size_t ifr, size_t ito)
        : fr(ifr), to(ito), tag(T{}), lch(nullptr), rch(nullptr) {}

    void Pushdown() {
      if (fr == to)
        throw std::runtime_error("Node: pushdown() called at leaf node " +
                                 std::to_string(fr));
      const size_t mid = Midpoint();
      if (lch == nullptr)
        lch = std::make_shared<Node>(fr, mid);
      if (rch == nullptr)
        rch = std::make_shared<Node>(mid + 1, to);

      if (tag.has_value()) {
        lch->tag = tag;
        rch->tag = tag;
        tag.reset();
      }
    }

    size_t Midpoint() const { return fr + ((to - fr) >> 1); }
  };

  T const& Get_impl(size_t index, std::shared_ptr<Node> const& nowAt) const {
    if (nowAt == nullptr) {
      throw std::runtime_error(
          "DynamicStorage::Get_impl: Unknown error nowAt is nullptr");
    }

    if (nowAt->tag.has_value())
      return nowAt->tag.value();
    if (index <= nowAt->Midpoint())
      return Get_impl(index, nowAt->lch);
    else
      return Get_impl(index, nowAt->rch);
  }

  void Set_impl(size_t ibegin,
                size_t iend,
                T const& value,
                std::shared_ptr<Node>& nowAt) {
    if (nowAt == nullptr)
      return;
    if (nowAt->to < ibegin || iend < nowAt->fr)
      return;

    if (!nowAt.unique())
      nowAt = std::make_shared<Node>(*nowAt);

    if (ibegin <= nowAt->fr && nowAt->to <= iend) {
      nowAt->tag = value;
    } else {
      nowAt->Pushdown();
      Set_impl(ibegin, iend, value, nowAt->lch);
      Set_impl(ibegin, iend, value, nowAt->rch);
    }
  }

  void Rebuild(const size_t size) {
    auto oldroot = root_;
    root_ = std::make_shared<Node>(0, size - 1);
    Apply(oldroot, [&](size_t fr, size_t to, const T& value) {
      this->Set_impl(fr, to, value, root_);
    });
  }

  template <class U>
  static void Apply(std::shared_ptr<const Node> nowAt, const U& fn) {
    if (nowAt == nullptr)
      return;
    if (nowAt->tag.has_value()) {
      std::invoke(fn, nowAt->fr, nowAt->to, nowAt->tag.value());
      return;
    }
    Apply(nowAt->lch, fn);
    Apply(nowAt->rch, fn);
  }

  std::shared_ptr<Node> root_;
  size_t size_;
};
