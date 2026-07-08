// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
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
//
// -----------------------------------------------------------------------

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

class IntListProxy {
 public:
  explicit IntListProxy(std::vector<int>& data);

  int Get(std::size_t index, std::uint8_t bits = 32);
  void Set(std::size_t index, int value, std::uint8_t bits = 32);
  void Resize(std::size_t size);
  std::size_t GetSize() const;
  void Fill(std::size_t begin, std::size_t end, int value);

 private:
  void EnsureSizeForIndex(std::size_t index);
  void EnsureSize(std::size_t size);

  std::vector<int>& data_;
};

class StrListProxy {
 public:
  explicit StrListProxy(std::vector<std::string>& data);

  std::string Get(std::size_t index);
  void Set(std::size_t index, const std::string& value);
  void Resize(std::size_t size);
  std::size_t GetSize() const;
  void Fill(std::size_t begin, std::size_t end, const std::string& value);

 private:
  void EnsureSizeForIndex(std::size_t index);
  void EnsureSize(std::size_t size);

  std::vector<std::string>& data_;
};
