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

#include "m6/libs/libstr.hpp"

#include "m6/symbol_table.hpp"
#include "m6/value.hpp"

namespace m6 {

void LoadLibstr(std::shared_ptr<SymbolTable> symtab) {
  Value strcpy_fn =
      make_value("strcpy",
                 [](std::vector<Value> args,
                    std::map<std::string, Value> kwargs) -> Value {
                   auto src = static_cast<std::string*>(args[1]->Getptr());
                   auto dst = static_cast<std::string*>(args[0]->Getptr());
                   int cnt = std::any_cast<int>(args[2]->Get());
                   *dst = src->substr(0, cnt);

                   return {};
                 });

  symtab->Set("strcpy", strcpy_fn);
}

}  // namespace m6
