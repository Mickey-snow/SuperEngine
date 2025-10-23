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

#include "vm/promise.hpp"

#include "vm/future.hpp"
#include "vm/object.hpp"

#include <algorithm>
#include <stdexcept>

namespace serilang {

std::string Promise::ToDebugString() const { return "<promise>"; }

auto Promise::AddWaker(waker_t waker) -> bool {
  if (!waker)
    throw std::invalid_argument("Promise: waker should not be nullptr");

  if (HasResult()) {
    waker(*result);
    ++wake_count_;
    return false;
  } else {
    wakers_.emplace_back(std::move(waker));
    return true;
  }
}

void Promise::WakeAll() {
  std::for_each(wakers_.begin(), wakers_.end(),
                [this](auto it) { it(this->result.value()); });
  wake_count_ += wakers_.size();
  wakers_.clear();
}

void Promise::Resolve(Value value) {
  if (HasResult())
    return;
  AddRoot(value);
  result = value;
  status = Status::Resolved;
  WakeAll();
}

void Promise::Reject(std::string msg) {
  if (HasResult())
    return;
  result = unexpected(std::move(msg));
  status = Status::Rejected;
  WakeAll();
}

//------------------------------------------------------------------------------

std::shared_ptr<Promise> get_promise(Value& value) {
  if (auto tf = value.Get_if<Fiber>()) {
    return tf->completion_promise;
  } else if (auto ft = value.Get_if<Future>()) {
    return ft->promise;
  }
  return nullptr;
}

}  // namespace serilang
