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

#pragma once

#include <algorithm>
#include <filesystem>
#include <iomanip>
#include <iterator>
#include <map>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "utilities/expected.hpp"

struct GexeErr {
  std::string key;
  std::string message;
  std::optional<std::size_t> line;

  inline bool operator==(const GexeErr& other) const {
    return key == other.key && message == other.message && line == other.line;
  }
};

template <typename T>
using GexeExpected = expected<T, GexeErr>;

class Gameexe;

struct GexeStr {
  std::string value;
  int id = 0;
};

using GexeVal = std::variant<int, GexeStr>;

class GameexeInterpretObject {
 public:
  ~GameexeInterpretObject() = default;

  template <typename... Ts>
  GameexeInterpretObject operator()(Ts&&... nextKeys) {
    std::string new_key = key_;
    if constexpr (sizeof...(nextKeys) > 0) {
      if (!new_key.empty())
        new_key += '.';
      new_key += MakeKey(std::forward<Ts>(nextKeys)...);
    }
    return GameexeInterpretObject(owner_, std::move(new_key));
  }

  int ToInt() const;
  GexeExpected<int> Int() const;
  GexeExpected<int> IntAt(std::size_t index) const;

  std::string ToStr() const;
  GexeExpected<std::string> Str() const;
  GexeExpected<std::string> StrAt(std::size_t index) const;

  GexeExpected<std::vector<int>> IntVec() const;
  std::vector<int> ToIntVec() const;
  GexeExpected<std::vector<std::string>> StrVec() const;
  std::vector<std::string> ToStrVec() const;

  bool Exists() const;
  inline const std::string& key() const { return key_; }

  std::vector<std::string> GetKeyParts() const;

  GameexeInterpretObject& operator=(std::string value);
  GameexeInterpretObject& operator=(int value);

  friend std::ostream& operator<<(std::ostream& os,
                                  GameexeInterpretObject const& io);

 private:
  friend class Gameexe;

  GameexeInterpretObject(Gameexe* owner, std::string key);
  GameexeInterpretObject(Gameexe* owner,
                         std::string key,
                         std::vector<GexeVal>* direct_values);

  GexeExpected<const std::vector<GexeVal>*> FetchValues() const;
  std::vector<GexeVal>* EnsureMutableValues();

  std::string ToKeyString(int value) {
    std::ostringstream ss;
    ss << std::setw(3) << std::setfill('0') << value;
    return ss.str();
  }

  std::string ToKeyString(const std::string& value) { return value; }

  template <typename T, typename... Ts>
  std::string MakeKey(T&& first, Ts&&... params) {
    std::string key = ToKeyString(std::forward<T>(first));
    if constexpr (sizeof...(params) > 0)
      key += '.' + MakeKey(std::forward<Ts>(params)...);
    return key;
  }

  Gameexe* owner_ = nullptr;
  std::string key_;
  std::vector<GexeVal>* direct_values_ = nullptr;
};

class Gameexe {
 public:
  using Storage = std::multimap<std::string, std::vector<GexeVal>>;

  Gameexe();

  static GexeExpected<Gameexe> FromFile(const std::filesystem::path& path);
  GexeExpected<void> LoadFromFile(const std::filesystem::path& path);

  template <typename... Ts>
  GameexeInterpretObject operator()(Ts&&... keys) {
    GameexeInterpretObject root(this, "");
    return root(std::forward<Ts>(keys)...);
  }

  bool Exists(std::string_view key) const;
  std::size_t Size() const { return entries_.size(); }

  class FilterRange;
  FilterRange Filter(std::string prefix);

  void SetStringAt(std::string_view key, std::string value);
  void SetIntAt(std::string_view key, int value);

  inline void parseLine(const std::string& line) { (void)ParseLine(line, 0); }

 private:
  friend class GameexeInterpretObject;
  friend class Gameexe::FilterRange;

  GexeExpected<void> ParseLine(std::string_view line, std::size_t line_number);

  GexeExpected<std::vector<GexeVal>> ParseValues(std::string_view key,
                                                 std::string_view value,
                                                 std::size_t line_number);

  const std::vector<GexeVal>* Find(std::string_view key) const;
  std::vector<GexeVal>* Find(std::string_view key);
  std::vector<GexeVal>& Ensure(std::string key);

  GexeVal MakeIntValue(int value) const;
  GexeVal MakeStringValue(std::string value);

 public:
  class FilterRange {
   public:
    class iterator {
     public:
      using iterator_category = std::forward_iterator_tag;
      using value_type = GameexeInterpretObject;
      using difference_type = std::ptrdiff_t;

      iterator(Gameexe* owner,
               Storage::iterator current,
               Storage::iterator end,
               std::string prefix);

      GameexeInterpretObject operator*() const;
      iterator& operator++();
      bool operator==(const iterator& other) const;
      bool operator!=(const iterator& other) const { return !(*this == other); }

     private:
      void AdvanceToMatch();

      Gameexe* owner_ = nullptr;
      Storage::iterator current_;
      Storage::iterator end_;
      std::string prefix_;
    };

    FilterRange(Gameexe* owner, std::string prefix);

    iterator begin();
    iterator end();

   private:
    Gameexe* owner_;
    std::string prefix_;
  };

 private:
  Storage entries_;
  int next_string_id_ = 0;
};
