// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2004-2006 Patrick Audley
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
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

/***************************************************************************
 *   Copyright (C) 2004-2006 by Patrick Audley                             *
 *   paudley@blackcat.ca                                                   *
 *   http://blackcat.ca                                                    *
 *                                                                         *
 ***************************************************************************/
/**
 * @file lru_cache.hpp Template cache with an LRU removal policy
 * @author Patrick Audley
 * @version 1.1
 * @date March 2006
 * @par
 * This cache uses `ThreadingModel` policy for dealing with threading issues.
 */
#include <concepts>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <optional>

namespace ThreadingModel {
class ILock {
 public:
  virtual ~ILock() = default;
};
using Lock = std::unique_ptr<ILock>;

class UniqueLock : public ILock {
 public:
  UniqueLock(std::unique_lock<std::mutex>&& lock) : _lock(std::move(lock)) {}
  ~UniqueLock() = default;

 private:
  std::unique_lock<std::mutex> _lock;
};

class DummyLock : public ILock {};

class SingleThreaded {
 public:
  SingleThreaded() = default;
  ~SingleThreaded() = default;

  Lock GetLock() { return std::make_unique<DummyLock>(); }
};

class MultiThreaded {
 public:
  MultiThreaded() = default;
  ~MultiThreaded() = default;

  Lock GetLock() {
    return std::make_unique<UniqueLock>(std::unique_lock<std::mutex>(_mutex));
  }

 private:
  std::mutex _mutex;
};
}  // namespace ThreadingModel

/**
 * @brief Template cache with an LRU removal policy.
 * @class LRUCache
 * @author Patrick Audley
 *
 * @par
 * This template creats a simple collection of key-value pairs that grows
 * until the size specified at construction is reached and then begins
 * discard the Least Recently Used element on each insertion.
 *
 */
template <class Key, class Data, class Locker = ThreadingModel::SingleThreaded>
class LRUCache {
 public:
  typedef std::list<std::pair<Key, Data>> List;  ///< Main cache storage typedef
  typedef typename List::iterator List_Iter;     ///< Main cache iterator
  typedef typename List::const_iterator
      List_cIter;                           ///< Main cache iterator (const)
  typedef std::map<Key, List_Iter> Map;     ///< Index typedef
  typedef std::pair<Key, List_Iter> Pair;   ///< Pair of Map elements
  typedef typename Map::iterator Map_Iter;  ///< Index iterator
  typedef typename Map::const_iterator Map_cIter;  ///< Index iterator (const)

 private:
  /// Main cache storage
  List _list;
  /// Cache storage index
  Map _index;

  /// Maximum size of the cache in elements
  unsigned long _max_size;

  mutable Locker _locker;
  using Lock = ThreadingModel::Lock;

 public:
  /** @brief Creates a cache that holds at most Size elements.
   *  @param Size maximum elements for cache to hold
   */
  LRUCache(const unsigned long Size) : _max_size(Size) {}
  /// Destructor - cleans up both index and storage
  ~LRUCache() { this->clear(); }

  /** @brief Gets the current size (in elements) of the cache.
   *  @return size in elements
   */
  unsigned long size(void) const {
    Lock lock = _locker.GetLock();
    return _list.size();
  }

  /** @brief Gets the current size (in elements) of the cache.
   *  @return size in elements
   */
  unsigned long max_size(void) const { return _max_size; }

  /// Clears all storage and indices.
  void clear(void) {
    Lock lock = _locker.GetLock();
    _list.clear();
    _index.clear();
  };

  /** @brief Checks for the existance of a key in the cache.
   *  @param key to check for
   *  @return bool indicating whether or not the key was found.
   */
  bool exists(const Key& key) const {
    Lock lock = _locker.GetLock();
    return _index.find(key) != _index.end();
  }

  /** @brief Removes a key-data pair from the cache.
   *  @param key to be removed
   */
  void remove(const Key& key) {
    Lock lock = _locker.GetLock();
    Map_Iter miter = _index.find(key);
    if (miter == _index.end())
      return;
    _remove(miter);
  }

  /** @brief Touches a key in the Cache and makes it the most recently used.
   *  @param key to be touched
   */
  void touch(const Key& key) {
    Lock lock = _locker.GetLock();
    auto miter = _index.find(key);
    if (miter == _index.end())
      return;
    _touch(miter);
  }

  /** @brief Fetches a pointer to cache data.
   *  @param key to fetch data for
   *  @param touch whether or not to touch the data
   *  @return pointer to data or NULL on error
   */
  Data* fetch_ptr(const Key& key, bool should_touch = true) {
    Lock lock = _locker.GetLock();
    auto miter = _index.find(key);
    if (miter == _index.end())
      return NULL;
    if (should_touch)
      _touch(miter);
    return &(miter->second->second);
  }

  /** @brief Fetches a copy of cached data.
   *  @param key to fetch data for
   *  @param touch_data whether or not to touch the data
   *  @return copy of the data or an empty Data object if not found
   */
  Data fetch(const Key& key, bool should_touch = true) {
    Lock lock = _locker.GetLock();
    auto miter = _index.find(key);
    if (miter == _index.end())
      return Data();
    Data tmp = miter->second->second;
    if (should_touch)
      _touch(miter);
    return tmp;
  }

  /** @brief Fetches a copy of cached data.
   *  @param key to fetch data for
   *  @param default_value default value if not found
   *  @param touch_data whether or not to touch the data
   *  @return copy of the data or the default value
   */
  Data fetch_or(const Key& key,
                const Data& default_value,
                bool should_touch = true) {
    Lock lock = _locker.GetLock();
    auto miter = _index.find(key);
    if (miter == _index.end())
      return default_value;
    Data tmp = miter->second->second;
    if (should_touch)
      _touch(miter);
    return tmp;
  }

  /** @brief Fetches a copy of cached data, using a default value generator if
   * not found.
   *  @param key to fetch data for
   *  @param default_factory invokable to generate default value if data is not
   * found
   *  @param should_touch whether or not to touch the data
   *  @return copy of the data or the default value generated by default_factory
   */
  template <std::invocable<> Fn>
  Data fetch_or_else(const Key& key,
                     Fn&& default_factory,
                     bool should_insert = true,
                     bool should_touch = true)
    requires std::constructible_from<
        Data,
        std::invoke_result_t<decltype(default_factory)>>
  {
    Lock lock = _locker.GetLock();
    auto miter = _index.find(key);
    if (miter == _index.end()) {
      Data default_value(std::invoke(std::forward<Fn>(default_factory)));
      if (should_insert)
        _insert_nolock(key, default_value);
      return default_value;
    }
    Data tmp = miter->second->second;
    if (should_touch)
      _touch(miter);
    return tmp;
  }

  /**
   * @brief Transform a cached value atomically under the cache lock.
   * If the key is present, applies `transform` and returns the updated value.
   * If the key is absent, returns default-constructed Data without inserting.
   *
   * transform can be either:
   *   - void(Data&)    : mutate in place
   *   - Data(Data&)    : return a new value (which will be stored)
   */
  template <std::invocable<Data&> Fn>
  Data fetch_transform(const Key& key,
                       Fn&& transform,
                       bool should_touch = true) {
    Lock lock = _locker.GetLock();
    auto miter = _index.find(key);
    if (miter == _index.end())
      return Data();

    if (should_touch)
      _touch(miter);

    if constexpr (std::is_void_v<std::invoke_result_t<Fn, Data&>>) {
      std::invoke(std::forward<Fn>(transform), miter->second->second);
    } else {
      miter->second->second =
          std::invoke(std::forward<Fn>(transform), miter->second->second);
    }
    return miter->second->second;
  }

  /**
   * @brief Run a continuation on the cached value under the cache lock,
   * returning its result (or indicating success for void).
   *
   * Overload 1 (non-void result): returns std::optional<R>.
   * Overload 2 (void result): returns bool (true if key existed & fn ran).
   */
  template <std::invocable<Data&> Fn>
    requires(!std::is_void_v<std::invoke_result_t<Fn, Data&>>)
  auto fetch_and_then(const Key& key, Fn&& fn, bool should_touch = true)
      -> std::optional<std::invoke_result_t<Fn, Data&>> {
    Lock lock = _locker.GetLock();
    auto miter = _index.find(key);
    if (miter == _index.end())
      return std::nullopt;

    if (should_touch)
      _touch(miter);
    using R = std::invoke_result_t<Fn, Data&>;
    return std::optional<R>(
        std::invoke(std::forward<Fn>(fn), miter->second->second));
  }

  template <std::invocable<Data&> Fn>
    requires(std::is_void_v<std::invoke_result_t<Fn, Data&>>)
  bool fetch_and_then(const Key& key, Fn&& fn, bool should_touch = true) {
    Lock lock = _locker.GetLock();
    auto miter = _index.find(key);
    if (miter == _index.end())
      return false;

    if (should_touch)
      _touch(miter);
    std::invoke(std::forward<Fn>(fn), miter->second->second);
    return true;
  }

  /** @brief Inserts a key-data pair into the cache and removes entries if
   * neccessary.
   *  @param key object key for insertion
   *  @param data object data for insertion
   *  @note This function checks key existance and touches the key if it already
   * exists.
   */
  void insert(const Key& key, const Data& data) {
    Lock lock = _locker.GetLock();
    _insert_nolock(key, data);
  }

 private:
  /// Helper – caller *must* already hold the cache lock.
  void _insert_nolock(const Key& key, const Data& data) {
    auto miter = _index.find(key);
    if (miter != _index.end())
      _remove(miter);

    _list.emplace_front(key, data);
    _index[key] = _list.begin();

    if (_list.size() > _max_size) {
      auto last = std::prev(_list.end());
      _remove(last->first);
    }
  }

 public:
  /** @brief Get a list of keys.
                  @return list of the current keys.
  */
  std::vector<Key> get_all_keys(void) const {
    Lock lock = _locker.GetLock();
    std::vector<Key> keys;
    for (const auto& item : _list) {
      keys.push_back(item.first);
    }
    return keys;
  }

 private:
  /** @brief Internal touch function.
   *  @param iterator to the key to be touched
   */
  void _touch(Map_Iter& miter) {
    _list.splice(_list.begin(), _list, miter->second);
  }

  /** @brief Interal remove function
   *  @param miter Map_Iter that points to the key to remove
   *  @warning miter is no longer usable after being passed to this function.
   */
  void _remove(const Map_Iter& miter) {
    _list.erase(miter->second);
    _index.erase(miter);
  }

  /** @brief Interal remove function
   *  @param key to remove
   */
  void _remove(const Key& key) {
    auto miter = _index.find(key);
    if (miter != _index.end()) {
      _remove(miter);
    }
  }
};
