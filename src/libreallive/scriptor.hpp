// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2024 Serina Sakurai
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

#include <boost/iterator/iterator_facade.hpp>
#include <memory>
#include <string>
#include <vector>

namespace libreallive {

class Archive;
class Scenario;
class BytecodeElement;

class Scriptor {
 public:
  explicit Scriptor(Archive& ar);
  ~Scriptor();

  class const_iterator
      : public boost::iterator_facade<const_iterator,
                                      const std::shared_ptr<BytecodeElement>&,
                                      boost::forward_traversal_tag> {
   public:
    int ScenarioNumber() const;
    uint32_t Location() const;
    bool HasNext() const;
    const Scenario* GetScenario() const;

   private:
    friend class Scriptor;
    const_iterator(const Scenario* sc, std::size_t off);

    const Scenario* scenario_;
    std::size_t offset_;

    friend class boost::iterator_core_access;

    void increment();
    bool equal(const const_iterator& other) const;
    const std::shared_ptr<BytecodeElement>& dereference() const;
  };

  const_iterator Load(int scenario_number, uint32_t loc);
  const_iterator LoadEntry(int scenario_number, int entry);

 private:
  Archive& archive_;
};

}  // namespace libreallive
