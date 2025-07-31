// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2008 Elliot Glaysher
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

#include <algorithm>
#include <cstdint>
#include <functional>
#include <ranges>
#include <string>

// Converts a CP932/Shift_JIS string into a wstring with Unicode
// characters.
//
// If the string was not CP932, but actually another encoding
// transformed such that it can be processed as CP932, this additional
// transformation should be reversed here.  This technique is how
// non-Japanese text is used in RealLive.  Transformations that might
// potentially be encountered are:
//
//   0 - plain CP932 (no transformation)
//   1 - CP936
//   2 - CP1252 (also requires rlBabel support)
//   3 - CP949
//
// These are the transformations applied by RLdev, and translation
// tables can be found in the rlBabel source code.  There are also at
// least two CP936 transformations used by the Key Fans Club and
// possibly one or more other CP949 transformations, but details of
// these are not publicly available.
std::wstring cp932toUnicode(const std::string& line, int transformation);

// String representation of the transformation name.
std::string TransformationName(int transformation);

// Converts a UTF-16 string to a UTF-8 one.
std::string UnicodeToUTF8(const std::wstring& widestring);

// For convenience. Combines the two above functions.
std::string cp932toUTF8(const std::string& line, int transformation);

// Returns true if codepoint is either of the Japanese quote marks or '('.
bool IsOpeningQuoteMark(int codepoint);

// Returns whether |codepoint| is part of a word that should be wrapped if we
// go over the right margin.
bool IsWrappingRomanCharacter(int codepoint);

// Returns whether the unicode |codepoint| is a piece of breaking punctuation.
bool IsKinsoku(int codepoint);

// Returns the unicode codpoint for the next UTF-8 character in |c|.
int Codepoint(const std::string& c);

// Checks to see if the byte c is the first byte of a two byte
// character. RealLive encodes its strings in Shift_JIS, so we have to
// deal with this character encoding.
inline bool shiftjis_lead_byte(const char c) {
  return (c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xfc);
}

// Advanced the Shift_JIS character string c by one char.
void AdvanceOneShiftJISChar(const char*& c);

// Copies a single Shift_JIS character into output and advances the string.
void CopyOneShiftJisCharacter(const char*& str, std::string& output);

// If there is currently a two byte fullwidth Latin capital letter at the
// current point in str (a shift_jis string), then convert it to ASCII, copy it
// to output and return true. Returns false otherwise.
bool ReadFullwidthLatinLetter(const char*& str, std::string& output);

// Adds a character to the string |output|. Two byte characters are encoded in
// an uint16_t.
void AddShiftJISChar(uint16_t c, std::string& output);

// Feeds each two consecutive pair of characters to |fun|.
void PrintTextToFunction(
    std::function<bool(const std::string& c, const std::string& nextChar)> fun,
    const std::string& charsToPrint,
    const std::string& nextCharForFinal);

// Removes quotes from the beginning and end of the string.
std::string RemoveQuotes(const std::string& quotedString);

/**
 * @brief Converts a base-26 letter string to its corresponding integer value.
 *
 * This function interprets a string composed of one or more uppercase letters
 * ('A' to 'Z') as a number in a base-26 system and converts it to
 * its integer equivalent.
 *
 * Each character in the string represents a digit in base-26:
 * - 'A' corresponds to 0
 * - 'B' corresponds to 1
 * - ...
 * - 'Z' corresponds to 25
 *
 * @param value The input string representing a base-26 number. Must consist
 *              of one or more uppercase letters ('A' to 'Z') only.
 *
 * @return The integer equivalent of the base-26 number.
 *
 * @throws std::invalid_argument If the input string is empty, contains
 *                                characters outside the range 'A' to 'Z',
 *                                or has an invalid format.
 * @throws std::overflow_error   If the resulting integer exceeds the maximum
 *                                value for a 32-bit signed integer.
 *
 * @example
 * @code
 * int num1 = ConvertLetterIndexToInt("A");    // Returns 0
 * int num2 = ConvertLetterIndexToInt("Z");    // Returns 25
 * int num3 = ConvertLetterIndexToInt("AB");   // Returns 27
 * @endcode
 */
int ConvertLetterIndexToInt(const std::string& value);

template <std::ranges::input_range R>
  requires std::convertible_to<std::ranges::range_value_t<R>, std::string_view>
std::string Join(std::string_view sep, R&& range) {
  std::string result;
  bool first = true;

  for (auto&& str : range) {
    if (first)
      first = false;
    else
      result += sep;
    result += std::string_view(str);
  }

  return result;
}

template <typename R>
inline auto view_to_string(R const& cont) {
  return std::views::all(cont) |
         std::views::transform([](const auto& x) { return std::to_string(x); });
}

inline void ltrim(std::string& s) {
  s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
            return !std::isspace(ch);
          }));
}
inline void rtrim(std::string& s) {
  s.erase(std::find_if(s.rbegin(), s.rend(),
                       [](unsigned char ch) { return !std::isspace(ch); })
              .base(),
          s.end());
}
inline void trim(std::string& s) {
  rtrim(s);
  ltrim(s);
}
inline std::string trim_cp(std::string s) {
  trim(s);
  return s;
}

inline std::string_view ltrim_sv(std::string_view s) {
  auto it = std::find_if(s.begin(), s.end(),
                         [](unsigned char ch) { return !std::isspace(ch); });
  return s.substr(std::distance(s.begin(), it));
}

inline std::string_view rtrim_sv(std::string_view s) {
  auto it = std::find_if(s.rbegin(), s.rend(),
                         [](unsigned char ch) { return !std::isspace(ch); });
  // base() is the first non-space from the end; distance to begin gives new
  // length
  return s.substr(0, std::distance(s.begin(), it.base()));
}

inline std::string_view trim_sv(std::string_view s) {
  return ltrim_sv(rtrim_sv(s));
}
