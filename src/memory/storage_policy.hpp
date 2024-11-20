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

#include <memory>
#include <utility>
#include <vector>

template <typename T>
class StoragePolicy {
 public:
  virtual ~StoragePolicy() = default;

  virtual T Get(size_t index) const = 0;
  virtual void Set(size_t index, T const& value) = 0;
  virtual void Resize(size_t size) = 0;
  virtual size_t GetSize() const = 0;
  virtual void Fill(size_t begin, size_t end, T const& value) = 0;
  virtual std::shared_ptr<StoragePolicy<T>> Clone() const = 0;

  struct Serialized {
    size_t size;
    std::vector<std::tuple<size_t, size_t, T>> data;
  };
  virtual Serialized Save() const = 0;
  virtual void Load(Serialized) = 0;
};

enum class Storage { STATIC, DYNAMIC,DEFAULT };
template <typename T>
std::shared_ptr<StoragePolicy<T>> MakeStorage(Storage type);
