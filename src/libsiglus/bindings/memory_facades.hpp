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

#include "core/memory_internal/memory.hpp"
#include "vm/value.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace libsiglus::binding {

class SiglusIntBank {
 public:
  SiglusIntBank(Memory& memory, IntBank bank, uint8_t bits = 32);

  int get(int idx);
  void set(int idx, int value);
  void Set(int idx, std::vector<serilang::Value> values);

  int b1(int idx);
  void write_b1(int idx, int value);
  int b2(int idx);
  void write_b2(int idx, int value);
  int b4(int idx);
  void write_b4(int idx, int value);
  int b8(int idx);
  void write_b8(int idx, int value);
  int b16(int idx);
  void write_b16(int idx, int value);

  void resize(int size);
  int size() const;
  void fill(int begin, int end, int value);
  void init(int value = 0);

 private:
  int get_bits(int idx, std::uint8_t bits);
  void set_bits(int idx, int value, std::uint8_t bits);
  void EnsureSize(std::size_t logical_size);
  void EnsureSize(std::size_t logical_size, std::uint8_t bits);

  Memory* memory_;
  IntBank bank_;
  uint8_t bits_;
};

class SiglusStrBank {
 public:
  SiglusStrBank(Memory& memory, StrBank bank);

  std::string get(int idx);
  void set(int idx, std::string value);
  void Set(int idx, std::string value);
  void resize(int size);
  int size() const;
  void fill(int begin, int end, std::string value);
  void init(std::string value = "");

 private:
  void EnsureSize(std::size_t size);

  Memory* memory_;
  StrBank bank_;
};

class SiglusIntList {
 public:
  explicit SiglusIntList(int size);

  int get(int idx);
  void set(int idx, int value);
  void Set(int idx, std::vector<serilang::Value> values);
  void resize(int size);
  int size() const;
  void fill(int begin, int end, int value);
  void init();

  int b1(int idx);
  void write_b1(int idx, int value);
  int b2(int idx);
  void write_b2(int idx, int value);
  int b4(int idx);
  void write_b4(int idx, int value);
  int b8(int idx);
  void write_b8(int idx, int value);
  int b16(int idx);
  void write_b16(int idx, int value);

 private:
  int get_bits(int idx, std::uint8_t bits);
  void set_bits(int idx, int value, std::uint8_t bits);

  IntBankStorage storage_;
  std::size_t default_size_;
};

class SiglusStrList {
 public:
  explicit SiglusStrList(int size);

  std::string get(int idx);
  void set(int idx, std::string value);
  void resize(int size);
  int size() const;
  void fill(int begin, int end, std::string value);
  void init();

 private:
  StrBankStorage storage_;
  std::size_t default_size_;
};

}  // namespace libsiglus::binding
