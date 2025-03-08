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

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>

class Value;

namespace m6 {

/**
 * @brief Exception thrown when a symbol is not found in the symbol table.
 */
class NameError : public std::runtime_error {
 public:
  explicit NameError(const std::string& name);
  using std::runtime_error::what;
};

/**
 * @brief A symbol table that maps names to values.
 *
 * This class provides basic operations to store, retrieve, update,
 * and remove symbols associated with their values.
 */
class SymbolTable {
 public:
  /**
   * @brief Constructs a new SymbolTable object.
   */
  SymbolTable();

  /**
   * @brief Checks if a symbol exists in the table.
   * @param name The name of the symbol to check.
   * @return true if the symbol exists, false otherwise.
   */
  virtual bool Exists(const std::string& name) const;

  /**
   * @brief Retrieves the value associated with a symbol.
   * @param name The name of the symbol.
   * @return A shared pointer to the associated IValue.
   * @throws NameError if the symbol is not found.
   */
  virtual std::shared_ptr<Value> Get(const std::string& name) const;

  /**
   * @brief Inserts or updates a symbol with its associated value.
   * @param name The name of the symbol.
   * @param value The value to be associated with the symbol.
   */
  virtual void Set(const std::string& name, std::shared_ptr<Value> value);

  /**
   * @brief Removes a symbol from the table.
   * @param name The name of the symbol to remove.
   * @return true if the symbol was successfully removed, false otherwise.
   */
  virtual bool Remove(const std::string& name);

  /**
   * @brief Clears the entire symbol table.
   */
  virtual void Clear();

 private:
  /// Internal storage mapping symbol names to their values.
  std::unordered_map<std::string, std::shared_ptr<Value>> table_;
};

}  // namespace m6
