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

#include "memory/memory.hpp"

#include <iostream>
#include <map>
#include <string>
#include <unordered_set>

#include "base/gameexe.hpp"
#include "memory/serialization_global.hpp"
#include "memory/serialization_local.hpp"
#include "utilities/string_utilities.h"

bool Memory::HasBeenRead(int scenario, int kidoku) const {
  std::map<int, boost::dynamic_bitset<>>::const_iterator it =
      kidoku_data.find(scenario);

  if ((it != kidoku_data.end()) &&
      (static_cast<size_t>(kidoku) < it->second.size()))
    return it->second.test(kidoku);

  return false;
}

void Memory::RecordKidoku(int scenario, int kidoku) {
  boost::dynamic_bitset<>& bitset = kidoku_data[scenario];
  if (bitset.size() <= static_cast<size_t>(kidoku))
    bitset.resize(kidoku + 1, false);

  bitset[kidoku] = true;
}

Memory::Memory() {
  for (size_t i = 0; i < int_bank_cnt; ++i)
    intbanks_[i].Resize(SIZE_OF_MEM_BANK);
  for (size_t i = 0; i < str_bank_cnt; ++i)
    strbanks_[i].Resize(SIZE_OF_MEM_BANK);
}

Memory::~Memory() {}

void Memory::LoadFrom(Gameexe& gameexe) {
  // Note: We ignore the \#NAME_MAXLEN variable because manual allocation is
  // error prone and for losers.
  for (auto it : gameexe.Filter("NAME.")) {
    try {
      Write(StrMemoryLocation(StrBank::global_name,
                              ConvertLetterIndexToInt(it.GetKeyParts().at(1))),
            RemoveQuotes(it.ToString()));
    } catch (...) {
      std::cerr << "WARNING: Invalid format for key " << it.key() << std::endl;
    }
  }

  for (auto it : gameexe.Filter("LOCALNAME.")) {
    try {
      Write(StrMemoryLocation(StrBank::local_name,
                              ConvertLetterIndexToInt(it.GetKeyParts().at(1))),
            RemoveQuotes(it.ToString()));
    } catch (...) {
      std::cerr << "WARNING: Invalid format for key " << it.key() << std::endl;
    }
  }
}

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
    return Read(loc.Bank(), loc.Index());
  else {
    static const std::unordered_set<int> allowed_bits{1, 2, 4, 8, 16};
    if (!allowed_bits.count(bits))
      throw std::invalid_argument("Memory: access type " +
                                  std::to_string(bits) + "b not supported.");

    const auto index32 = loc.Index() * bits / 32;
    auto val32 = Read(IntMemoryLocation(loc.Bank(), index32));
    const int mask = (1 << bits) - 1;
    const int shiftbits = loc.Index() * bits % 32;
    return (val32 >> shiftbits) & mask;
  }
}

int Memory::Read(IntBank bank, size_t index) const {
  return GetBank(bank).Get(index);
}

std::string const& Memory::Read(StrMemoryLocation loc) const {
  return Read(loc.Bank(), loc.Index());
}

std::string const& Memory::Read(StrBank bank, size_t index) const {
  return GetBank(bank).Get(index);
}

void Memory::Write(IntMemoryLocation loc, int value) {
  const auto bits = loc.Bitwidth();
  if (bits == 32) {
    Write(loc.Bank(), loc.Index(), value);
    return;
  }

  auto& bank = const_cast<MemoryBank<int>&>(GetBank(loc.Bank()));
  static const std::unordered_set<int> allowed_bits{1, 2, 4, 8, 16};
  if (!allowed_bits.count(bits))
    throw std::invalid_argument("Memory: access type " + std::to_string(bits) +
                                "b not supported.");

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

void Memory::Write(IntBank bankid, size_t index, int value) {
  auto& bank = const_cast<MemoryBank<int>&>(GetBank(bankid));
  bank.Set(index, value);
}

void Memory::Write(StrMemoryLocation loc, const std::string& value) {
  auto& bank = const_cast<MemoryBank<std::string>&>(GetBank(loc.Bank()));
  bank.Set(loc.Index(), value);
}

void Memory::Write(StrBank bank, size_t index, const std::string& value) {
  Write(StrMemoryLocation(bank, index), value);
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

Memory::Stack Memory::GetStackMemory() const {
  return Stack{intbanks_[static_cast<uint8_t>(IntBank::L)],
               strbanks_[static_cast<uint8_t>(StrBank::K)]};
}

void Memory::PartialReset(Stack stack_memory) {
  intbanks_[static_cast<uint8_t>(IntBank::L)] = std::move(stack_memory.L);
  strbanks_[static_cast<uint8_t>(StrBank::K)] = std::move(stack_memory.K);
}

GlobalMemory Memory::GetGlobalMemory() const {
  return GlobalMemory{
      .G = intbanks_[static_cast<uint8_t>(IntBank::G)],
      .Z = intbanks_[static_cast<uint8_t>(IntBank::Z)],
      .M = strbanks_[static_cast<uint8_t>(StrBank::M)],
      .global_names = strbanks_[static_cast<uint8_t>(StrBank::global_name)]};
}

void Memory::PartialReset(GlobalMemory global_memory) {
  intbanks_[static_cast<uint8_t>(IntBank::G)] = std::move(global_memory.G);
  intbanks_[static_cast<uint8_t>(IntBank::Z)] = std::move(global_memory.Z);
  strbanks_[static_cast<uint8_t>(StrBank::M)] = std::move(global_memory.M);
  strbanks_[static_cast<uint8_t>(StrBank::global_name)] =
      std::move(global_memory.global_names);
}

LocalMemory Memory::GetLocalMemory() const {
  return LocalMemory{
      .A = intbanks_[static_cast<uint8_t>(IntBank::A)],
      .B = intbanks_[static_cast<uint8_t>(IntBank::B)],
      .C = intbanks_[static_cast<uint8_t>(IntBank::C)],
      .D = intbanks_[static_cast<uint8_t>(IntBank::D)],
      .E = intbanks_[static_cast<uint8_t>(IntBank::E)],
      .F = intbanks_[static_cast<uint8_t>(IntBank::F)],
      .X = intbanks_[static_cast<uint8_t>(IntBank::X)],
      .H = intbanks_[static_cast<uint8_t>(IntBank::H)],
      .I = intbanks_[static_cast<uint8_t>(IntBank::I)],
      .J = intbanks_[static_cast<uint8_t>(IntBank::J)],
      .S = strbanks_[static_cast<uint8_t>(StrBank::S)],
      .local_names = strbanks_[static_cast<uint8_t>(StrBank::local_name)],
  };
}

void Memory::PartialReset(LocalMemory local_memory) {
  intbanks_[static_cast<uint8_t>(IntBank::A)] = std::move(local_memory.A);
  intbanks_[static_cast<uint8_t>(IntBank::B)] = std::move(local_memory.B);
  intbanks_[static_cast<uint8_t>(IntBank::C)] = std::move(local_memory.C);
  intbanks_[static_cast<uint8_t>(IntBank::D)] = std::move(local_memory.D);
  intbanks_[static_cast<uint8_t>(IntBank::E)] = std::move(local_memory.E);
  intbanks_[static_cast<uint8_t>(IntBank::F)] = std::move(local_memory.F);
  intbanks_[static_cast<uint8_t>(IntBank::X)] = std::move(local_memory.X);
  intbanks_[static_cast<uint8_t>(IntBank::H)] = std::move(local_memory.H);
  intbanks_[static_cast<uint8_t>(IntBank::I)] = std::move(local_memory.I);
  intbanks_[static_cast<uint8_t>(IntBank::J)] = std::move(local_memory.J);
  strbanks_[static_cast<uint8_t>(StrBank::S)] = std::move(local_memory.S);
  strbanks_[static_cast<uint8_t>(StrBank::local_name)] =
      std::move(local_memory.local_names);
}
