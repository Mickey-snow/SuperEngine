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

#ifndef SRC_LIBREALLIVE_GAMEEXE_H_
#define SRC_LIBREALLIVE_GAMEEXE_H_

#include <boost/iterator/iterator_facade.hpp>
#include <filesystem>

#include <iomanip>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

class Gameexe;

// -----------------------------------------------------------------------

class Token;

// Storage backend for the Gameexe
typedef std::vector<std::shared_ptr<Token>> Gameexe_vec_type;
typedef std::multimap<std::string, Gameexe_vec_type> GameexeData_t;

// -----------------------------------------------------------------------

// Encapsulates a line of the game configuration file that's passed to the
// user. This is a temporary class, which should hopefully be inlined
// away from the target implementation.
//
// This allows us to write code like this:
//
//   vector<string> x = gameexe("WHATEVER", 5).to_strVector();
//   int var = gameexe("EXPLICIT_CAST").ToInt();
//   int var2 = gameexe("IMPLICIT_CAST");
//   gameexe("SOMEVAL") = 5;
//
// This design solves the problem with the old interface, where all
// the default parameters and overloads lead to confusion about
// whether a parameter was part of the key, or was the deafult
// value. Saying that components of the key are part of the operator()
// on Gameexe and that default values are in the casting function in
// GameexeInterpretObject solves this accidental difficulty.
class GameexeInterpretObject {
 public:
  ~GameexeInterpretObject();

 private:
  std::string ToKeyString(const int& value) {
    std::ostringstream ss;
    ss << std::setw(3) << std::setfill('0') << value;
    return ss.str();
  }

  std::string ToKeyString(const std::string& value) { return value; }

  // Connect several key pieces together to form a key string
  template <typename T, typename... Ts>
  std::string MakeKey(T&& first, Ts&&... params) {
    std::string key = ToKeyString(std::forward<T>(first));
    if constexpr (sizeof...(params) > 0)
      key += '.' + MakeKey(std::forward<Ts>(params)...);
    return key;
  }

 public:
  // Extend a key by several key pieces
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

  // Assign a value. Unlike all the other methods, we can safely
  // templatize this since the functions it calls can be overloaded.
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

  // Private; only allow construction by Gameexe and GameexeFilteringiterator
  // Two instantiation methods:
  // 1. By string key: Does not validate key; defers error checking to data
  // accessing
  //    For instance, using Gameexe("IMG") and then accessing with ini("005") is
  //    permitted if "IMG.005" is valid.
  // Note: Iterators are generated on-demand via data->find(key) when accessed
  // by key, which may not align with client expectations due to the multimap
  // nature of GameexeData.
  GameexeInterpretObject(const std::string& key, GameexeData_t* data);
  // 2. By iterator: Directly accesses a multimap entry at the iterator's
  // position.
  //    The iterator must be valid and not at data->end(), with key_ initialized
  //    to it->first.
  GameexeInterpretObject(GameexeData_t::const_iterator it, GameexeData_t* data);
};

// New interface to Gameexe, replacing the one inherited from Haeleth,
// which was hard to use and was very C-ish. This interface's goal is
// to make accessing data in the Gameexe as easy as possible.
class Gameexe {
 public:
  explicit Gameexe(const std::filesystem::path& filename);
  Gameexe();
  ~Gameexe();

  // Parses an individual Gameexe.ini line.
  void parseLine(const std::string& line);

  // When the client code trys to access/modify data, create a
  // GameexeInterpretobject, transfer control of read/write to the object
  template <typename... Ts>
  GameexeInterpretObject operator()(Ts&&... keys) {
    auto it = GameexeInterpretObject("", &data_);
    return it(std::forward<Ts>(keys)...);
  }

  class filtering_iterator;

  // Returns iterators that filter on a possible value.
  filtering_iterator FilterBegin(std::string filter);
  filtering_iterator FilterEnd();

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
  // const filtering iterator
  class filtering_iterator
      : public boost::iterator_facade<filtering_iterator,
                                      GameexeInterpretObject,
                                      boost::forward_traversal_tag,
                                      GameexeInterpretObject> {
   public:
    explicit filtering_iterator(GameexeData_t::const_iterator begin,
                                GameexeData_t::const_iterator end,
                                GameexeData_t* indata)
        : currentIt(begin), endIt(end), data_(indata) {
      if (begin == end)
        currentIt = indata->end();  // range is empty
    }

   private:
    friend class boost::iterator_core_access;

    bool equal(filtering_iterator const& other) const {
      return currentIt == other.currentIt;
    }

    void increment() {
      if (++currentIt == endIt)
        currentIt = data_->end();
    }

    GameexeInterpretObject dereference() const {
      return GameexeInterpretObject(currentIt, data_);
    }

    GameexeData_t::const_iterator currentIt, endIt;
    GameexeData_t* data_;  // We don't own this object
  };

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

using GameexeFilteringIterator = Gameexe::filtering_iterator;

// -----------------------------------------------------------------------

#endif  // SRC_LIBREALLIVE_GAMEEXE_H_
