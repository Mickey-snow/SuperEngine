// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2024 Serina Sakurai
// Copyright (C) 2007 Elliot Glaysher
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

#include "libreallive/intmemref.h"
#include "memory/bank.hpp"
#include "memory/location.hpp"

#include <algorithm>
#include <array>
#include <boost/dynamic_bitset/dynamic_bitset.hpp>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

[[maybe_unused]] constexpr int NUMBER_OF_INT_LOCATIONS = 8;
[[maybe_unused]] constexpr int SIZE_OF_MEM_BANK = 2000;
[[maybe_unused]] constexpr int SIZE_OF_INT_PASSING_MEM = 40;
[[maybe_unused]] constexpr int SIZE_OF_NAME_BANK = 702;

class Gameexe;

struct GlobalMemory;
struct LocalMemory;

// Class that encapsulates access to all integer and string
// memory. Multiple instances of this class will probably exist if
// save games are used.
class Memory {
 public:
  Memory();
  ~Memory();

  // Reads in default memory values from the passed in Gameexe, such as \#NAME
  // and \#LOCALNAME values.
  // @note For now, we only read \#NAME and \#LOCALNAME variables, skipping any
  // declaration of the form \#intvar[index] or \#strvar[index].
  void LoadFrom(Gameexe& gameexe);

  // TODO: Extract class for this
  // Methods that record whether a piece of text has been read. RealLive
  // scripts have a piece of metadata called a kidoku marker which signifies if
  // the text between that and the next kidoku marker have been previously read.
  bool HasBeenRead(int scenario, int kidoku) const;
  void RecordKidoku(int scenario, int kidoku);

 private:
  std::map<int, boost::dynamic_bitset<>> kidoku_data;

 public:
  void Write(IntMemoryLocation, int);
  void Write(IntBank bank, size_t index, int value);
  void Write(StrMemoryLocation, const std::string&);
  void Write(StrBank bank, size_t index, const std::string& value);

  void Fill(IntBank, size_t begin, size_t end, int value);
  void Fill(StrBank, size_t begin, size_t end, const std::string& value);

  int Read(IntMemoryLocation) const;
  int Read(IntBank bank, size_t index) const;
  std::string const& Read(StrMemoryLocation) const;
  std::string const& Read(StrBank bank, size_t index) const;

  void Resize(IntBank, std::size_t);
  void Resize(StrBank, std::size_t);

  struct Stack {
    MemoryBank<int> L;
    MemoryBank<std::string> K;
  };
  // Create and return a copy of stack memory
  Stack GetStackMemory() const;

  // Create and return a copy of global memory
  GlobalMemory GetGlobalMemory() const;

  // Create and return a copy of local memory
  LocalMemory GetLocalMemory() const;

  void PartialReset(Stack stack_memory);
  void PartialReset(GlobalMemory global_memory);
  void PartialReset(LocalMemory local_memory);

 private:
  const MemoryBank<int>& GetBank(IntBank) const;
  const MemoryBank<std::string>& GetBank(StrBank) const;

  static constexpr auto int_bank_cnt = static_cast<size_t>(IntBank::CNT);
  static constexpr auto str_bank_cnt = static_cast<size_t>(StrBank::CNT);

  // internally MemoryBank<T> is a structure representing a dynamic array,
  // supports COW and can be trivally copied.
  MemoryBank<int> intbanks_[int_bank_cnt];
  MemoryBank<std::string> strbanks_[str_bank_cnt];
};
