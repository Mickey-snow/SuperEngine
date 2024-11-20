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

#include "machine/call_stack.hpp"

#include <stdexcept>
#include <utility>

CallStack CallStack::Clone() const {
  if (is_locked_)
    throw std::runtime_error("CallStack: cannot clone a locked call stack.");
  CallStack result;
  result.real_frames_ = real_frames_;
  result.stack_ = stack_;
  return result;
}

void CallStack::Push(StackFrame frame) {
  if (is_locked_) {
    delayed_modifications_.emplace_back(
        [this, frame = std::move(frame)]() mutable {
          this->Push(std::move(frame));
        });
    return;
  }

  if (frame.frame_type != StackFrame::TYPE_LONGOP)
    real_frames_.push_back(stack_.size());
  stack_.emplace_back(std::move(frame));
}

void CallStack::Pop() {
  if (is_locked_) {
    delayed_modifications_.emplace_back([this]() { this->Pop(); });
    return;
  }

  if (stack_.empty())
    throw std::underflow_error("CallStack: cannot pop from an empty stack.");

  stack_.pop_back();
  if (!real_frames_.empty() && real_frames_.back() == stack_.size())
    real_frames_.pop_back();
}

size_t CallStack::Size() const { return stack_.size(); }

std::vector<StackFrame>::reverse_iterator CallStack::begin() {
  return stack_.rbegin();
}

std::vector<StackFrame>::reverse_iterator CallStack::end() {
  return stack_.rend();
}

StackFrame* CallStack::Top() const {
  if (stack_.empty())
    return nullptr;
  return const_cast<StackFrame*>(&stack_.back());
}

StackFrame* CallStack::FindTopRealFrame() const {
  if (real_frames_.empty())
    return nullptr;
  return const_cast<StackFrame*>(&stack_[real_frames_.back()]);
}

CallStack::Lock CallStack::GetLock() { return Lock(*this); }

void CallStack::ApplyDelayedModifications() {
  for (const auto& modification : delayed_modifications_)
    modification();
  delayed_modifications_.clear();
}

CallStack::Lock::~Lock() {
  call_stack_.is_locked_ = false;
  call_stack_.ApplyDelayedModifications();
}

CallStack::Lock::Lock(CallStack& call_stack) : call_stack_(call_stack) {
  if (call_stack_.is_locked_)
    throw std::logic_error("Attempts to create multiple CallStack::Lock.");
  call_stack_.is_locked_ = true;
}
