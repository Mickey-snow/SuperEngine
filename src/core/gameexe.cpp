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

#include "core/gameexe.hpp"

#include <boost/algorithm/string.hpp>
#include <boost/tokenizer.hpp>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace fs = std::filesystem;

// A boost::TokenizerFunction used to extract valid pieces of data
// from the value part of a gameexe key/value pair.
class gameexe_token_extractor {
 private:
  inline bool is_space(char c) {
    return (c == '\r' || c == '\n' || c == ' ' || c == '\t');
  }

  inline bool is_num(char c) { return (c == '-' || (c >= '0' && c <= '9')); }

  inline bool is_data(char c) { return (c == '"' || is_num(c)); }

 public:
  void reset() {}

  template <typename InputIterator, typename Token>
  bool operator()(InputIterator& next, InputIterator end, Token& tok) {
    tok = Token();
    // Advance to the next data character
    for (; next != end && (!is_data(*next)); ++next) {
    }

    if (next == end)
      return false;

    if (*next == '"') {
      tok += '"';
      next++;
      for (; next != end && *next != '"'; ++next)
        tok += *next;
      tok += '"';
      next++;
    } else {
      char lastChar = '\0';

      // Eat the current character and all
      while (next != end) {
        if (*next == '-') {
          // Dashes are ambiguous. They are both seperators and the negative
          // sign and we have to tokenize differently based on what it's
          // doing. If the previous character is a number, we are being used as
          // a range separator.
          if (lastChar >= '0' && lastChar <= '9') {
            // Skip the dash so we don't treat the next number as negative.
            next++;
            break;
          } else {
            // Consume the dash for the next parts.
            tok += *next;
          }
        } else if (is_num(*next)) {
          // All other numbers are consumed here.
          tok += *next;
        } else {
          // We only deal with numbers in this branch.
          break;
        }

        lastChar = *next;
        ++next;
      }
    }

    return true;
  }
};

// -----------------------------------------------------------------------

class Token {
 public:
  ~Token() = default;

  virtual int ToInt() const = 0;
  operator int() const { return ToInt(); }

  virtual std::string ToString() const = 0;
  operator std::string() const { return ToString(); }
};

// -----------------------------------------------------------------------

class IntToken : public Token {
 public:
  IntToken(const int& val) : val_(val) {}

  int ToInt() const override { return val_; }

  std::string ToString() const override { return std::to_string(val_); }

 private:
  int val_;
};

// -----------------------------------------------------------------------

class StringToken : public Token {
 private:
  inline static int stridn = 0;

 public:
  StringToken(std::string val) : val_(std::move(val)), id_(stridn++) {}
  StringToken(int val) : val_(std::to_string(val)), id_(stridn++) {}

  int ToInt() const override { return id_; }

  std::string ToString() const override { return val_; }

 private:
  std::string val_;
  int id_;
};

// -----------------------------------------------------------------------

Gameexe::Gameexe() {}

// -----------------------------------------------------------------------

Gameexe::Gameexe(const fs::path& gameexefile) : data_() {
  std::ifstream ifs(gameexefile);
  if (!ifs) {
    std::ostringstream oss;
    oss << "Could not find Gameexe.ini file! (Looking in " << gameexefile
        << ")";
    throw std::runtime_error(oss.str());
  }

  std::string line;
  while (std::getline(ifs, line)) {
    parseLine(line);
  }
}

Gameexe::~Gameexe() {}

// -----------------------------------------------------------------------

namespace {

bool TryParseIntToken(const std::string& token, int& parsed_value) {
  if (token.empty())
    return false;

  size_t pos = 0;
  try {
    int value = std::stoi(token, &pos, 10);
    if (pos == token.size()) {
      parsed_value = value;
      return true;
    }
  } catch (...) {
    return false;
  }

  return false;
}

}  // namespace

void Gameexe::parseLine(const std::string& line) {
  std::string trimmed = line;
  boost::trim(trimmed);
  if (trimmed.empty())
    return;

  size_t firstEqual = trimmed.find_first_of('=');
  if (firstEqual == std::string::npos)
    return;

  if (!trimmed.empty() && trimmed.front() == '#') {
    // Extract what's the key and value
    std::string key = trimmed.substr(1, firstEqual - 1);
    std::string value = trimmed.substr(firstEqual + 1);

    // Get rid of extra whitespace
    boost::trim(key);
    boost::trim(value);

    Gameexe_vec_type vec;

    // Extract all numeric and data values from the value
    typedef boost::tokenizer<gameexe_token_extractor> ValueTokenizer;
    ValueTokenizer tokenizer(value);
    for (const std::string& tok : tokenizer) {
      if (tok[0] == '"') {  // a string token
        std::string unquoted = tok.substr(1, tok.size() - 2);
        vec.push_back(std::make_shared<StringToken>(unquoted));
      } else if (tok != "-") {  // an int token
        int asint;
        try {
          asint = std::stoi(tok);
        } catch (...) {
          std::cerr << "Couldn't int-ify '" << tok << "'" << std::endl;
          asint = 0;
        }
        vec.push_back(std::make_shared<IntToken>(asint));
      }
    }
    data_.emplace(key, vec);
    return;
  }

  std::string key = trimmed.substr(0, firstEqual);
  std::string value = trimmed.substr(firstEqual + 1);
  boost::trim(key);
  boost::trim(value);

  if (key.empty())
    return;

  Gameexe_vec_type vec;

  if (value.empty()) {
    data_.emplace(key, vec);
    return;
  }

  std::vector<std::string> tokens;
  boost::split(tokens, value, boost::is_any_of(","), boost::token_compress_off);

  for (std::string token : tokens) {
    boost::trim(token);

    if (!token.empty() && token.front() == '"' && token.back() == '"' &&
        token.size() >= 2) {
      token = token.substr(1, token.size() - 2);
      vec.push_back(std::make_shared<StringToken>(token));
      continue;
    }

    int asint;
    if (TryParseIntToken(token, asint)) {
      vec.push_back(std::make_shared<IntToken>(asint));
    } else {
      vec.push_back(std::make_shared<StringToken>(token));
    }
  }

  data_.emplace(key, vec);
}

// -----------------------------------------------------------------------

bool Gameexe::Exists(const std::string& key) {
  return data_.find(key) != data_.end();
}

// -----------------------------------------------------------------------

void Gameexe::SetStringAt(const std::string& key, std::string value) {
  Gameexe_vec_type toStore{std::make_shared<StringToken>(std::move(value))};
  data_.erase(key);
  data_.emplace(key, toStore);
}

// -----------------------------------------------------------------------

void Gameexe::SetIntAt(const std::string& key, int value) {
  Gameexe_vec_type toStore{std::make_shared<IntToken>(value)};
  data_.erase(key);
  data_.emplace(key, toStore);
}

// -----------------------------------------------------------------------

Gameexe::range Gameexe::Filter(const std::string& filter) {
  return range(&data_, filter);
}

// -----------------------------------------------------------------------
// GameexeInterpretObject
// -----------------------------------------------------------------------
GameexeInterpretObject::GameexeInterpretObject(GameexeData_t::const_iterator it,
                                               GameexeData_t* data)
    : key_(it->first), data_(data), iterator_(it) {}

GameexeInterpretObject::GameexeInterpretObject(const std::string& key,
                                               GameexeData_t* data)
    : key_(key), data_(data), iterator_(data->find(key)) {}

// -----------------------------------------------------------------------

GameexeInterpretObject::~GameexeInterpretObject() {}

// -----------------------------------------------------------------------

int GameexeInterpretObject::ToInt(const int defaultValue) const {
  const std::vector<int>& ints = GetIntArray();
  if (ints.size() == 0)
    return defaultValue;

  return ints[0];
}

// -----------------------------------------------------------------------

int GameexeInterpretObject::ToInt() const {
  const std::vector<int>& ints = GetIntArray();
  if (ints.size() == 0)
    ThrowUnknownKey(key_);

  return ints[0];
}

// -----------------------------------------------------------------------

int GameexeInterpretObject::GetIntAt(int index) const {
  if (iterator_ == data_->end())
    ThrowUnknownKey(key_);

  std::shared_ptr<Token> token = iterator_->second.at(index);
  return token->ToInt();
}

// -----------------------------------------------------------------------

std::string GameexeInterpretObject::ToString(
    const std::string& defaultValue) const {
  try {
    return GetStringAt(0);
  } catch (...) {
    return defaultValue;
  }
}

// -----------------------------------------------------------------------

std::string GameexeInterpretObject::ToString() const {
  try {
    return GetStringAt(0);
  } catch (...) {
    ThrowUnknownKey(key_);
  }

  // Shut the -Wall up
  return "";
}

// -----------------------------------------------------------------------

std::string GameexeInterpretObject::GetStringAt(int index) const {
  if (iterator_ == data_->end())
    ThrowUnknownKey("TMP");
  std::shared_ptr<Token> token = iterator_->second.at(index);
  return token->ToString();
}

// -----------------------------------------------------------------------

std::vector<int> GameexeInterpretObject::ToIntVector() const {
  const std::vector<int>& ints = GetIntArray();
  if (ints.size() == 0)
    ThrowUnknownKey(key_);

  return ints;
}

std::vector<std::string> GameexeInterpretObject::ToStrVector() const {
  std::vector<std::string> strs;
  for (const auto& it : iterator_->second)
    strs.emplace_back(it->ToString());
  return strs;
}

// -----------------------------------------------------------------------

bool GameexeInterpretObject::Exists() const {
  return data_->find(key_) != data_->end();
}

// -----------------------------------------------------------------------

std::vector<std::string> GameexeInterpretObject::GetKeyParts() const {
  std::vector<std::string> keyparts;
  boost::split(keyparts, key_, boost::is_any_of("."));
  return keyparts;
}

// -----------------------------------------------------------------------

GameexeInterpretObject& GameexeInterpretObject::operator=(
    const std::string& value) {
  Gameexe_vec_type toStore{std::make_shared<StringToken>(value)};
  data_->erase(key_);
  data_->emplace(key_, toStore);
  return *this;
}

// -----------------------------------------------------------------------

GameexeInterpretObject& GameexeInterpretObject::operator=(const int value) {
  Gameexe_vec_type toStore{std::make_shared<IntToken>(value)};
  data_->erase(key_);
  data_->emplace(key_, toStore);
  return *this;
}

// -----------------------------------------------------------------------

void GameexeInterpretObject::ThrowUnknownKey(const std::string& key) {
  std::ostringstream ss;
  ss << "Unknown Gameexe key '" << key << "'";
  throw std::runtime_error(ss.str());
}

std::vector<int> GameexeInterpretObject::GetIntArray() const {
  static const std::vector<int> falseVector;
  if (iterator_ == data_->end())
    return falseVector;

  std::vector<int> vec;
  vec.reserve(iterator_->second.size());
  for (const auto& tok : iterator_->second)
    vec.push_back(tok->ToInt());
  return vec;
}
