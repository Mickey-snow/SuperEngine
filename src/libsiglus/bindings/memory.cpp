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
#include "srbind/srbind.hpp"
#include "vm/exception.hpp"
#include "vm/gc.hpp"
#include "vm/vm.hpp"

#include <memory>

namespace libsiglus::binding {
namespace sb = srbind;
using namespace serilang;

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

  // TODO: install memory banks
};

}  // namespace libsiglus::binding
