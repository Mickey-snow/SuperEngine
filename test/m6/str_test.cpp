// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2006, 2007 Elliot Glaysher
// Copyright (C) 2025 Serina Sakurai
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

#include <gtest/gtest.h>

#include "util.hpp"

#include "m6/evaluator.hpp"
#include "m6/parser.hpp"
#include "m6/symbol_table.hpp"
#include "m6/tokenizer.hpp"
#include "m6/value_internal/function.hpp"

#include <utf8.h>

#include <cstring>
#include <iomanip>
#include <sstream>

using namespace m6;

namespace Libstr {
// Counts the number of code points in a utf8 string.
size_t strcplen(std::string_view sv) {
  return utf8::distance(sv.cbegin(), sv.cend());
}

// Impelmentation for the ASCII versions of itoa. Sets |length| |fill|
// characters.
std::string rl_itoa(int number, int length, char fill) {
  std::ostringstream ss;
  if (number < 0)
    ss << "-";
  if (length > 0)
    ss << std::setw(length);
  ss << std::right << std::setfill(fill) << abs(number);
  return ss.str();
}

void strcpy(std::string* dst, std::string* src, std::optional<int> cnt) {
  if (cnt.has_value())
    *dst = src->substr(0, cnt.value());
  else
    *dst = *src;
}

void strcat(std::string* dst, std::string append) {
  (*dst) += std::move(append);
}

int strlen(std::string* s) { return s->length(); }

// Returns the number of characters (as opposed to bytes) in a string. This
// function deals with Shift_JIS characters properly.
int strcharlen(std::string* s) { return strcplen(*s); }

void strclear(std::vector<std::string*> arr) {
  for (auto it : arr)
    it->clear();
}

int strcmp(std::string* lhs, std::string* rhs) {
  return std::strcmp(lhs->c_str(), rhs->c_str());
}

std::string strcpsub(std::string* s, int offset, std::optional<int> length) {
  auto first = s->cbegin(), last = s->cend();
  utf8::advance(first, offset, s->cend());
  if (length.has_value()) {
    try {
      last = first;
      utf8::advance(last, *length, s->cend());
    } catch (utf8::not_enough_room& e) {
      last = s->cend();
    }
  }

  return std::string(first, last);
}

std::string strcprsub(std::string* source,
                      int offsetFromBack,
                      std::optional<int> length) {
  if (length > offsetFromBack) {
    throw std::runtime_error(
        "strrsub: length of substring greater then offset.");
  }

  int offset = strcplen(*source) - offsetFromBack;
  return strcpsub(source, offset, length);
}

// Truncates string such that its length does not exceed len characters.
std::string strcptrunc(std::string* s, int len) {
  try {
    auto end = s->cbegin();
    utf8::advance(end, len, s->cend());
    return std::string(s->cbegin(), end);
  } catch (utf8::not_enough_room& e) {
    return *s;
  }
}

std::string toupper(std::string* s) {
  std::string result;
  std::transform(s->cbegin(), s->cend(), std::back_inserter(result),
                 [](auto x) { return std::toupper(x); });
  return result;
}

std::string tolower(std::string* s) {
  std::string result;
  std::transform(s->cbegin(), s->cend(), std::back_inserter(result),
                 [](auto x) { return std::tolower(x); });
  return result;
}

}  // namespace Libstr

void LoadLibstr(std::shared_ptr<SymbolTable> symtab) {
  auto bind = [symtab](std::string name, auto&& fn) {
    symtab->Set(name, make_fn_value(name, std::forward<decltype(fn)>(fn)));
  };

  bind("strcpy", Libstr::strcpy);
  bind("strlen", Libstr::strlen);
  bind("strcharlen", Libstr::strcharlen);
  bind("strclear", Libstr::strclear);
  bind("strcat", Libstr::strcat);
  bind("strsub", Libstr::strcpsub);
  bind("strrsub", Libstr::strcprsub);
  bind("strtrunc", Libstr::strcptrunc);
  bind("uppercase", Libstr::toupper);
  bind("lowercase", Libstr::tolower);
}

class StrTest : public ::testing::Test {
 protected:
  auto Eval(const std::string_view input) {
    Tokenizer tokenizer(input);
    auto expr = ParseExpression(std::span(tokenizer.parsed_tok_));
    return expr->Apply(*evaluator);
  }

  void SetUp() override {
    symtab = std::make_shared<SymbolTable>();
    evaluator = std::make_shared<Evaluator>(symtab);

    LoadLibstr(symtab);

    Eval(R"( s0 = "" )");
    Eval(R"( s1 = "hello " )");
    Eval(R"( s2 = "わたしの名前、まだ覚えてる？" )");
  }

  std::shared_ptr<SymbolTable> symtab;
  std::shared_ptr<Evaluator> evaluator;
};

TEST_F(StrTest, CompoundAssignment) {
  Eval("s1 = \"hello\"");
  Eval("s2 = s1 + \" \" ");
  Eval("s1 += \", world\"");
  Eval("s2 *= 3");

  EXPECT_VALUE_EQ(symtab->Get("s1"), "hello, world");
  EXPECT_VALUE_EQ(symtab->Get("s2"), "hello hello hello ");
}

TEST_F(StrTest, strcpy) {
  Eval(R"( strcpy(s0, "valid", 2) )");
  Eval(R"( strcpy(s2, "valid") )");
  EXPECT_VALUE_EQ(symtab->Get("s0"), "va");
  EXPECT_VALUE_EQ(symtab->Get("s2"), "valid");
}

TEST_F(StrTest, strlen) {
  EXPECT_VALUE_EQ(Eval(R"( strlen(s0) )"), 0);
  EXPECT_VALUE_EQ(Eval(R"( strlen(s1) )"), 6);
  EXPECT_VALUE_EQ(Eval(R"( strcharlen(s2) )"), 14);
}

TEST_F(StrTest, strclear) {
  Eval("strclear(s2)");
  EXPECT_VALUE_EQ(symtab->Get("s2"), "");
}

TEST_F(StrTest, strcat) {
  Eval(R"( strcat(s0, "Furukawa: ") )");
  Eval(R"( strcat(s0, s2) )");
  EXPECT_VALUE_EQ(symtab->Get("s0"), "Furukawa: わたしの名前、まだ覚えてる？");
}

TEST_F(StrTest, strsub) {
  Eval(R"( s4 = strsub(s2, 7) )");
  Eval(R"( s5 = strrsub("valid", 2) )");
  EXPECT_VALUE_EQ(symtab->Get("s4"), "まだ覚えてる？");
  EXPECT_VALUE_EQ(symtab->Get("s5"), "id");
}

TEST_F(StrTest, strtrunc) {
  Eval(R"( s4 = strtrunc(s1, 5) )");
  Eval(R"( s5 = strtrunc(s1, 1024) )");
  Eval(R"( s6 = strtrunc("wわたしの名前", 3) )");
  EXPECT_VALUE_EQ(Eval(R"( s4+s5+s6 )"), "hellohello wわた");
}

TEST_F(StrTest, caseconv) {
  Eval(R"( s4 = lowercase("Valid") )");
  Eval(R"( s5 = uppercase("Valid") )");
  Eval(R"( s6 = uppercase("wわたしの名前") )");
  EXPECT_VALUE_EQ(Eval(R"( s4+s5+s6 )"), "validVALIDWわたしの名前");
}
