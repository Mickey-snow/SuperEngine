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

#include "m6/compiler_pipeline.hpp"
#include "m6/source_buffer.hpp"
#include "m6/vm_factory.hpp"
#include "utilities/string_utilities.hpp"
#include "vm/disassembler.hpp"
#include "vm/object.hpp"
#include "vm/value.hpp"

#include <filesystem>
#include <format>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>

namespace m6test {

using namespace m6;
using namespace serilang;
namespace fs = std::filesystem;

class CompilerTest : public ::testing::Test {
 public:
  // Holds everything we care about after one script run
  struct ExecutionResult {
    serilang::Value last;  ///< value left on the VM stack (result)
    std::string stdout;    ///< text produced on std::cout
    std::string stderr;    ///< text produced on std::cerr
    std::string disasm;    ///< human readable disassembly

    bool operator==(std::string_view rhs) const {
      return stderr.empty() && trim_cp(stdout) == trim_sv(rhs);
    }

    friend std::ostream& operator<<(std::ostream& os,
                                    const ExecutionResult& res) {
      if (!res.stderr.empty())
        os << "\nErrors:\n" << res.stderr;
      os << "\nOutput:\n" << res.stdout;
      os << "\nDisassembly:\n" << res.disasm;
      return os;
    }
  };

  std::stringstream inBuf;
  std::shared_ptr<GarbageCollector> gc = std::make_shared<GarbageCollector>();

  // Compile + run `source`.
  [[nodiscard]] ExecutionResult Run(std::string source) {
    std::stringstream outBuf, errBuf;
    ExecutionResult r;

    // ── compile ────────────────────────────────────────────────────
    auto vm = m6::VMFactory::Create(gc, outBuf, inBuf, errBuf);
    m6::CompilerPipeline pipe(gc, false);
    auto sb = SourceBuffer::Create(std::move(source), "<CompilerTest>");
    pipe.compile(sb);

    // any compile-time diagnostics?
    if (!pipe.Ok()) {
      r.stderr += pipe.FormatErrors();
      return r;
    }

    auto chunk = pipe.Get();
    if (!chunk) {
      r.stderr += "internal: pipeline returned null chunk\n";
      return r;
    }
    r.disasm = Disassembler().Dump(*chunk);

    // ── run ────────────────────────────────────────────────────────
    try {
      r.last = vm.Evaluate(chunk);
    } catch (std::exception const& ex) {
      errBuf << ex.what();
    }

    r.stdout += outBuf.str();
    r.stderr += errBuf.str();
    return r;
  }
  [[nodiscard]] ExecutionResult Interpret(
      std::initializer_list<std::string> src) {
    std::stringstream outBuf, errBuf;
    ExecutionResult r;
    auto vm = m6::VMFactory::Create(gc, outBuf, inBuf, errBuf);

    for (const auto& line : src) {
      m6::CompilerPipeline pipe(gc, /*repl=*/true);
      auto sb = SourceBuffer::Create(line, "<Compiler Test>");
      pipe.compile(sb);

      if (!pipe.Ok()) {
        r.stderr += pipe.FormatErrors();
        continue;
      }

      auto chunk = pipe.Get();
      if (!chunk) {
        r.stderr +=
            "internal: pipeline returned null chunk, when compiling " + line;
        continue;
      }

      r.disasm += Disassembler().Dump(*chunk);

      EXPECT_NO_THROW(r.last = vm.Evaluate(chunk));
    }

    r.stdout += outBuf.str();
    r.stderr += errBuf.str();
    return r;
  }
};

TEST_F(CompilerTest, ConstantArithmetic) {
  auto res = Run(R"( print([1+2, 2**3, 1-3], end=""); )");
  ASSERT_TRUE(res.stderr.empty()) << res.stderr;
  EXPECT_EQ(res.stdout, "[3,8,-2]") << "\nDisassembly:\n" << res.disasm;
}

TEST_F(CompilerTest, GlobalVariable) {
  auto res = Run("a = 10;\n print(a + 5);");
  ASSERT_TRUE(res.stderr.empty()) << res.stderr;
  EXPECT_EQ(res.stdout, "15\n") << "\nDisassembly:\n" << res.disasm;
}

TEST_F(CompilerTest, MultipleStatements) {
  auto res = Run("x = 4;\n y = 6;\n print(x * y);");
  ASSERT_TRUE(res.stderr.empty()) << res.stderr;
  EXPECT_EQ(res.stdout, "24\n") << "\nDisassembly:\n" << res.disasm;
}

TEST_F(CompilerTest, List) {
  {
    auto res = Run("x=1; a=[x,x+1,x*3]; print(a);");
    EXPECT_EQ(res, "[1,2,3]\n");
  }
  {
    auto res = Run("a=[1,2]; a[0]=2; a[1]=3; print(a);");
    EXPECT_EQ(res, "[2,3]\n");
  }
}

TEST_F(CompilerTest, Dict) {
  {
    auto res = Run(R"( x=1; a={"1":x,"2":x+1,"3":1+x*2}; print(a); )");
    EXPECT_EQ(res, "{2:2,3:3,1:1}\n");
  }
  {
    auto res = Run(R"( a={"1":1}; a["1"]+=1; print(a); )");
    EXPECT_EQ(res, "{1:2}\n");
  }
}

TEST_F(CompilerTest, If) {
  auto res = Run(R"(
result = "none";
one = 1;
two = 2;
if(one < two){
  if(one+1 < two) result = "first";
  else { result = one + two; }
} else result = "els";
print(result);
)");

  ASSERT_TRUE(res.stderr.empty()) << res.stderr;
  EXPECT_EQ(res.stdout, "3\n") << "\nDisassembly:\n" << res.disasm;
}

TEST_F(CompilerTest, VariableScope) {
  auto res = Run(R"(
fn foo(){
  a = 1;
  b = a + a;
  global a;
  result = a + b;
  a = b;
  return result;
}
a = 12;
print(foo(), a);
)");

  ASSERT_TRUE(res.stderr.empty()) << res.stderr;
  EXPECT_EQ(res.stdout, "14 2\n") << "\nDisassembly:\n" << res.disasm;
}

TEST_F(CompilerTest, While) {
  auto res = Run(R"(
sum = 0;
i = 1;
while(i <= 10){ sum+=i; i+=1; }
print(sum);
)");

  ASSERT_TRUE(res.stderr.empty()) << res.stderr;
  EXPECT_EQ(res.stdout, "55\n") << "\nDisassembly:\n" << res.disasm;
}

TEST_F(CompilerTest, For) {
  auto res = Run(R"(
fact = 1;
for(i=1; i<10; i+=1) fact *= i;
print(fact);
)");

  ASSERT_TRUE(res.stderr.empty()) << res.stderr;
  EXPECT_EQ(res.stdout, "362880\n") << "\nDisassembly:\n" << res.disasm;
}

TEST_F(CompilerTest, Function) {
  {
    auto res = Run(R"(
fn print_twice(msg) {
  print(msg);
  print(msg);
}
print_twice("hello");
)");

    EXPECT_EQ(res, "hello\nhello\n");
  }

  {
    auto res = Run(R"(
fn print_time(){ t = time(); print(t); }
print_time();
)");

    auto isNumber = [](std::string s) {
      if (!s.empty() && s.back() == '\n')
        s.pop_back();
      static const std::regex re{R"(^[+-]?\d+(\.\d+)?$)"};
      return std::regex_match(s, re);
    };
    EXPECT_TRUE(isNumber(res.stdout)) << res;
  }

  {
    auto res = Run(R"(
fn return_plus_1(x) { return x+1; }
print(return_plus_1(123));
)");

    EXPECT_EQ(res, "124\n");
  }

  {
    auto res = Run(R"(
fn complex_func(a, b, c=3, *args, d=10, e=5, **kwargs){
  print(a,b,c,d,e,args,kwargs);
}
complex_func(1, 2, 10, 20, 30, 40, 50, extra="foo");
complex_func(10 ,20);
)");

    EXPECT_EQ(res, "1 2 10 20 30 [40,50] {extra:foo}\n10 20 3 10 5 [] {}\n");
  }

  {
    auto res = Run(R"(
fn foo(a=1, b=2, **kwargs){
  print("a =", a);
  print("b =", b);
  print("extra =", kwargs);
}
foo(b=10, a=20);
)");

    EXPECT_EQ(res, "a = 20\nb = 10\nextra = {}\n");
  }

  {
    auto res = Run(R"(
try{ a = 1; a(); }
catch(e){ print(e); }
)");

    EXPECT_EQ(res, "'<int: 1>' object is not callable.");
  }
}

TEST_F(CompilerTest, RecursiveFunction) {
  {
    auto res = Run(R"(
fn fib(n) {
  if (n < 2) return 1;
  return fib(n-1) + fib(n-2);
}
print(fib(10));
)");

    EXPECT_EQ(res, "89\n");
  }
}

TEST_F(CompilerTest, Class) {
  {
    auto res = Run(R"(
class Klass{
  fn foo(){ return 1; }
  fn boo(x,y){ return x+y; }
}

print(Klass);
klass = Klass();
print(klass, klass.foo(), klass.boo(2,3), end="", sep=",");
)");

    EXPECT_EQ(res, "<class Klass>\n<Klass object>,1,5");
  }

  {
    auto res = Run(R"(
class Klass{
  fn foo(self, x){
    self.result += "*" * x + "0";
    if(x > 1) foo(self, x-1);
  }
}

inst = Klass();
inst.result = "";

foo = inst.foo;
foo(5);

print(inst.result);
)");

    EXPECT_EQ(res, "*****0****0***0**0*0\n");
  }

  {
auto res = Run(R"(
class Point2{
  fn __init__(self, x=0, y=0){ self.x=x; self.y=y; }
  fn dist_sq(self){ return self.x*self.x + self.y*self.y; }
}

pt1 = Point2(4,5);
pt2 = Point2(10);
pt3 = Point2();

print(pt1.dist_sq(), pt2.dist_sq(), pt3.dist_sq());
)");

  EXPECT_EQ(res, "41 100 0\n");
  }

  {
    auto res = Run(R"(
class A{}
class B{}
try{ A() + B(); }
catch(e){ print(e); }
)");
    EXPECT_EQ(res,
              "no match for 'operator +' (operand type <A object>,<B object>)");
  }

  {
    auto res = Run(R"(
class A{}
try{ a = A(); b = a.missing; print(b); }
catch(e){ print(e); }
)");
    EXPECT_EQ(res, "'<A object>' object has no member 'missing'");
  }
}

TEST_F(CompilerTest, Coroutine) {
  {
    auto res = Run(R"(
fn foo(){for(i=0;;i+=1) yield i;}
f = spawn foo();
for(i=0;i<5;i+=1)
  print(await f);
)");

    ASSERT_TRUE(res.stderr.empty()) << res.stderr;
    EXPECT_EQ(res.stdout, "0\n1\n2\n3\n4\n") << "\nDisassembly:\n"
                                             << res.disasm;
  }

  {
    auto res = Run(R"(
fn deep(n){
  if(n<=0) return 0;
  return n + await spawn deep(n-1);
}

print(await spawn deep(1000));
)");

    ASSERT_TRUE(res.stderr.empty()) << res.stderr;
    EXPECT_EQ(res.stdout, "500500\n") << "\nDisassembly:\n" << res.disasm;
  }
}

TEST_F(CompilerTest, Import) {
  struct Source {
    fs::path path;
    std::string modname;
    Source(fs::path p, std::string src) : path(p), modname(p.string()) {
      if (path.extension() != ".sr")
        path.replace_filename(path.string() + ".sr");

      std::ofstream ofs(path.string());
      ofs << std::move(src);
    }
    ~Source() { fs::remove(path); }
  };

  // basic imports
  {
    Source srcx("modulex", R"(
val = 123;
fn func(){ return val; }
)");

    auto res = Run(std::format(R"(
import {0};
from {0} import val as v;

val = 999;

print({0}.val);
print({0}.func());
print(v);
)",
                               srcx.modname));

    EXPECT_EQ(res, "123\n123\n123\n");
  }

  // name collisions
  {
    Source srcx("modulex", R"(
val = 123;
)");
    Source srcy("moduley", R"(
modulex = "456";
)");

    auto res = Run(std::format(R"(
import {0};
from {1} import {0};

print({0});
)",
                               srcx.modname, srcy.modname));

    EXPECT_EQ(res, "456\n");
  }

  // circular import
  {
    Source srca("circ_a", R"(
import circ_b;
fn func_a(){ return "A"; }
fn call_b(){ return circ_b.func_b(); }
)");
    Source srcb("circ_b", R"(
import circ_a;
fn func_b(){ return "B"; }
fn call_a(){ return circ_a.func_a(); }
)");

    auto res = Run(R"(
import circ_a as a;
import circ_b as b;
print(a.call_b());
print(b.call_a());
)");

    EXPECT_EQ(res, "B\nA\n");
  }

  {  // import missing name
    Source mod("repl_mod_missing_attr", R"(val = 1;)");
    auto res = Run(std::format(R"(
try{{ from {} import missing;}}
catch(e){{ print(e); }}
)",
                               mod.modname));
    EXPECT_EQ(res, "module 'repl_mod_missing_attr' has no attribute 'missing'");
  }

  {  // module missing attribute
    Source mod("repl_mod_no_attr", R"(val = 1;)");
    auto res = Run(std::format(R"(
import {} as m;
try{{ a = m.missing; }}
catch(e){{ print(e); }}
)",
                               mod.modname));
    EXPECT_EQ(res, "module 'repl_mod_no_attr' has no attribute 'missing'");
  }
}

TEST_F(CompilerTest, TryCatchThrow) {
  {
    auto res = Run(R"(
try{
  throw 5;
} catch(e){
  print(e);
}
)");
    EXPECT_EQ(res, "5\n");
  }

  {
    auto res = Run(R"(
result = 0;
try{
  result = 1;
  try{ throw 2; }
  catch(e){ result = e; }
} catch(e) {
  result = 3;
}

print(result);
)");
    EXPECT_EQ(res, "2\n");
  }
}

// ==============================================================================
// Unhandled errors in repl mode
TEST_F(CompilerTest, UnhandledThrow) {
  {
    auto res = Interpret({"a=non_exist;", "\"still alive\";"});
    EXPECT_EQ(res, "NameError: 'non_exist' is not defined\nstill alive\n");
  }
}

TEST_F(CompilerTest, ListIndexType) {
  auto res = Interpret({
      R"(a = [1,2,3];)",
      R"(a["0"];)",
      R"("ok";)",
  });
  EXPECT_EQ(res, "list index must be integer, but got: <str: 0>\nok");
}

TEST_F(CompilerTest, DictIndexType) {
  auto res = Interpret({
      R"(a = {"x":1};)",
      R"(a[0];)",
      R"("ok";)",
  });
  EXPECT_EQ(res, "dictionary index must be string, but got: <int: 0>\nok");
}

TEST_F(CompilerTest, NegativeShiftRHS) {
  auto res = Interpret({
      R"(1 << -1;)",
      R"(1 >> -2;)",
      R"(1 >>> -3;)",
      R"("ok";)",
  });
  EXPECT_EQ(res, R"(
negative shift count: -1
negative shift count: -2
negative shift count: -3
ok
)");
}

TEST_F(CompilerTest, ItemAccessNonExist) {
  auto res = Interpret({
      R"(a = [1,2]; a[5];)",      // out of range
      R"(d = {"x":1}; d["y"];)",  // missing key
      R"("ok";)",
  });
  EXPECT_EQ(res, R"(
list index '5' out of range
dictionary has no key: y
ok
)");
}

TEST_F(CompilerTest, AssignmentNotSupported) {
  auto res = Interpret({
      R"(s = "abc"; s[0] = "x";)",
      R"("ok";)",
  });
  EXPECT_EQ(res, R"(
'<str: abc>' object does not support item assignment.
ok
)");
}

}  // namespace m6test
