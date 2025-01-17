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

#include "core/memory_internal/storage_policy.hpp"

#include "core/memory_internal/dynamic_storage.hpp"

#include <stdexcept>
#include <string>

template <typename T>
std::shared_ptr<StoragePolicy<T>> MakeStorage(Storage type, size_t size) {
  switch (type) {
    case Storage::DEFAULT:
    case Storage::DYNAMIC: {
      auto result = std::make_shared<DynamicStorage<T>>();
      result->Resize(size);
      return result;
    }

    case Storage::STATIC:

    default:
      throw std::invalid_argument("MakeStorage: Unknown policy type " +
                                  std::to_string(static_cast<int>(type)));
  }
}

template std::shared_ptr<StoragePolicy<int>> MakeStorage(Storage type,
                                                         size_t size);
template std::shared_ptr<StoragePolicy<std::string>> MakeStorage(Storage type,
                                                                 size_t size);
