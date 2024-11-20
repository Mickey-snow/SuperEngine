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

#include <stdexcept>
#include <type_traits>

class CallStack;

enum class StackBank { IntL, StrK };

namespace {
template <StackBank T>
struct StackBankTraits;
template <>
struct StackBankTraits<StackBank::IntL> {
  using Type = int;
};
template <>
struct StackBankTraits<StackBank::StrK> {
  using Type = std::string;
};
template <StackBank T>
using StackBank_t = typename StackBankTraits<T>::Type;
}  // namespace

template <StackBank bank>
class StackMemoryAdapter : public StoragePolicy<StackBank_t<bank>> {
 public:
  StackMemoryAdapter(CallStack& stack);

  using T = StackBank_t<bank>;

  virtual T Get(size_t index) const override;
  virtual void Set(size_t index, T const& value) override;
  virtual void Resize(size_t size) override;
  virtual size_t GetSize() const override;
  virtual void Fill(size_t begin, size_t end, T const& value) override;
  virtual std::shared_ptr<StoragePolicy<T>> Clone() const override {
    return std::make_shared<StackMemoryAdapter<bank>>(*this);
  }

  using Serialized = typename StoragePolicy<T>::Serialized;
  virtual Serialized Save() const override {
    throw std::runtime_error("StackMemoryAdapter: Save() not supported.");
  }
  virtual void Load(Serialized) override {
    throw std::runtime_error("StackMemoryAdapter: Save() not supported.");
  }

 private:
  CallStack& stack_;
};
