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

#pragma once

#include "m6/value.hpp"

#include <memory>
#include <string>

namespace m6 {

class SymbolTable;

/**
 * @brief A wrapper for variable references (lvalues) that interact with the
 * symbol table.
 *
 * The lValue class encapsulates a reference to a variable stored in the
 * SymbolTable. It allows the interpreter to treat variables uniformly while
 * intercepting assignment operations. Specifically, when an assignment (or
 * compound assignment) operator is invoked, lValue updates the underlying
 * variable in the symbol table, creating it if necessary. For all other
 * operations (such as arithmetic or bitwise operators), lValue forwards the
 * call to the current value stored in the symbol table.
 *
 */
class lValue : public IValue {
 public:
  lValue(std::shared_ptr<SymbolTable> sym_tab, std::string name);
  virtual std::string Str() const override;
  virtual std::string Desc() const override;

  virtual std::type_index Type() const override;

  virtual std::any Get() const override;

  virtual Value Operator(Op op, Value rhs) const override;
  virtual Value Operator(Op op) const override;

 private:
  Value Deref() const;

  std::shared_ptr<SymbolTable> sym_tab_;
  std::string name_;
};

}  // namespace m6
