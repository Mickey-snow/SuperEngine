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

#include "base/expr_ast.hpp"
#include "interpreter/parser.hpp"
#include "interpreter/tokenizer.hpp"

#include <algorithm>

struct TestCase {
  std::string input;
  std::string expected_prefix_str;

  std::string operator()() const { return input; }
};
class ExprastParserIntegrationTest : public ::testing::TestWithParam<TestCase> {
};

TEST_P(ExprastParserIntegrationTest, InfixToPrefix) {
  const auto& test_data = GetParam();

  Tokenizer tokenizer(test_data.input, false);
  EXPECT_NO_THROW(tokenizer.Parse());

  std::vector<Token> tokens;
  std::copy_if(tokenizer.parsed_tok_.cbegin(), tokenizer.parsed_tok_.cend(),
               std::back_inserter(tokens), [](const Token& t) {
                 return !std::holds_alternative<tok::WS>(t);
               });

  std::shared_ptr<ExprAST> ast = nullptr;
  EXPECT_NO_THROW(ast = ParseExpression(std::span(tokens)));
  ASSERT_NE(ast, nullptr);

  static const GetPrefix get_prefix_visitor;
  EXPECT_EQ(ast->Apply(get_prefix_visitor), test_data.expected_prefix_str);
}

INSTANTIATE_TEST_SUITE_P(
    ExprastParserIntegrationInstance,
    ExprastParserIntegrationTest,
    ::testing::Values(
        TestCase{"a + b * (c - d) / e << f && g || h == i != j",
                 "|| && << + a / * b - c d e f g != == h i j"},
        TestCase{"x += y & (z | w) ^ (u << v) >>= t",
                 "+= x >>= ^ & y | z w << u v t"},
        TestCase{"array1[array2[index1 + index2] * (index3 - index4)] = value",
                 "= array1[* array2[+ index1 index2] - index3 index4] value"},
        TestCase{"~a + -b * +c - (d && e) || f",
                 "|| - + ~ a * - b + c && d e f"},
        TestCase{"(a <= b) && (c > d) || (e == f) && (g != h)",
                 "|| && <= a b > c d && == e f != g h"},
        TestCase{"result = a * (b + c) - d / e += f << g",
                 "= result += - * a + b c / d e << f g"},
        TestCase{"data[index1] += (temp - buffer[i] * factor[j]) >> shift",
                 "+= data[index1] >> - temp * buffer[i] factor[j] shift"},
        TestCase{"a + b * c - d / e % f & g | h ^ i << j >> k",
                 "| & - + a * b c % / d e f g ^ h >> << i j k"},
        TestCase{"array[i += 2] *= (k[j -= 3] /= 4) + l",
                 "*= array[+= i 2] + /= k[-= j 3] 4 l"},
        TestCase{"data[array1[index] << 2] = value",
                 "= data[<< array1[index] 2] value"},
        TestCase{"final_result = ((a + b) * (c - d) / e) << (f & g) | (h ^ ~i) "
                 "&& j || k == l != m <= n >= o < p > q",
                 "= final_result || && | << / * + a b - c d e & f g ^ h ~ i j "
                 "!= == k l > < >= <= m n o p q"}));
