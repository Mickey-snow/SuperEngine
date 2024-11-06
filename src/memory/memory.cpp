// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
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

#include "memory/memory.hpp"

#include <iostream>
#include <map>
#include <string>
#include <unordered_set>

#include "base/gameexe.hpp"
#include "libreallive/intmemref.h"
#include "machine/rlmachine.h"
#include "utilities/exception.h"
#include "utilities/string_utilities.h"

// -----------------------------------------------------------------------
// GlobalMemory
// -----------------------------------------------------------------------
GlobalMemory::GlobalMemory() = default;

// -----------------------------------------------------------------------
// LocalMemory
// -----------------------------------------------------------------------
LocalMemory::LocalMemory() = default;

void LocalMemory::reset() {
  intA.fill(0);
  intB.fill(0);
  intC.fill(0);
  intD.fill(0);
  intE.fill(0);
  intF.fill(0);

  for (auto& str : strS)
    str.clear();
  for (auto& name : local_names)
    name.clear();
}

// -----------------------------------------------------------------------
// Memory
// -----------------------------------------------------------------------
Memory::Memory(RLMachine& machine, Gameexe& gameexe)
    : Memory(std::make_shared<MemoryServices>(machine)) {
  InitializeDefaultValues(gameexe);
}

Memory::Memory(std::shared_ptr<IMemoryServices> services,
               std::shared_ptr<GlobalMemory> global)
    : global_(global), local_(), service_(services) {
  if (global_ == nullptr)
    global_ = std::make_shared<GlobalMemory>();
  ConnectIntVarPointers();

  for (size_t i = 0; i < int_bank_cnt; ++i)
    intbanks_[i].Resize(SIZE_OF_MEM_BANK);
  for (size_t i = 0; i < str_bank_cnt; ++i)
    strbanks_[i].Resize(SIZE_OF_MEM_BANK);
}

Memory::~Memory() {}

void Memory::ConnectIntVarPointers() {
  int_var[0] = local_.intA.data();
  int_var[1] = local_.intB.data();
  int_var[2] = local_.intC.data();
  int_var[3] = local_.intD.data();
  int_var[4] = local_.intE.data();
  int_var[5] = local_.intF.data();
  int_var[6] = global_->intG.data();
  int_var[7] = global_->intZ.data();

  original_int_var[0] = &local_.original_intA;
  original_int_var[1] = &local_.original_intB;
  original_int_var[2] = &local_.original_intC;
  original_int_var[3] = &local_.original_intD;
  original_int_var[4] = &local_.original_intE;
  original_int_var[5] = &local_.original_intF;
  original_int_var[6] = NULL;
  original_int_var[7] = NULL;
}

void Memory::CheckNameIndex(int index, const std::string& name) const {
  if (index > (SIZE_OF_NAME_BANK - 1)) {
    std::ostringstream oss;
    oss << "Invalid index " << index << " in " << name;
    throw rlvm::Exception(oss.str());
  }
}

void Memory::SetName(int index, const std::string& name) {
  CheckNameIndex(index, "Memory::set_name");
  global_->global_names[index] = name;
}

const std::string& Memory::GetName(int index) const {
  CheckNameIndex(index, "Memory::get_name");
  return global_->global_names[index];
}

void Memory::SetLocalName(int index, const std::string& name) {
  CheckNameIndex(index, "Memory::set_local_name");
  local_.local_names[index] = name;
}

const std::string& Memory::GetLocalName(int index) const {
  CheckNameIndex(index, "Memory::set_local_name");
  return local_.local_names[index];
}

bool Memory::HasBeenRead(int scenario, int kidoku) const {
  std::map<int, boost::dynamic_bitset<>>::const_iterator it =
      global_->kidoku_data.find(scenario);

  if ((it != global_->kidoku_data.end()) &&
      (static_cast<size_t>(kidoku) < it->second.size()))
    return it->second.test(kidoku);

  return false;
}

void Memory::RecordKidoku(int scenario, int kidoku) {
  boost::dynamic_bitset<>& bitset = global_->kidoku_data[scenario];
  if (bitset.size() <= static_cast<size_t>(kidoku))
    bitset.resize(kidoku + 1, false);

  bitset[kidoku] = true;
}

void Memory::TakeSavepointSnapshot() {
  local_.original_intA.clear();
  local_.original_intB.clear();
  local_.original_intC.clear();
  local_.original_intD.clear();
  local_.original_intE.clear();
  local_.original_intF.clear();
  local_.original_strS.clear();
}

// static
int Memory::ConvertLetterIndexToInt(const std::string& value) {
  int total = 0;

  if (value.size() == 1) {
    total += (value[0] - 'A');
  } else if (value.size() == 2) {
    total += 26 * ((value[0] - 'A') + 1);
    total += (value[1] - 'A');
  } else {
    throw rlvm::Exception("Invalid value in convert_name_var!");
  }

  return total;
}

void Memory::InitializeDefaultValues(Gameexe& gameexe) {
  // Note: We ignore the \#NAME_MAXLEN variable because manual allocation is
  // error prone and for losers.
  for (auto it : gameexe.Filter("NAME.")) {
    try {
      SetName(ConvertLetterIndexToInt(it.GetKeyParts().at(1)),
              RemoveQuotes(it.ToString()));
    } catch (...) {
      std::cerr << "WARNING: Invalid format for key " << it.key() << std::endl;
    }
  }

  for (auto it : gameexe.Filter("LOCALNAME.")) {
    try {
      SetLocalName(ConvertLetterIndexToInt(it.GetKeyParts().at(1)),
                   RemoveQuotes(it.ToString()));
    } catch (...) {
      std::cerr << "WARNING: Invalid format for key " << it.key() << std::endl;
    }
  }
}

// -----------------------------------------------------------------------

const MemoryBank<int>& Memory::GetBank(IntBank bank) const {
  const auto bankidx = static_cast<uint8_t>(bank);
  if (bankidx >= int_bank_cnt)
    throw std::invalid_argument("Memory: invalid int bank " +
                                std::to_string(bankidx));
  return intbanks_[bankidx];
}

const MemoryBank<std::string>& Memory::GetBank(StrBank bank) const {
  const auto bankidx = static_cast<uint8_t>(bank);
  if (bankidx >= str_bank_cnt)
    throw std::invalid_argument("Memory: invalid string bank " +
                                std::to_string(bankidx));
  return strbanks_[bankidx];
}

int Memory::Read(IntMemoryLocation loc) const {
  const auto bits = loc.Bitwidth();
  if (bits == 32)
    return GetBank(loc.Bank()).Get(loc.Index());
  else {
    static const std::unordered_set<int> allowed_bits{1, 2, 4, 8, 16};
    if (!allowed_bits.count(bits))
      throw std::invalid_argument("Memory: access type " +
                                  std::to_string(bits) + " not supported.");

    const auto index32 = loc.Index() * bits / 32;
    auto val32 = Read(IntMemoryLocation(loc.Bank(), index32));
    const int mask = (1 << bits) - 1;
    const int shiftbits = loc.Index() * bits % 32;
    return (val32 >> shiftbits) & mask;
  }
}

std::string const& Memory::Read(StrMemoryLocation loc) const {
  return GetBank(loc.Bank()).Get(loc.Index());
}

void Memory::Write(IntMemoryLocation loc, int value) {
  auto& bank = const_cast<MemoryBank<int>&>(GetBank(loc.Bank()));

  if (loc.Bitwidth() == 32)
    bank.Set(loc.Index(), value);
  else {
    const auto bits = loc.Bitwidth();

    static const std::unordered_set<int> allowed_bits{1, 2, 4, 8, 16};
    if (!allowed_bits.count(bits))
      throw std::invalid_argument("Memory: access type " +
                                  std::to_string(bits) + " not supported.");

    const auto index32 = loc.Index() * bits / 32;
    auto val32 = Read(IntMemoryLocation(loc.Bank(), index32));
    int mask = (1 << bits) - 1;
    if (value > mask) {
      throw std::overflow_error("Memory: value " + std::to_string(value) +
                                " overflow when casting to " +
                                std::to_string(bits) + " bit int.");
    }
    const int shiftbits = loc.Index() * bits % 32;
    mask <<= shiftbits;
    val32 &= (~mask);
    val32 |= value << shiftbits;
    bank.Set(index32, val32);
  }
}

void Memory::Write(StrMemoryLocation loc, const std::string& value) {
  auto& bank = const_cast<MemoryBank<std::string>&>(GetBank(loc.Bank()));
  bank.Set(loc.Index(), value);

  if (loc.Bank() == StrBank::K) {
    auto& currentStrKBank = service_->StrKBank();
    const auto index = loc.Index();
    if (index >= currentStrKBank.size())
      currentStrKBank.resize(index + 1);
    currentStrKBank[index] = value;
  }
}

void Memory::Fill(IntBank bankid, size_t begin, size_t end, int value) {
  auto& bank = const_cast<MemoryBank<int>&>(GetBank(bankid));
  if (begin > end) {
    throw std::invalid_argument("Memory::Fill: invalid range [" +
                                std::to_string(begin) + ',' +
                                std::to_string(end) + ").");
  }
  if (end > bank.GetSize()) {
    throw std::out_of_range("Memory::Fill: range [" + std::to_string(begin) +
                            ',' + std::to_string(end) + ") out of bounds.");
  }

  bank.Fill(begin, end, value);
}

void Memory::Fill(StrBank bankid,
                  size_t begin,
                  size_t end,
                  const std::string& value) {
  auto& bank = const_cast<MemoryBank<std::string>&>(GetBank(bankid));
  if (begin > end) {
    throw std::invalid_argument("Memory::Fill: invalid range [" +
                                std::to_string(begin) + ',' +
                                std::to_string(end) + ").");
  }
  if (end > bank.GetSize()) {
    throw std::out_of_range("Memory::Fill: range [" + std::to_string(begin) +
                            ',' + std::to_string(end) + ") out of bounds.");
  }

  bank.Fill(begin, end, value);
}

void Memory::Resize(IntBank bankid, std::size_t size) {
  auto& bank = const_cast<MemoryBank<int>&>(GetBank(bankid));
  bank.Resize(size);
}

void Memory::Resize(StrBank bankid, std::size_t size) {
  auto& bank = const_cast<MemoryBank<std::string>&>(GetBank(bankid));
  bank.Resize(size);
}
