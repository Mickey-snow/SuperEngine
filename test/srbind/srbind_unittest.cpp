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

#include "srbind/srbind.hpp"

#include "vm/exception.hpp"
#include "vm/gc.hpp"
#include "vm/object.hpp"
#include "vm/value.hpp"
#include "vm/vm.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace srbind_test {

using namespace serilang;
using namespace srbind;

class SrbindTest : public ::testing::Test {
 protected:
  using error_type = serilang::RuntimeError;
  std::shared_ptr<GarbageCollector> gc;

 private:
  VM vm;
  Fiber* f;

 protected:
  serilang::Dict* dict;
  srbind::module_ mod;

  SrbindTest()
      : gc(std::make_shared<GarbageCollector>()),
        vm(gc),
        f(gc->Allocate<Fiber>()),
        dict(gc->Allocate<Dict>()),
        mod(gc.get(), dict) {}

  // Helper: push a callee and args on the fiber stack and invoke the callee.
  // Layout: [callee, pos0, pos1, ..., k1, v1, k2, v2, ...]
  Value CallCallee(
      Value callee,
      const std::vector<Value>& pos = {},
      const std::vector<std::pair<std::string, Value>>& kwargs = {}) {
    f->stack.clear();
    f->stack.emplace_back(std::move(callee));  // callee first
    for (auto const& v : pos)
      f->stack.emplace_back(v);
    for (auto const& kv : kwargs) {
      f->stack.emplace_back(kv.first);
      f->stack.emplace_back(kv.second);
    }
    const uint8_t nargs = static_cast<uint8_t>(pos.size());
    const uint8_t nkwargs = static_cast<uint8_t>(kwargs.size());

    callee.Call(vm, *f, nargs, nkwargs);
    Value ret = f->stack.back();  // (callee) <- (retval)
    return ret;
  }

  // Helper: get a member value
  Value GetMember(IObject* receiver, std::string_view item) {
    TempValue tval = receiver->Member(item);
    return gc->TrackValue(std::move(tval));
  }
  Value GetItem(serilang::Dict* dict, char const* item) {
    auto it = dict->map.find(item);
    if (it == dict->map.end()) {
      ADD_FAILURE() << dict->Desc() << " has no item " << item;
      return nil;
    }
    return it->second;
  }

  template <typename T>
  auto get_as(NativeInstance* inst, const char* item) -> std::add_pointer_t<T> {
    TempValue tval = inst->Member(item);
    Value val = gc->TrackValue(std::move(tval));
    auto* r = val.template Get_if<T>();
    if (!r)
      ADD_FAILURE() << "item not found: " << item;
    return r;
  }
  template <typename T, typename U>
  auto get_as(transparent_hashmap<U>& map, std::string_view item)
      -> std::add_pointer_t<T> {
    auto it = map.find(item);
    if (it == map.cend()) {
      ADD_FAILURE() << "item not found: " << item;
      return nullptr;
    }
    auto* r = it->second.template Get_if<T>();
    if (!r) {
      ADD_FAILURE() << "unexpected nullptr: " << item;
      return nullptr;
    }
    return r;
  }
};

TEST_F(SrbindTest, VoidReturn_YieldsNil) {
  bool touched = false;
  auto touch = [&](int x) { touched = (x == 42); };

  NativeFunction* nf = make_function(gc.get(), "touch", touch, arg("x"));

  Value r = CallCallee(Value(nf), {}, {{"x", Value(42)}});
  EXPECT_TRUE(touched);
  // nil is std::monostate in Value
  EXPECT_TRUE(r == std::monostate{});
}

TEST_F(SrbindTest, FreeFunction_PositionalOnly) {
  auto add = [](int a, int b) { return a + b; };
  NativeFunction* nf = make_function(gc.get(), "add", add);  // no arg spec
  Value r = CallCallee(Value(nf), {Value(2), Value(3)});
  EXPECT_TRUE(r == 5);

  EXPECT_THROW(
      [&]() { std::ignore = CallCallee(Value(nf), {}, {{"a", Value(1)}}); }(),
      error_type)
      << "Passing kwargs should error: function takes no keyword arguments";
}

TEST_F(SrbindTest, FreeFunction_KeywordsAndDefaults) {
  // a + 10*b + 100*c
  auto mix = [](int a, int b, int c) { return a + 10 * b + 100 * c; };

  NativeFunction* nf_ptr =
      make_function(gc.get(), "mix", mix, arg("a") = 1, arg("b") = 2,
                    arg("c")  // required
      );
  Value nf = Value(nf_ptr);

  // kwargs only (use defaults for a,b)
  Value r1 = CallCallee(nf, /*pos*/ {}, /*kw*/ {{"c", Value(7)}});
  EXPECT_EQ(r1, 1 + 10 * 2 + 100 * 7);

  // mixed: positional a=9, kw c=7 (b default=2)
  Value r2 = CallCallee(nf, {Value(9)}, {{"c", Value(7)}});
  EXPECT_EQ(r2, 9 + 10 * 2 + 100 * 7);

  // all keywords re-ordered
  Value r3 =
      CallCallee(nf, {}, {{"b", Value(3)}, {"a", Value(2)}, {"c", Value(4)}});
  EXPECT_EQ(r3, 2 + 10 * 3 + 100 * 4);

  // duplicate (positional + same kw)
  EXPECT_THROW(std::ignore = CallCallee(nf, {Value(5)},
                                        {{"a", Value(6)}, {"c", Value(1)}}),
               error_type)
      << "Expected error for duplicate arg";
  ;

  // unexpected keyword
  EXPECT_THROW(
      std::ignore = CallCallee(nf, {}, {{"z", Value(1)}, {"c", Value(2)}}),
      error_type)
      << "Expected error for unexpected keyword";

  // missing required 'c'
  EXPECT_THROW(std::ignore = CallCallee(nf), error_type)
      << "Expected error for missing required argument";

  // too many positionals
  EXPECT_THROW(
      std::ignore = CallCallee(nf, {Value(1), Value(2), Value(3), Value(4)}),
      error_type)
      << "Expected error for too many positional";
  // type error (c must be int)
  EXPECT_THROW(
      std::ignore = CallCallee(nf, {}, {{"c", Value(std::string("oops"))}}),
      error_type)
      << "Expected error for type mismatch";
}

struct V {
  int s{0};
  void add(int dx, int dy) { s += dx + dy; }
  int sum() const { return s; }
};

TEST_F(SrbindTest, Class_Methods) {
  // Bind class V with __init__() (no args) and methods:
  // add(dx, dy=0), sum()
  class_<V> cv(mod, "V");
  cv.def(init<>())  // no args
      .def("add", &V::add, arg("dx"), arg("dy") = 0)
      .def("sum", &V::sum);

  // Fetch class object from dict
  Value vclass = GetItem(dict, "V");

  // Construct instance by calling the class (allocates NativeInstance and
  // copies methods)
  Value inst_v = CallCallee(vclass);  // returns NativeInstance*
  auto* inst = inst_v.Get_if<NativeInstance>();
  ASSERT_NE(inst, nullptr);

  // Call add using kwargs to set dx=3, dy=2
  Value add_fn = GetMember(inst, "add");
  ASSERT_NO_THROW(std::ignore = CallCallee(
                      add_fn, {}, {{"dy", Value(2)}, {"dx", Value(3)}}));

  // sum() -> 5
  Value sum_fn = GetMember(inst, "sum");
  Value ret;
  ASSERT_NO_THROW(ret = CallCallee(sum_fn));
  EXPECT_EQ(ret, 5);

  // Now call add with only dx (dy default=0)
  ASSERT_NO_THROW(std::ignore = CallCallee(add_fn, {Value(4)}));
  ASSERT_NO_THROW(ret = CallCallee(sum_fn));
  EXPECT_TRUE(ret == 9);

  // Duplicate value for dx (positional + kw)
  EXPECT_THROW(std::ignore = CallCallee(add_fn, {Value(1)}, {{"dx", Value(2)}}),
               error_type)
      << "Expected error duplicate 'dx'";
}

struct P {
  int x, y;
  P(int x_, int y_) : x(x_), y(y_) {}
  int sum() const { return x + y; }
};

TEST_F(SrbindTest, Class_Init) {
  class_<P> cp(mod, "P");
  cp.def(init<int, int>(), arg("x") = 10, arg("y")).def("sum", &P::sum);

  Value klass = GetItem(dict, "P");

  // Allocate instance by calling class
  // Call __init__(self, y=5)  -> x=10 (default), y=5
  Value inst_v = CallCallee(klass, {}, {{"y", Value(5)}});
  auto* inst = inst_v.Get_if<NativeInstance>();
  ASSERT_NE(inst, nullptr);

  Value sum_fn = GetMember(inst, "sum");
  Value sum_ret = CallCallee(sum_fn);
  EXPECT_TRUE(sum_ret == 15);

  // Missing required argument
  EXPECT_THROW(std::ignore = CallCallee(klass, {}, {{"x", Value(5)}}),
               error_type)
      << "Expected missing required argument 'y'";
}

}  // namespace srbind_test
