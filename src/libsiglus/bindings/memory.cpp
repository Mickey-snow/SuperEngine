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

#include "libsiglus/archive.hpp"
#include "libsiglus/bindings/memory_facades.hpp"
#include "libsiglus/bindings/registry.hpp"
#include "libsiglus/bindings/util.hpp"
#include "libsiglus/property.hpp"
#include "log/domain_logger.hpp"
#include "srbind/srbind.hpp"
#include "vm/exception.hpp"
#include "vm/gc.hpp"
#include "vm/list.hpp"
#include "vm/string.hpp"
#include "vm/vm.hpp"

#include <algorithm>
#include <cstddef>
#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace libsiglus::binding {

namespace sb = srbind;
using namespace serilang;

namespace {

template <typename T, typename... Args>
Value MakeBoundNativeInstance(VM& vm,
                              std::string_view class_name,
                              Args&&... args) {
  auto it = vm.globals_->find(std::string(class_name));
  if (it == vm.globals_->end())
    throw RuntimeError(std::format("native class {} is not bound", class_name));

  auto* klass = it->second.Get_if<NativeClass>();
  if (!klass)
    throw RuntimeError(std::format("{} is not a native class", class_name));

  auto* inst = vm.gc_->Allocate<NativeInstance>(klass);
  inst->SetForeign<T>(new T(std::forward<Args>(args)...));
  return Value(inst);
}

template <typename T>
void BindIntSequenceMethods(sb::class_<T>& klass) {
  klass.def("__getitem__", &T::get, sb::arg("idx"));
  klass.def("__setitem__", &T::set, sb::arg("idx"), sb::arg("val"));
  klass.def("Set", &T::Set, sb::arg("idx"), sb::vararg);
  klass.def("resize", &T::resize, sb::arg("size"));
  klass.def("size", &T::size);
  klass.def("fill", &T::fill, sb::arg("begin"), sb::arg("end"), sb::arg("val"));
  klass.def("b1", &T::b1, sb::arg("idx"));
  klass.def("write_b1", &T::write_b1, sb::arg("idx"), sb::arg("val"));
  klass.def("b2", &T::b2, sb::arg("idx"));
  klass.def("write_b2", &T::write_b2, sb::arg("idx"), sb::arg("val"));
  klass.def("b4", &T::b4, sb::arg("idx"));
  klass.def("write_b4", &T::write_b4, sb::arg("idx"), sb::arg("val"));
  klass.def("b8", &T::b8, sb::arg("idx"));
  klass.def("write_b8", &T::write_b8, sb::arg("idx"), sb::arg("val"));
  klass.def("b16", &T::b16, sb::arg("idx"));
  klass.def("write_b16", &T::write_b16, sb::arg("idx"), sb::arg("val"));
}

template <typename T>
void BindStrSequenceMethods(sb::class_<T>& klass) {
  klass.def("__getitem__", &T::get, sb::arg("idx"));
  klass.def("__setitem__", &T::set, sb::arg("idx"), sb::arg("val"));
  klass.def("resize", &T::resize, sb::arg("size"));
  klass.def("size", &T::size);
  klass.def("fill", &T::fill, sb::arg("begin"), sb::arg("end"), sb::arg("val"));
}

}  // namespace

void BindMemory(SiglusRuntime& runtime) {
  VM& vm = *runtime.vm;
  sb::module_ m(vm.gc_.get(), runtime.vm->globals_.get());
  if (!runtime.memory)
    runtime.memory = std::make_unique<Memory>();
  Memory& memory = *runtime.memory;

  sb::class_<SiglusIntList> ilist(m, "__SiglusIntList");
  BindIntSequenceMethods(ilist);
  ilist.def("init", &SiglusIntList::init);

  sb::class_<SiglusStrList> slist(m, "__SiglusStrList");
  BindStrSequenceMethods(slist);
  slist.def("init", &SiglusStrList::init);

  m.def(
      "make_intlist",
      [](VM& vm, int size) {
        return MakeBoundNativeInstance<SiglusIntList>(vm, "__SiglusIntList",
                                                      size);
      },
      sb::arg("size"));
  m.def(
      "make_strlist",
      [](VM& vm, int size) {
        return MakeBoundNativeInstance<SiglusStrList>(vm, "__SiglusStrList",
                                                      size);
      },
      sb::arg("size"));

  std::string src = "__globalprop = [];\n";
  // install archive-global user properties
  if (runtime.archive) {
    Archive& archive = *runtime.archive;
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
  }
  Execute(vm, std::move(src));

  // Install memory bank classes and global bank views.
  sb::class_<SiglusIntBank> ibank(m, "__SiglusIntBank");
  BindIntSequenceMethods(ibank);
  ibank.def("init", &SiglusIntBank::init, sb::arg("val") = 0);

  sb::class_<SiglusStrBank> sbank(m, "__SiglusStrBank");
  BindStrSequenceMethods(sbank);
  sbank.def("Set", &SiglusStrBank::Set, sb::arg("idx"), sb::arg("val"));
  sbank.def("init", &SiglusStrBank::init, sb::arg("val") = "");

  auto bind_int_bank = [&](std::string_view name, IntBank bank) {
    ibank.inst(name, memory, bank);
  };
  auto bind_str_bank = [&](std::string_view name, StrBank bank) {
    sbank.inst(name, memory, bank);
  };

  bind_int_bank("A", IntBank::A);
  bind_int_bank("B", IntBank::B);
  bind_int_bank("C", IntBank::C);
  bind_int_bank("D", IntBank::D);
  bind_int_bank("E", IntBank::E);
  bind_int_bank("F", IntBank::F);
  bind_int_bank("X", IntBank::X);
  bind_int_bank("G", IntBank::G);
  bind_int_bank("Z", IntBank::Z);
  bind_int_bank("L", IntBank::L);

  bind_str_bank("S", StrBank::S);
  bind_str_bank("M", StrBank::M);
  bind_str_bank("K", StrBank::K);
  bind_str_bank("LN", StrBank::local_name);
  bind_str_bank("GN", StrBank::global_name);

  auto frame_stack = std::make_shared<std::vector<Memory::Stack>>();
  m.def(
      "__builtin_push_frame",
      [&memory, frame_stack](Value newl, Value newk) {
        const List* largs = RequireList(newl, "L frame arguments");
        const List* kargs = RequireList(newk, "K frame arguments");

        frame_stack->push_back(memory.GetStackMemory());
        Memory::Stack stack{
            .L = IntBankStorage(std::max<std::size_t>(8, largs->items.size())),
            .K = StrBankStorage(std::max<std::size_t>(8, kargs->items.size()))};
        memory.PartialReset(std::move(stack));

        for (std::size_t i = 0; i < largs->items.size(); ++i)
          memory.Write(IntBank::L, i,
                       RequireInt(largs->items[i], "L frame argument"));
        for (std::size_t i = 0; i < kargs->items.size(); ++i)
          memory.Write(StrBank::K, i, kargs->items[i].Str());
      },
      sb::arg("newl"), sb::arg("newk"));

  m.def("__builtin_pop_frame", [&memory, frame_stack]() {
    if (frame_stack->empty())
      throw RuntimeError("Siglus call frame stack underflow");
    memory.PartialReset(std::move(frame_stack->back()));
    frame_stack->pop_back();
  });
}

RLVM_REGISTER(SiglusBindingRegistry, "memory", BindMemory)

}  // namespace libsiglus::binding
