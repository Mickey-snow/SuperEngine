// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2025 Serina Sakurai
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

#include "m6/symbol_table.hpp"
#include "m6/exception.hpp"

namespace m6 {

//============================================================================
// SymbolTable Implementation
//============================================================================

SymbolTable::SymbolTable() = default;

bool SymbolTable::Exists(const std::string& name) const {
  return table_.find(name) != table_.end();
}

std::shared_ptr<Value> SymbolTable::Get(const std::string& name) const {
  auto it = table_.find(name);
  if (it != table_.end()) {
    return it->second;
  }
  throw NameError(name);
}

void SymbolTable::Set(const std::string& name, std::shared_ptr<Value> value) {
  table_[name] = value;
}

bool SymbolTable::Remove(const std::string& name) {
  return table_.erase(name) > 0;
}

void SymbolTable::Clear() { table_.clear(); }

}  // namespace m6
