// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
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

#include "m6/script_engine.hpp"
#include "m6/tokenizer.hpp"
#include "utilities/string_utilities.hpp"

namespace m6tests {

using namespace m6;

class CompileErrorTest : public ::testing::Test {
 protected:
  CompileErrorTest() : interpreter(nullptr) {}

  std::string GetErrors(std::string input) {
    EXPECT_NO_THROW(interpreter.Execute(input));
    return interpreter.FlushErrors();
  }

  ScriptEngine interpreter;

  struct ErrCase {
    const char* src;
    std::string expected_msg;
  };
};

TEST_F(CompileErrorTest, TokenizerErrors) {
  static const ErrCase kCases[] = {{"@", R"(
Unknown token
1│ @
   ^
)"},
                                   {"\"unterminated", R"(
Expected '"'
1│ "unterminated
                ^
)"},
                                   {"\"line\nbreak", R"(
Expected '"'
1│ "line
        ^
)"},
                                   {"\"escape at end\\", R"(
Expected '"'
1│ "escape at end\
                  ^
)"},
                                   {"\"valid\" @ \"again", R"(
Unknown token
1│ "valid" @ "again
           ^       
Expected '"'
1│ "valid" @ "again
                   ^
)"},
                                   // Character literal is entirely unrecognized
                                   {"'c'", R"(
Unknown token
1│ 'c'
   ^  
Unknown token
1│ 'c'
     ^
)"},

                                   {"2147483648", R"(
Integer literal is too large.
1│ 2147483648
   ^^^^^^^^^^
)"},
                                   {"0o678a", R"(
Invalid digit.
1│ 0o678a
       ^ 
Invalid digit.
1│ 0o678a
        ^
)"}};

  for (const auto& c : kCases) {
    std::string error = trim_cp(GetErrors(c.src));
    std::string expected = trim_cp(c.expected_msg);
    EXPECT_EQ(error, expected) << "source:\n"
                               << c.src << '\n'
                               << "diagnostic:\n"
                               << error << '\n'
                               << "expected:\n"
                               << expected;
  }
}

TEST_F(CompileErrorTest, ParserErrors) {
  static const ErrCase kCases[] = {// if / while
                                   {"if 1) 0;", R"(
expected '(' after if
1│ if 1) 0;
     ^
)"},
                                   {"if (1 { }", R"(
expected ')'
1│ if (1 { }
        ^
)"},
                                   {"while 1) 0;", R"(
expected '(' after while
1│ while 1) 0;
        ^
)"},

                                   // for‑loop trio
                                   {"for i=0; i<10; i+=1) foo();", R"(
expected '(' after for
1│ for i=0; i<10; i+=1) foo();
      ^
)"},
                                   {"for (i=0 i<10; i+=1) foo();", R"(
expected ';' after for‑init
1│ for (i=0 i<10; i+=1) foo();
           ^
)"},
                                   {"for (; i<10 i+=1) foo();", R"(
Expected ';' after for‑cond.
1│ for (; i<10 i+=1) foo();
              ^
)"},
                                   {"for (; ; i+=1 foo();", R"(
Expected ')' after for‑inc.
1│ for (; ; i+=1 foo();
                ^
)"},

                                   // postfix productions
                                   {"foo(1, 2;", R"(
Expected ')' after function call.
1│ foo(1, 2;
           ^
)"},
                                   {"x.;", R"(
expected identifier after '.'
1│ x.;
     ^
)"},
                                   {"arr[1;", R"(
Expected ']' after subscript.
1│ arr[1;
        ^
)"},

                                   // missing semicolon
                                   {"x = 1", R"(
Expected ';'.
1│ x = 1
        ^
)"},

                                   // bad assignment target
                                   {"(x + 1) = 2;",
                                    R"(
left‑hand side of assignment must be an identifier
1│ (x + 1) = 2;
   ^^^^^^^
)"},
                                   {"(a+b)+=c;", R"(
left‑hand side of assignment must be an identifier
1│ (a+b)+=c;
   ^^^^^
)"},

                                   // primary‑expr failures
                                   {".;", R"(
expected primary expression
1│ .;
   ^
)"},
                                   {"d/ /e;", R"(
expected primary expression
1│ d/ /e;
      ^
)"},

                                   // missing ')'
                                   {"(1 + 2;", R"(
missing ')' in expression
1│ (1 + 2;
         ^
)"},
                                   {"a(b,c;", R"(
Expected ')' after function call.
1│ a(b,c;
        ^
)"},

                                   // missing ']'
                                   {"a[123+456;", R"(
Expected ']' after subscript.
1│ a[123+456;
            ^
)"},

                                   // missing identifier after '.'
                                   {"a. ;", R"(
expected identifier after '.'
1│ a. ;
      ^
)"}};

  for (const auto& c : kCases) {
    std::string error = trim_cp(GetErrors(c.src));
    std::string expected = trim_cp(c.expected_msg);
    EXPECT_EQ(error, expected) << "source:\n"
                               << c.src << '\n'
                               << "diagnostic:\n"
                               << error << '\n'
                               << "expected:\n"
                               << expected;
  }
}

}  // namespace m6tests
