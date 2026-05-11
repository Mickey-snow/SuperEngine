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
#include "libsiglus/bindings/registry.hpp"
#include "libsiglus/bindings/util.hpp"
#include "libsiglus/property.hpp"
#include "log/domain_logger.hpp"
#include "srbind/srbind.hpp"
#include "utilities/assertx.hpp"
#include "vm/exception.hpp"
#include "vm/gc.hpp"
#include "vm/list.hpp"
#include "vm/string.hpp"
#include "vm/vm.hpp"

#include <algorithm>
#include <cstddef>
#include <format>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace libsiglus::binding {

namespace sb = srbind;
using namespace serilang;

namespace {

std::size_t CheckIndex(int idx) {
  if (idx < 0)
    throw RuntimeError("negative memory bank index: " + std::to_string(idx));
  return static_cast<std::size_t>(idx);
}

std::size_t CheckSize(int size) {
  if (size < 0)
    throw RuntimeError("negative memory bank size: " + std::to_string(size));
  return static_cast<std::size_t>(size);
}

int CheckedIntSize(std::size_t size) {
  if (size > static_cast<std::size_t>(std::numeric_limits<int>::max()))
    throw RuntimeError("memory bank size exceeds script integer range");
  return static_cast<int>(size);
}

std::size_t RequiredIntWords(std::size_t logical_size, uint8_t bits) {
  if (bits == 32)
    return logical_size;

  const std::size_t max_bits = std::numeric_limits<std::size_t>::max() - 31;
  if (logical_size > max_bits / bits)
    throw RuntimeError("memory bank size overflow");

  return (logical_size * bits + 31) / 32;
}

int RequireInt(Value const& value, std::string_view where) {
  if (auto* i = value.Get_if<int>())
    return *i;
  throw RuntimeError(
      std::format("expected int for {}, got {}", where, value.Desc()));
}

const List* RequireList(Value const& value, std::string_view where) {
  if (auto* list = value.Get_if<List>())
    return list;
  throw RuntimeError(
      std::format("expected list for {}, got {}", where, value.Desc()));
}

class SiglusIntBank {
 public:
  SiglusIntBank(Memory& memory, IntBank bank, uint8_t bits = 32)
      : memory_(&memory), bank_(bank), bits_(bits) {
    ASSERTX_TRUE(bits == 32 || bits == 1 || bits == 2 || bits == 4 ||
                 bits == 8 || bits == 16);
  }

  int get(int idx) {
    const std::size_t index = CheckIndex(idx);
    EnsureSize(index + 1);
    return memory_->Read(IntMemoryLocation(bank_, index, bits_));
  }

  void set(int idx, int value) {
    const std::size_t index = CheckIndex(idx);
    EnsureSize(index + 1);
    memory_->Write(IntMemoryLocation(bank_, index, bits_), value);
  }

  void Set(int idx, int value) { set(idx, value); }

  void resize(int size) {
    memory_->Resize(bank_, RequiredIntWords(CheckSize(size), bits_));
  }

  int size() const {
    const std::size_t words = memory_->Size(bank_);
    if (bits_ == 32)
      return CheckedIntSize(words);
    return CheckedIntSize(words * (32 / bits_));
  }

  void fill(int begin, int end, int value) {
    const std::size_t begin_index = CheckIndex(begin);
    const std::size_t end_index = CheckIndex(end);
    if (begin_index > end_index)
      throw RuntimeError("invalid memory fill range");
    if (begin_index == end_index)
      return;

    EnsureSize(end_index);
    if (bits_ == 32) {
      memory_->Fill(bank_, begin_index, end_index, value);
      return;
    }

    for (std::size_t i = begin_index; i < end_index; ++i)
      memory_->Write(IntMemoryLocation(bank_, i, bits_), value);
  }

  void init(int value = 0) { fill(0, size(), value); }

 private:
  void EnsureSize(std::size_t logical_size) {
    const std::size_t required = RequiredIntWords(logical_size, bits_);
    if (memory_->Size(bank_) < required)
      memory_->Resize(bank_, required);
  }

  Memory* memory_;
  IntBank bank_;
  uint8_t bits_;
};

class SiglusStrBank {
 public:
  SiglusStrBank(Memory& memory, StrBank bank) : memory_(&memory), bank_(bank) {}

  std::string get(int idx) {
    const std::size_t index = CheckIndex(idx);
    EnsureSize(index + 1);
    return memory_->Read(bank_, index);
  }

  void set(int idx, std::string value) {
    const std::size_t index = CheckIndex(idx);
    EnsureSize(index + 1);
    memory_->Write(bank_, index, value);
  }

  void Set(int idx, std::string value) { set(idx, std::move(value)); }

  void resize(int size) { memory_->Resize(bank_, CheckSize(size)); }

  int size() const { return CheckedIntSize(memory_->Size(bank_)); }

  void fill(int begin, int end, std::string value) {
    const std::size_t begin_index = CheckIndex(begin);
    const std::size_t end_index = CheckIndex(end);
    if (begin_index > end_index)
      throw RuntimeError("invalid memory fill range");
    if (begin_index == end_index)
      return;

    EnsureSize(end_index);
    memory_->Fill(bank_, begin_index, end_index, value);
  }

  void init(std::string value = "") { fill(0, size(), std::move(value)); }

 private:
  void EnsureSize(std::size_t size) {
    if (memory_->Size(bank_) < size)
      memory_->Resize(bank_, size);
  }

  Memory* memory_;
  StrBank bank_;
};

}  // namespace

void BindMemory(Context& ctx, SiglusRuntime& runtime) {
  VM& vm = *runtime.vm;
  sb::module_ m(vm.gc_.get(), runtime.vm->globals_.get());
  if (!runtime.memory)
    runtime.memory = std::make_unique<Memory>();
  Memory& memory = *runtime.memory;

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
  ibank.def("__getitem__", &SiglusIntBank::get, sb::arg("idx"));
  ibank.def("__setitem__", &SiglusIntBank::set, sb::arg("idx"), sb::arg("val"));
  ibank.def("Set", &SiglusIntBank::Set, sb::arg("idx"), sb::arg("val"));
  ibank.def("resize", &SiglusIntBank::resize, sb::arg("size"));
  ibank.def("size", &SiglusIntBank::size);
  ibank.def("fill", &SiglusIntBank::fill, sb::arg("begin"), sb::arg("end"),
            sb::arg("val"));
  ibank.def("init", &SiglusIntBank::init, sb::arg("val") = 0);

  sb::class_<SiglusStrBank> sbank(m, "__SiglusStrBank");
  sbank.def("__getitem__", &SiglusStrBank::get, sb::arg("idx"));
  sbank.def("__setitem__", &SiglusStrBank::set, sb::arg("idx"), sb::arg("val"));
  sbank.def("Set", &SiglusStrBank::Set, sb::arg("idx"), sb::arg("val"));
  sbank.def("resize", &SiglusStrBank::resize, sb::arg("size"));
  sbank.def("size", &SiglusStrBank::size);
  sbank.def("fill", &SiglusStrBank::fill, sb::arg("begin"), sb::arg("end"),
            sb::arg("val"));
  sbank.def("init", &SiglusStrBank::init, sb::arg("val") = "");

  auto bind_int_bank = [&](std::string_view name, IntBank bank) {
    auto inst = ibank.inst(name, memory, bank);
    inst["b1"] = ibank.make_inst(memory, bank, 1);
    inst["b2"] = ibank.make_inst(memory, bank, 2);
    inst["b4"] = ibank.make_inst(memory, bank, 4);
    inst["b8"] = ibank.make_inst(memory, bank, 8);
    inst["b16"] = ibank.make_inst(memory, bank, 16);
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
            .L = MemoryBank<int>(Storage::DYNAMIC,
                                 std::max<std::size_t>(8, largs->items.size())),
            .K = MemoryBank<std::string>(
                Storage::DYNAMIC,
                std::max<std::size_t>(8, kargs->items.size()))};
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
