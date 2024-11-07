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

#ifndef SRC_MEMORY_MEMORY_HPP_
#define SRC_MEMORY_MEMORY_HPP_

#include <boost/dynamic_bitset.hpp>
#include <boost/serialization/array.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/serialization/version.hpp>

#include <algorithm>
#include <array>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "libreallive/intmemref.h"
#include "memory/bank.hpp"
#include "memory/location.hpp"

constexpr int NUMBER_OF_INT_LOCATIONS = 8;
constexpr int SIZE_OF_MEM_BANK = 2000;
constexpr int SIZE_OF_INT_PASSING_MEM = 40;
constexpr int SIZE_OF_NAME_BANK = 702;

class RLMachine;
class Gameexe;

// Struct that represents Global Memory. In any one rlvm process, there
// should only be one GlobalMemory struct existing, as it will be
// shared over all the Memory objects in the process.
struct GlobalMemory {
  GlobalMemory();

  std::array<int, SIZE_OF_MEM_BANK> intG, intZ;
  std::array<std::string, SIZE_OF_MEM_BANK> strM;
  std::array<std::string, SIZE_OF_NAME_BANK> global_names;

  // A mapping from a scenario number to a dynamic bitset, where each bit
  // represents a specific kidoku bit.
  std::map<int, boost::dynamic_bitset<>> kidoku_data;

  // boost::serialization
  template <class Archive>
  void serialize(Archive& ar, unsigned int version) {
    int _G[2000] = {}, _Z[2000] = {};
    std::string _M[2000], _name[702];
    ar & _G & _Z & _M;

    // Starting in version 1, \#NAME variable storage were added.
    if (version > 0) {
      ar & _name;
      ar & kidoku_data;
    }

    std::copy(_G, _G + 2000, intG.begin());
    std::copy(_Z, _Z + 2000, intZ.begin());
    std::copy(_M, _M + 2000, strM.begin());
    std::copy(_name, _name + 702, global_names.begin());
  }
};

BOOST_CLASS_VERSION(GlobalMemory, 1)

// Struct that represents Local Memory. In any one rlvm process, lots
// of these things will be created, because there are commands
struct LocalMemory {
  LocalMemory();

  // Zeros and clears all of local memory.
  void reset();

  std::array<int, SIZE_OF_MEM_BANK> intA, intB, intC, intD, intE, intF;

  std::array<std::string, SIZE_OF_MEM_BANK> strS;

  // When one of our values is changed, we put the original value in here. Why?
  // So that we can save the state of string memory at the time of the last
  // Savepoint(). Instead of doing some sort of copying entire memory banks
  // whenever we hit a Savepoint() call, only reconstruct the original memory
  // when we save.
  std::map<int, int> original_intA;
  std::map<int, int> original_intB;
  std::map<int, int> original_intC;
  std::map<int, int> original_intD;
  std::map<int, int> original_intE;
  std::map<int, int> original_intF;
  std::map<int, std::string> original_strS;

  std::array<std::string, SIZE_OF_NAME_BANK> local_names;

  // Combines an array with a log of original values and writes the de-modified
  // array to |ar|.
  // template <class Archive, typename T>
  // void saveArrayRevertingChanges(Archive& ar,
  //                                const T& a,
  //                                const std::map<int, T>& original) const {
  //   T merged[SIZE_OF_MEM_BANK];
  //   std::copy(a.cbegin(), a.cend(), merged);
  //   for (auto it = original.cbegin(); it != original.cend(); ++it) {
  //     merged[it->first] = it->second;
  //   }
  //   ar & merged;
  // }

  // boost::serialization support
  template <class Archive>
  void save(Archive& ar, unsigned int version) const {
    // saveArrayRevertingChanges(ar, intA, original_intA);
    // saveArrayRevertingChanges(ar, intB, original_intB);
    // saveArrayRevertingChanges(ar, intC, original_intC);
    // saveArrayRevertingChanges(ar, intD, original_intD);
    // saveArrayRevertingChanges(ar, intE, original_intE);
    // saveArrayRevertingChanges(ar, intF, original_intF);

    // saveArrayRevertingChanges(ar, strS, original_strS);

    ar & local_names;
  }

  template <class Archive>
  void load(Archive& ar, unsigned int version) {
    ar & intA & intB & intC & intD & intE & intF & strS;

    // Starting in version 2, we no longer have the intL and strK in
    // LocalMemory. They were moved to StackFrame because they're stack
    // local. Throw away old data.
    if (version < 2) {
      int intL[SIZE_OF_INT_PASSING_MEM];
      ar & intL;

      std::string strK[3];
      ar & strK;
    }

    // Starting in version 1, \#LOCALNAME variable storage were added.
    if (version > 0)
      ar & local_names;
  }

  BOOST_SERIALIZATION_SPLIT_MEMBER()
};

BOOST_CLASS_VERSION(LocalMemory, 2)

// Class that encapsulates access to all integer and string
// memory. Multiple instances of this class will probably exist if
// save games are used.
//
// @note Because I use BSD code from xclannad in some of the methods
//       in this class, for licensing purposes, that code is separated
//       into memory_intmem.cc.
class Memory {
 public:
  Memory(std::shared_ptr<GlobalMemory> global_memptr = nullptr);
  ~Memory();

  // Methods that record whether a piece of text has been read. RealLive
  // scripts have a piece of metadata called a kidoku marker which signifies if
  // the text between that and the next kidoku marker have been previously read.
  bool HasBeenRead(int scenario, int kidoku) const;
  void RecordKidoku(int scenario, int kidoku);

  // Accessors for serialization.
  GlobalMemory& global() { return *global_; }
  auto globalptr() { return global_; }
  const GlobalMemory& global() const { return *global_; }
  LocalMemory& local() { return local_; }
  const LocalMemory& local() const { return local_; }

  // Reads in default memory values from the passed in Gameexe, such as \#NAME
  // and \#LOCALNAME values.
  // @note For now, we only read \#NAME and \#LOCALNAME variables, skipping any
  // declaration of the form \#intvar[index] or \#strvar[index].
  void LoadFrom(Gameexe& gameexe);

 private:
  // Connects the memory banks in local_ and in global_ into int_var.
  void ConnectIntVarPointers();

  // Pointer to the GlobalMemory structure. While there can (and will
  // be) multiple Memory instances (this is how we implement
  // GetSaveFlag), we don't really need to duplicate this data
  // structure and can simply pass a pointer to it.
  std::shared_ptr<GlobalMemory> global_;

  // Local memory to a save file
  LocalMemory local_;

  // Integer variable pointers. This redirect into Global and local
  // memory (as the case may be) allows us to overlay new views of
  // local memory without copying global memory.
  int* int_var[NUMBER_OF_INT_LOCATIONS];

  // Change records for original.
  std::map<int, int>* original_int_var[NUMBER_OF_INT_LOCATIONS];

  // -----------------------------------------------------------

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
  Stack StackMemory() const;

  void PartialReset(Stack stack_memory);

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

#endif  // SRC_MEMORY_MEMORY_HPP_
