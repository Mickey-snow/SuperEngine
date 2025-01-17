// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2025 Serina Sakurai
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

#include <boost/serialization/split_member.hpp>

#include <map>
#include <set>

class KidokuTable {
 public:
  KidokuTable() = default;

  // Checks if a specific piece of text has been read in a given scenario.
  bool HasBeenRead(int scenario, int kidoku) const;

  // Marks a specific piece of text as read in a given scenario.
  void RecordKidoku(int scenario, int kidoku);

 private:
  // Maps each scenario to a set of kidoku markers.
  std::map<int, std::set<int>> kidoku_data_;

  // boost serialization support
  friend class boost::serialization::access;

  // serialization format:
  // n <scenario count>
  // following n lines
  // s <scenario id> m <kidoku count> k1 k2 ... km
  void save(auto& ar, const unsigned int version) const {
    ar & kidoku_data_.size();
    for (const auto& [scenario, table] : kidoku_data_) {
      ar & scenario & table.size();
      for (auto it : table)
        ar& static_cast<int>(it);
    }
  }

  void load(auto& ar, const unsigned int version) {
    kidoku_data_.clear();

    int scenario_count, kidoku_count, scenario_id;

    ar & scenario_count;
    while (scenario_count--) {
      ar & scenario_id & kidoku_count;
      auto& table = kidoku_data_[scenario_id];
      while (kidoku_count--) {
        int x;
        ar & x;
        table.insert(x);
      }
    }
  }

  BOOST_SERIALIZATION_SPLIT_MEMBER();
};
