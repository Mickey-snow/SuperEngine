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

#include "core/memory_internal/stack_adapter.hpp"

#include "machine/call_stack.hpp"

#include <stdexcept>

inline static MemoryBank<int>& GetL(CallStack& stack) {
  auto top_frame = stack.FindTopRealFrame();
  if (top_frame == nullptr)
    throw std::runtime_error("StackMemoryAdapter: stack is empty.");
  return top_frame->intL;
}

inline static MemoryBank<std::string>& GetK(CallStack& stack) {
  auto top_frame = stack.FindTopRealFrame();
  if (top_frame == nullptr)
    throw std::runtime_error("StackMemoryAdapter: stack is empty.");
  return top_frame->strK;
}

template <>
StackMemoryAdapter<StackBank::IntL>::StackMemoryAdapter(CallStack& stack)
    : stack_(stack) {}

template <>
int StackMemoryAdapter<StackBank::IntL>::Get(size_t index) const {
  return GetL(stack_).Get(index);
}

template <>
void StackMemoryAdapter<StackBank::IntL>::Set(size_t index, int const& value) {
  GetL(stack_).Set(index, value);
}

template <>
void StackMemoryAdapter<StackBank::IntL>::Resize(size_t size) {
  GetL(stack_).Resize(size);
}

template <>
size_t StackMemoryAdapter<StackBank::IntL>::GetSize() const {
  return GetL(stack_).GetSize();
}

template <>
void StackMemoryAdapter<StackBank::IntL>::Fill(size_t begin,
                                               size_t end,
                                               int const& value) {
  return GetL(stack_).Fill(begin, end, value);
}

// -----------------------------------------------------------------------

template <>
StackMemoryAdapter<StackBank::StrK>::StackMemoryAdapter(CallStack& stack)
    : stack_(stack) {}

template <>
std::string StackMemoryAdapter<StackBank::StrK>::Get(size_t index) const {
  return GetK(stack_).Get(index);
}

template <>
void StackMemoryAdapter<StackBank::StrK>::Set(size_t index,
                                              std::string const& value) {
  GetK(stack_).Set(index, value);
}

template <>
void StackMemoryAdapter<StackBank::StrK>::Resize(size_t size) {
  GetK(stack_).Resize(size);
}

template <>
size_t StackMemoryAdapter<StackBank::StrK>::GetSize() const {
  return GetK(stack_).GetSize();
}

template <>
void StackMemoryAdapter<StackBank::StrK>::Fill(size_t begin,
                                               size_t end,
                                               std::string const& value) {
  return GetK(stack_).Fill(begin, end, value);
}
