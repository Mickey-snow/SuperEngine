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

#ifndef SRC_OBJECT_PARAMETER_MANAGER_H_
#define SRC_OBJECT_PARAMETER_MANAGER_H_

#include "object/properties.h"
#include "utilities/mpl.h"

#include <any>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class Scapegoat {
 public:
  using key_t = int;
  using value_t = std::any;
  static constexpr float alpha = 0.78f;

  Scapegoat();
  ~Scapegoat();

  std::any const& operator[](int key) const;
  std::any const& Get(int key) const;

  void Set(int key, std::any value);

  bool Contains(int key) const;

  void Remove(int key);

 private:
  struct Node {
    key_t key;
    value_t value;
    size_t tree_size;
    std::shared_ptr<Node> lch, rch;

    Node(key_t ikey, std::any ival = {});

    Node& Update();

    bool IsBalance() const;
  };

  std::shared_ptr<Node> Find(key_t key,
                             const std::shared_ptr<Node>& nowAt) const;

  std::shared_ptr<Node> Touch(key_t key,
                              value_t value,
                              std::shared_ptr<Node>& nowAt);

  void Remove(key_t key, std::shared_ptr<Node>& nowAt);

  void CheckRebuild(key_t key, std::shared_ptr<Node>& nowAt);

  std::shared_ptr<Node> Rebuild(std::shared_ptr<Node> root);

  std::shared_ptr<Node> Build(
      const std::vector<std::pair<key_t, value_t>>& nodes,
      size_t fr,
      size_t to);

  void Collect(std::shared_ptr<Node> nowAt,
               std::vector<std::pair<key_t, value_t>>& container) const;

  std::shared_ptr<Node> root;
};

class ParameterManager {
 public:
  ParameterManager() = default;
  ~ParameterManager() = default;

  void Set(ObjectProperty property, std::any value) {
    bst_.Set(static_cast<int>(property), std::move(value));
  }

  template <ObjectProperty property>
  auto Get() {
    constexpr int param_id = static_cast<int>(property);
    if constexpr (param_id < 0) {
      static_assert(false, "Invalid parameter ID");
    }

    using result_t = typename GetNthType<static_cast<size_t>(property),
                                         ObjectPropertyType>::type;
    try {
      return std::any_cast<result_t>(bst_.Get(param_id));
    } catch (std::bad_any_cast& e) {
      throw std::logic_error("ParameterManager: Parameter type mismatch (" +
                             std::to_string(param_id) + ")\n" + e.what());
    }
  }

 private:
  Scapegoat bst_;
};

#endif
