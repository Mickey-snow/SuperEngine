// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of libreallive, a dependency of RLVM.
//
// -----------------------------------------------------------------------
//
// Copyright (c) 2006, 2007 Peter Jolly
// Copyright (c) 2007 Elliot Glaysher
// Copyright (c) 2024 Serina Sakurai
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// -----------------------------------------------------------------------

#ifndef SRC_BASE_GAMEEXE_HPP_
#define SRC_BASE_GAMEEXE_HPP_

#include <boost/iterator/iterator_facade.hpp>
#include <filesystem>

#include <iomanip>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

class Gameexe;
class Token;

// Storage backend for the Gameexe
using Gameexe_vec_type = std::vector<std::shared_ptr<Token>>;
using GameexeData_t = std::multimap<std::string, Gameexe_vec_type>;

// -----------------------------------------------------------------------

/**
 * @brief Encapsulates a line of the game configuration file that's passed to
 * the user.
 *
 * This is a temporary class, which should hopefully be inlined away from the
 * target implementation.
 *
 * This allows us to write code like this:
 * @code
 * vector<string> x = gameexe("WHATEVER", 5).to_strVector();
 * int var = gameexe("EXPLICIT_CAST").ToInt();
 * int var2 = gameexe("IMPLICIT_CAST");
 * gameexe("SOMEVAL") = 5;
 * @endcode
 *
 * This design solves the problem with the old interface, where all
 * the default parameters and overloads lead to confusion about
 * whether a parameter was part of the key, or was the default
 * value. Saying that components of the key are part of the operator()
 * on Gameexe and that default values are in the casting function in
 * GameexeInterpretObject solves this accidental difficulty.
 */
class GameexeInterpretObject {
 public:
  ~GameexeInterpretObject();

 private:
  /**
   * @brief Converts an integer value to a key string with leading zeros.
   *
   * @param value The integer value to convert.
   * @return The formatted key string.
   */
  std::string ToKeyString(const int& value) {
    std::ostringstream ss;
    ss << std::setw(3) << std::setfill('0') << value;
    return ss.str();
  }

  std::string ToKeyString(const std::string& value) { return value; }

  /**
   * @brief Connects several key pieces together to form a key string.
   *
   * @tparam T The type of the first parameter.
   * @tparam Ts The types of the remaining parameters.
   * @param first The first key piece.
   * @param params The remaining key pieces.
   * @return A concatenated key string.
   */
  template <typename T, typename... Ts>
  std::string MakeKey(T&& first, Ts&&... params) {
    std::string key = ToKeyString(std::forward<T>(first));
    if constexpr (sizeof...(params) > 0)
      key += '.' + MakeKey(std::forward<Ts>(params)...);
    return key;
  }

 public:
  /**
   * @brief Extends the key by appending additional key pieces.
   *
   * @tparam Ts The types of the key pieces.
   * @param nextKeys The key pieces to append.
   * @return A new GameexeInterpretObject with the extended key.
   */
  template <typename... Ts>
  GameexeInterpretObject operator()(Ts&&... nextKeys) {
    std::string newkey = key_;
    if constexpr (sizeof...(nextKeys) > 0) {
      if (!key_.empty())
        newkey += '.';
      newkey += MakeKey(std::forward<Ts>(nextKeys)...);
    }
    return GameexeInterpretObject(newkey, data_);
  }

  // Finds an int value, returning a default if non-existant.
  int ToInt(const int defaultValue) const;

  // Finds an int value, throwing if non-existant.
  int ToInt() const;

  // Allow implicit casts to int with no default value
  operator int() const { return ToInt(); }

  // Returns a specific piece of data at index as an int
  int GetIntAt(int index) const;

  // Finds a string value, throwing if non-existant.
  std::string ToString(const std::string& defaultValue) const;

  // Finds a string value, throwing if non-existant.
  std::string ToString() const;

  // Allow implicit casts to string
  operator std::string() const { return ToString(); }

  // Returns a piece of data at a certain location as a string.
  std::string GetStringAt(int index) const;

  // Finds a vector of ints, throwing if non-existant.
  std::vector<int> ToIntVector() const;

  operator std::vector<int>() const { return ToIntVector(); }

  // Checks to see if the key exists.
  bool Exists() const;

  const std::string& key() const { return key_; }

  // Returns the key splitted on periods.
  std::vector<std::string> GetKeyParts() const;

  GameexeInterpretObject& operator=(const std::string& value);
  GameexeInterpretObject& operator=(const int value);

 private:
  // We expose our private constructor to Gameexe and GameexeFilteringiterator
  friend class Gameexe;

  std::vector<int> GetIntArray() const;

  static void ThrowUnknownKey(const std::string& key);

  std::string key_;
  GameexeData_t* data_;  // We don't own this object
  GameexeData_t::const_iterator iterator_;

  /**
   * @brief Constructs a GameexeInterpretObject with the specified key.
   *
   * Private constructor; only accessible by Gameexe and
   * GameexeFilteringIterator.
   *
   * Does not validate the key; defers error checking to data accessing.
   * For example, using Gameexe("IMG") and then accessing with ini("005") is
   * permitted if "IMG.005" is valid.
   *
   * Note: Iterators are generated on-demand via data->find(key) when accessed
   * by key, which may not align with client expectations due to the multimap
   * nature of GameexeData.
   *
   * @param key The key string.
   * @param data Pointer to the Gameexe data.
   */
  GameexeInterpretObject(const std::string& key, GameexeData_t* data);

  /**
   * @brief Constructs a GameexeInterpretObject with the specified iterator.
   *
   * Private constructor; only accessible by Gameexe and
   * GameexeFilteringIterator.
   *
   * Directly accesses a multimap entry at the iterator's position.
   * The iterator must be valid and not at data->end(), with key_ initialized
   * to it->first.
   *
   * @param it The iterator pointing to the multimap entry.
   * @param data Pointer to the Gameexe data.
   */
  GameexeInterpretObject(GameexeData_t::const_iterator it, GameexeData_t* data);
};

class Gameexe {
 public:
  explicit Gameexe(const std::filesystem::path& filename);
  Gameexe();
  ~Gameexe();

  // Parses an individual Gameexe.ini line.
  void parseLine(const std::string& line);

  /**
   * @brief Accesses or modifies data associated with the given keys.
   *
   * When the client code tries to access or modify data, this function creates
   * a GameexeInterpretObject and transfers control of read/write to the object.
   *
   * @tparam Ts Types of the keys.
   * @param keys The keys to access.
   * @return A GameexeInterpretObject for the specified keys.
   */
  template <typename... Ts>
  GameexeInterpretObject operator()(Ts&&... keys) {
    auto it = GameexeInterpretObject("", &data_);
    return it(std::forward<Ts>(keys)...);
  }

  class range;
  range Filter(const std::string& filter);

  // Returns whether key exists in the stored data
  bool Exists(const std::string& key);

  // Returns the number of keys in the Gameexe.ini file.
  size_t Size() const { return data_.size(); }

  // Exposed for testing.
  void SetStringAt(const std::string& key, const std::string& value);
  void SetIntAt(const std::string& key, const int value);

 private:
  GameexeData_t data_;

 public:
  // const iterator
  class iterator : public boost::iterator_facade<iterator,
                                                 GameexeInterpretObject,
                                                 boost::forward_traversal_tag,
                                                 GameexeInterpretObject> {
   public:
    iterator(GameexeData_t::const_iterator it, GameexeData_t* indata)
        : it_(it), data_(indata) {}

   private:
    friend class boost::iterator_core_access;

    bool equal(iterator const& other) const { return it_ == other.it_; }

    void increment() { ++it_; }

    GameexeInterpretObject dereference() const {
      return GameexeInterpretObject(it_, data_);
    }

   private:
    GameexeData_t::const_iterator it_;
    GameexeData_t* data_;
  };

  // range that can be iterate through
  class range {
   public:
    range(GameexeData_t* indata, const std::string& inkey)
        : data_(indata), key_(inkey) {}

    iterator begin() const { return iterator(data_->lower_bound(key_), data_); }
    iterator end() const {
      std::string upper_key = key_;
      GameexeData_t::const_iterator end_it;
      if (!upper_key.empty()) {
        upper_key.back()++;
        end_it = data_->lower_bound(upper_key);
      } else
        end_it = data_->cend();
      return iterator(end_it, data_);
    }

   private:
    GameexeData_t* data_;
    std::string key_;
  };
};

#endif
