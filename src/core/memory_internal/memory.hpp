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

#include "core/memory_internal/location.hpp"

#include <boost/serialization/access.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/string.hpp>
#include <boost/serialization/vector.hpp>

#include <array>
#include <cstddef>
#include <string>
#include <vector>

class Gameexe;
class CallStack;
struct GlobalMemory;
struct LocalMemory;

// Class that encapsulates access to all integer and string
// memory. Multiple instances of this class will probably exist if
// save games are used.
class Memory {
 public:
  Memory();
  Memory(const Memory& other);
  Memory& operator=(const Memory& other);
  Memory(Memory&&) = default;
  Memory& operator=(Memory&&) = default;
  ~Memory();

  // Reads in default memory values from the passed in Gameexe, such as \#NAME
  // and \#LOCALNAME values.
  // @note For now, we only read \#NAME and \#LOCALNAME variables, skipping any
  // declaration of the form \#intvar[index] or \#strvar[index].
  void LoadFrom(Gameexe& gameexe);

  void AttachCallStack(CallStack* call_stack);

 public:
  void Write(IntMemoryLocation, int);
  void Write(IntBank bank, size_t index, int value);
  void Write(StrMemoryLocation, const std::string&);
  void Write(StrBank bank, size_t index, const std::string& value);

  void Fill(IntBank, size_t begin, size_t end, int value);
  void Fill(StrBank, size_t begin, size_t end, const std::string& value);

  int Read(IntMemoryLocation);
  int Read(IntBank bank, size_t index);
  std::string Read(StrMemoryLocation);
  std::string Read(StrBank bank, size_t index);

  size_t Size(IntBank bank) const;
  size_t Size(StrBank bank) const;

  void Resize(IntBank, std::size_t);
  void Resize(StrBank, std::size_t);

  struct Stack {
    std::vector<int> L;
    std::vector<std::string> K;
  };
  // Create and return a value snapshot of stack memory.
  Stack GetStackMemory() const;

  // Create and return a copy of global memory
  GlobalMemory GetGlobalMemory() const;

  // Create and return a copy of local memory
  LocalMemory GetLocalMemory() const;

  void PartialReset(Stack stack_memory);
  void PartialReset(GlobalMemory global_memory);
  void PartialReset(LocalMemory local_memory);

  std::vector<int>& GetIntBankData(IntBank);
  const std::vector<int>& GetIntBankData(IntBank) const;
  std::vector<std::string>& GetStrBankData(StrBank);
  const std::vector<std::string>& GetStrBankData(StrBank) const;

 private:
  static constexpr auto int_bank_cnt = static_cast<size_t>(IntBank::CNT);
  static constexpr auto str_bank_cnt = static_cast<size_t>(StrBank::CNT);
  static constexpr std::size_t kDefaultBankSize = 2000;

  std::array<std::vector<int>, int_bank_cnt> intbanks_;
  std::array<std::vector<std::string>, str_bank_cnt> strbanks_;
  CallStack* call_stack_ = nullptr;  // rlvm only
};

struct GlobalMemory {
  std::vector<int> G, Z;
  std::vector<std::string> M, global_names;

 private:
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, unsigned int version) {
    ar & G & Z & M & global_names;
  }
};

struct LocalMemory {
  LocalMemory();
  ~LocalMemory();

  std::vector<int> A, B, C, D, E, F, X, H, I, J;
  std::vector<std::string> S, local_names;

 private:
  friend class boost::serialization::access;
  template <class Archive>
  void serialize(Archive& ar, unsigned int version) {
    ar & A & B & C & D & E & F & X & H & I & J & S & local_names;
  }
};
