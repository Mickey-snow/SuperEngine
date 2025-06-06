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

#include "serialization_local.hpp"

#include "core/memory_internal/storage_policy.hpp"

LocalMemory::LocalMemory()
    : A(MemoryBank<int>(Storage::DYNAMIC, 2000)),
      B(MemoryBank<int>(Storage::DYNAMIC, 2000)),
      C(MemoryBank<int>(Storage::DYNAMIC, 2000)),
      D(MemoryBank<int>(Storage::DYNAMIC, 2000)),
      E(MemoryBank<int>(Storage::DYNAMIC, 2000)),
      F(MemoryBank<int>(Storage::DYNAMIC, 2000)),
      X(MemoryBank<int>(Storage::DYNAMIC, 2000)),
      H(MemoryBank<int>(Storage::DYNAMIC, 2000)),
      I(MemoryBank<int>(Storage::DYNAMIC, 2000)),
      J(MemoryBank<int>(Storage::DYNAMIC, 2000)),
      S(MemoryBank<std::string>(Storage::DYNAMIC, 2000)),
      local_names(MemoryBank<std::string>(Storage::DYNAMIC, 2000)) {}

LocalMemory::~LocalMemory() = default;
