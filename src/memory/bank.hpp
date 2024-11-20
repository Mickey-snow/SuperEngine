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

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/split_member.hpp>

#include <memory>

template <typename T>
class MemoryBank {
 public:
  MemoryBank(std::shared_ptr<StoragePolicy<T>> storage = nullptr)
      : storage_(storage) {
    if (!storage_)
      storage_ = MakeStorage<T>(Storage::DEFAULT);
  }
  MemoryBank(Storage policy, size_t init_size)
      : MemoryBank(MakeStorage<T>(policy, init_size)) {}
  ~MemoryBank() = default;

  MemoryBank(const MemoryBank<T>& other) : storage_(other.storage_->Clone()) {}
  MemoryBank<T>& operator=(const MemoryBank<T>& other) {
    storage_ = other.storage_->Clone();
    return *this;
  }

  MemoryBank(MemoryBank<T>&&) = default;
  MemoryBank<T>& operator=(MemoryBank<T>&& other) = default;

  T Get(size_t index) const { return storage_->Get(index); }
  void Set(size_t index, T const& value) { storage_->Set(index, value); };

  void Resize(size_t size) { storage_->Resize(size); };
  size_t GetSize() const { return storage_->GetSize(); }

  void Fill(size_t begin, size_t end, T const& value) {
    return storage_->Fill(begin, end, value);
  }

 private:
  std::shared_ptr<StoragePolicy<T>> storage_;

  // boost serialization support
  friend class boost::serialization::access;
  BOOST_SERIALIZATION_SPLIT_MEMBER();
  using SerializedStorage = typename StoragePolicy<T>::Serialized;

  // serialization format:
  // <size> <cnt>
  // repeat cnt times: [<fr>,<to>) <value>
  template <class Archive>
  void save(Archive& ar, unsigned int version) const {
    SerializedStorage serialized = storage_->Save();
    ar & serialized.size & serialized.data.size();
    for (const auto& [fr, to, val] : serialized.data) {
      ar & fr & to & val;
    }
  }

  template <class Archive>
  void load(Archive& ar, unsigned int version) {
    SerializedStorage serialized;
    size_t cnt;

    ar & serialized.size & cnt;
    serialized.data.reserve(cnt);
    for (size_t i = 0; i < cnt; ++i) {
      size_t fr, to;
      T value;
      ar & fr & to & value;
      serialized.data.emplace_back(std::make_tuple(fr, to, std::move(value)));
    }

    storage_->Load(std::move(serialized));
  }
};
