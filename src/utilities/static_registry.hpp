// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2026 Serina Sakurai
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

#include <functional>
#include <map>
#include <stdexcept>
#include <utility>

template <typename Tag,
          typename Key,
          typename Value,
          typename Compare = std::less<Key>>
class StaticRegistry {
 public:
  using key_type = Key;
  using value_type = Value;
  using map_type = std::map<key_type, value_type, Compare>;
  using const_iterator = typename map_type::const_iterator;

  StaticRegistry() = delete;

  static void Register(key_type key, value_type value) {
    auto result =
        GetContext().map_.try_emplace(std::move(key), std::move(value));
    if (!result.second)
      throw std::invalid_argument("Static registry key registered twice.");
  }

  static const value_type* Find(const key_type& key) {
    const auto& map = GetContext().map_;
    auto it = map.find(key);
    if (it == map.cend())
      return nullptr;

    return &it->second;
  }

  static bool Contains(const key_type& key) { return Find(key) != nullptr; }

  static void Reset() { GetContext().map_.clear(); }

  static const_iterator cbegin() { return GetContext().map_.cbegin(); }
  static const_iterator cend() { return GetContext().map_.cend(); }

  static std::size_t size() { return GetContext().map_.size(); }
  static bool empty() { return GetContext().map_.empty(); }

  class Registrar {
   public:
    Registrar(key_type key, value_type value) {
      StaticRegistry::Register(std::move(key), std::move(value));
    }

    ~Registrar() = default;
  };

 private:
  struct Context {
    map_type map_;
  };

  static Context& GetContext() {
    static Context ctx;
    return ctx;
  }
};

#define RLVM_DETAIL_CONCAT_INNER(a, b) a##b
#define RLVM_DETAIL_CONCAT(a, b) RLVM_DETAIL_CONCAT_INNER(a, b)

#define RLVM_REGISTER(registry_type, key, value)                              \
  namespace {                                                                 \
  const registry_type::Registrar RLVM_DETAIL_CONCAT(rlvm_registrar_,          \
                                                    __COUNTER__)(key, value); \
  }
