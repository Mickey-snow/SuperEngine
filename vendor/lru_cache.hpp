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

#ifndef __lru_cache_hpp__
#define __lru_cache_hpp__

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
 * This cache is thread safe if compiled with the _REENTRANT defined.  It
 * uses the BOOST scientific computing library to provide the thread safety
 * mutexes.
 */
#include <list>
#include <map>
#include <vector>
#ifdef _REENTRANT
#include <boost/thread/mutex.hpp>
#endif

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
template <class Key, class Data>
class LRUCache {
 public:
  typedef std::list<std::pair<Key, Data>> List;  ///< Main cache storage typedef
  typedef typename List::iterator List_Iter;     ///< Main cache iterator
  typedef typename List::const_iterator
      List_cIter;                     ///< Main cache iterator (const)
  typedef std::vector<Key> Key_List;  ///< List of keys
  typedef typename Key_List::iterator Key_List_Iter;  ///< Main cache iterator
  typedef typename Key_List::const_iterator
      Key_List_cIter;                       ///< Main cache iterator (const)
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

#ifdef _REENTRANT
  mutable boost::mutex _mutex;
#endif

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
#ifdef _REENTRANT
    boost::mutex::scoped_lock lock(_mutex);
#endif
    return _list.size();
  }

  /** @brief Gets the current size (in elements) of the cache.
   *  @return size in elements
   */
  unsigned long max_size(void) const { return _max_size; }

  /// Clears all storage and indices.
  void clear(void) {
#ifdef _REENTRANT
    boost::mutex::scoped_lock lock(_mutex);
#endif
    _list.clear();
    _index.clear();
  };

  /** @brief Checks for the existance of a key in the cache.
   *  @param key to check for
   *  @return bool indicating whether or not the key was found.
   */
  bool exists(const Key& key) const {
#ifdef _REENTRANT
    boost::mutex::scoped_lock lock(_mutex);
#endif
    return _index.find(key) != _index.end();
  }

  /** @brief Removes a key-data pair from the cache.
   *  @param key to be removed
   */
  void remove(const Key& key) {
#ifdef _REENTRANT
    boost::mutex::scoped_lock lock(_mutex);
#endif
    Map_Iter miter = _index.find(key);
    if (miter == _index.end())
      return;
    _remove(miter);
  }

  /** @brief Touches a key in the Cache and makes it the most recently used.
   *  @param key to be touched
   */
  void touch(const Key& key) {
#ifdef _REENTRANT
    boost::mutex::scoped_lock lock(_mutex);
#endif
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
#ifdef _REENTRANT
    boost::mutex::scoped_lock lock(_mutex);
#endif
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
#ifdef _REENTRANT
    boost::mutex::scoped_lock lock(_mutex);
#endif
    auto miter = _index.find(key);
    if (miter == _index.end())
      return Data();
    Data tmp = miter->second->second;
    if (should_touch)
      _touch(miter);
    return tmp;
  }

  /** @brief Inserts a key-data pair into the cache and removes entries if
   * neccessary.
   *  @param key object key for insertion
   *  @param data object data for insertion
   *  @note This function checks key existance and touches the key if it already
   * exists.
   */
  void insert(const Key& key, const Data& data) {
#ifdef _REENTRANT
    boost::mutex::scoped_lock lock(_mutex);
#endif
    auto miter = _index.find(key);
    if (miter != _index.end()) {
      _remove(miter);
    }

    _list.push_front(std::make_pair(key, data));
    _index[key] = _list.begin();

    if (_list.size() > _max_size) {
      auto last = _list.end();
      --last;
      _remove(last->first);
    }
  }

  /** @brief Get a list of keys.
                  @return list of the current keys.
  */
  std::vector<Key> get_all_keys(void) const {
#ifdef _REENTRANT
    boost::mutex::scoped_lock lock(_mutex);
#endif
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
   *  @warning miter is now longer usable after being passed to this function.
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

#endif
