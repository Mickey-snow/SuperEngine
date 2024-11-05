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

#ifndef SRC_BASE_MEMORY_BANK_HPP_
#define SRC_BASE_MEMORY_BANK_HPP_

#include <cstddef>
#include <memory>
#include <optional>
#include <stdexcept>

template <typename T>
class MemoryBank {
 public:
  MemoryBank() : root_(nullptr), size_(0) {}

  T Get(size_t index) {
    if (index >= size_)
      throw std::out_of_range("MemoryBank: invalid access to index " +
                              std::to_string(index));
    return Get_impl(index, root_);
  }
  T const& Get(size_t index) const {
    if (index >= size_)
      throw std::out_of_range("MemoryBank: invalid access to index " +
                              std::to_string(index));

    return Get_impl(index, root_);
  }
  void Set(size_t index, T const& value) {
    if (index >= size_)
      throw std::out_of_range("MemoryBank: invalid write to index " +
                              std::to_string(index));
    Set_impl(index, index, value, root_);
  };

  void Resize(size_t size) {
    size_ = size;
    size_t newsize = root_ == nullptr ? 32 : root_->to;
    while (newsize < size_)
      newsize <<= 1;

    auto oldroot = root_;
    root_ = std::make_shared<Node>(0, newsize - 1);
    RebuildFrom(oldroot);
  };
  size_t GetSize() const { return size_; }

  void Fill(size_t begin, size_t end, T const& value) {
    Set_impl(begin, end, value, root_);
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
      size_t mid = (fr + to) >> 1;
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
  };

  T Get_impl(size_t index, std::shared_ptr<Node> const& nowAt) const {
    if (nowAt == nullptr)
      return T{};

    if (nowAt->tag.has_value())
      return nowAt->tag.value();
    size_t mid = (nowAt->fr + nowAt->to) >> 1;
    if (index <= mid)
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

  void RebuildFrom(std::shared_ptr<const Node> nowAt) {
    if (nowAt == nullptr)
      return;
    if (nowAt->tag.has_value()) {
      Set_impl(nowAt->fr, nowAt->to, nowAt->tag.value(), root_);
      return;
    }
    RebuildFrom(nowAt->lch);
    RebuildFrom(nowAt->rch);
  }

  std::shared_ptr<Node> root_;
  size_t size_;
};

#endif
