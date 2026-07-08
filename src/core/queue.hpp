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

#pragma once

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <mutex>
#include <stdexcept>

template <typename T>
class Queue {
 public:
  Queue(std::size_t capacity) : capacity_(capacity) {
    if (capacity <= 0)
      throw std::invalid_argument("capacity too small.");
  }

  void Push(T data) {
    std::unique_lock<std::mutex> lock(mtx_);
    not_full_.wait(lock, [&] { return queue_.size() < capacity_ || stopped_; });
    if (stopped_)
      return;
    queue_.emplace_back(std::move(data));
    not_empty_.notify_one();
  }

  bool Pop(T& out) {
    std::unique_lock<std::mutex> lock(mtx_);
    not_empty_.wait(lock, [&] { return !queue_.empty() || stopped_; });
    if (queue_.empty())
      return false;
    out = std::move(queue_.front());
    queue_.pop_front();
    not_full_.notify_one();
    return true;
  }

  void Stop() {
    {
      std::lock_guard<std::mutex> lock(mtx_);
      stopped_ = true;
    }
    not_full_.notify_all();
    not_empty_.notify_all();
  }

  std::size_t Size() {
    std::lock_guard<std::mutex> lock(mtx_);
    return queue_.size();
  }

  mutable std::mutex mtx_;
  std::size_t capacity_;
  std::deque<T> queue_;
  std::condition_variable not_full_;
  std::condition_variable not_empty_;
  bool stopped_ = false;
};
