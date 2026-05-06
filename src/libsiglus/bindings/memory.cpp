// -----------------------------------------------------------------------
//
// This file is part of RLVM
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2026 Serina Sakurai
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
// -----------------------------------------------------------------------

#include "libsiglus/bindings/memory.hpp"

#include "libsiglus/archive.hpp"
#include "libsiglus/bindings/util.hpp"
#include "libsiglus/property.hpp"
#include "log/core.hpp"
#include "log/domain_logger.hpp"
#include "srbind/module.hpp"
#include "srbind/srbind.hpp"
#include "utilities/assertx.hpp"
#include "vm/exception.hpp"
#include "vm/gc.hpp"
#include "vm/vm.hpp"

#include <memory>

namespace libsiglus::binding {
namespace sb = srbind;
using namespace serilang;

struct MemoryBank {
  bool is_dynamic = false;
  Value default_value = Value(0);
  std::vector<Value> payload;

  MemoryBank(bool dynamic = true, int size = 8, Value default_value = Value(0))
      : is_dynamic(dynamic), default_value(default_value) {
    if (size < 0)
      throw RuntimeError("cannot create memory bank with negative size: " +
                         std::to_string(size));
    payload.resize(static_cast<std::size_t>(size), this->default_value);
  }

  Value get(int idx) {
    ASSERTX_GE(idx, 0);
    auto index = static_cast<std::size_t>(idx);
    if (index >= payload.size()) {
      if (!is_dynamic)
        throw RuntimeError("Out of range: " + std::to_string(idx));
      payload.resize(index + 1, default_value);
    }
    return payload[index];
  }

  void set(int idx, Value val) {
    ASSERTX_GE(idx, 0);
    auto index = static_cast<std::size_t>(idx);
    if (index >= payload.size()) {
      if (!is_dynamic)
        throw RuntimeError("Out of range: " + std::to_string(idx));
      payload.resize(index + 1, default_value);
    }
    payload[index] = val;
  }
};

void TraceMemoryBank(GCVisitor& visitor, void* data) {
  auto* bank = static_cast<MemoryBank*>(data);
  visitor.MarkSub(bank->default_value);
  for (Value& value : bank->payload)
    visitor.MarkSub(value);
}

void Memory::Bind(SiglusRuntime& runtime) {
  VM& vm = *runtime.vm;
  sb::module_ m(vm.gc_.get(), runtime.vm->globals_.get());
  m.def(
      "make_intlist",
      [](VM& vm, int size) {
        if (size < 0)
          throw RuntimeError("cannot create integer list with negative size: " +
                             std::to_string(size));
        auto gc = vm.gc_;

        std::vector<Value> storage(size, Value(0));
        auto* list = gc->Allocate<List>(std::move(storage));
        return Value(list);
      },
      sb::arg("size"));
  m.def(
      "make_strlist",
      [](VM& vm, int size) {
        if (size < 0)
          throw RuntimeError("cannot create string list with negative size: " +
                             std::to_string(size));
        auto gc = vm.gc_;

        auto* str = gc->Allocate<String>("");
        std::vector<Value> storage(size, Value(str));
        auto* list = gc->Allocate<List>(std::move(storage));
        return Value(list);
      },
      sb::arg("size"));

  std::string src = "__globalprop = [];\n";
  Archive& archive = *runtime.archive;
  // install archive-global user properties
  for (size_t i = 0; i < archive.prop_.size(); ++i) {
    Property& p = archive.prop_[i];
    switch (p.form) {
      case Type::Int:
        src += "__globalprop.append(0);";
        break;
      case Type::IntList:
        src += std::format("__globalprop.append(make_intlist({}));", p.size);
        break;
      case Type::String:
        src += "__globalprop.append(\"\");";
        break;
      case Type::StrList:
        src += std::format("__globalprop.append(make_strlist({}));", p.size);
        break;

      default: {
        static DomainLogger log("Memory");
        log(Severity::Error)
            << "failed to install global property: " << p.ToDebugString();
        src += "__globalprop.append(nil);\n";
        break;
      }
    }
  }
  Execute(vm, std::move(src));

  // Install MemoryBank class
  sb::class_<MemoryBank> mb(m, "MemoryBank");
  mb.def(sb::init<bool, int, Value>(), sb::arg("dynamic") = true,
         sb::arg("size") = 8, sb::arg("default_value") = Value(0));
  mb.def("__getitem__", &MemoryBank::get, sb::arg("idx"));
  mb.def("__setitem__", &MemoryBank::set, sb::arg("idx"), sb::arg("val"));
  (*m.dict())["MemoryBank"].Get<NativeClass*>()->trace = &TraceMemoryBank;

  // Install global memory banks
  src.clear();
  constexpr std::string_view int_banks = "ABCDEFXGZL";
  for (auto bank : int_banks)
    src += std::format("{}=MemoryBank(true,8,0);", bank);
  for (std::string_view bank : {"K", "S", "M", "LN", "GN"})
    src += std::format("{}=MemoryBank(true,8,\"\");", bank);
  Execute(vm, std::move(src));

  // Install builtin call frame handlers for frame local memory banks
  src = R"(
__L_stk = []; __K_stk = [];
fn __builtin_push_frame(newl, newk){
  global L, K;
  __L_stk.append(L); __K_stk.append(K);
  L = MemoryBank(true, 8, 0);
  for(i=0; i<newl.len(); i+=1) L[i] = newl[i];
  K = MemoryBank(true, 8, "");
  for(i=0; i<newk.len(); i+=1) K[i] = newk[i];
}

fn __builtin_pop_frame(){
  global L, K;
  L = __L_stk.pop(); K = __K_stk.pop();
}
)";
  Execute(vm, std::move(src));
};

}  // namespace libsiglus::binding
