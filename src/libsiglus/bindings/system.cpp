// -----------------------------------------------------------------------
//
// This file is part of RLVM
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
// -----------------------------------------------------------------------

#include "libsiglus/bindings/system.hpp"

#include "libsiglus/bindings/exception.hpp"
#include "log/domain_logger.hpp"
#include "srbind/srbind.hpp"
#include "vm/gc.hpp"
#include "vm/vm.hpp"

#include <ctime>
#include <string>

namespace libsiglus::binding {
namespace sb = srbind;

static void nop() {}

void System::Bind(serilang::VM& vm) {
  serilang::GarbageCollector* gc = vm.gc_.get();
  serilang::Dict* dict;

  auto [it, ok] = vm.globals_->map.try_emplace(
      "system", dict = gc->Allocate<serilang::Dict>());
  if (!ok)
    throw binding_error("cannot add 'system' dictionary");

  sb::module_ m(gc, dict);
  m.def("is_debug", +[]() { return false; });
  m.def("time", +[]() { return static_cast<int>(std::time(nullptr)); });
  m.def("check_file_exist", [root = ctx.base_pth](std::string filename) {
    return fs::exists(root / filename);
  });
  m.def("check_save_file_exist", [root = ctx.save_pth](std::string filename) {
    return fs::exists(root / filename);
  });
  m.def("check_dummy", &nop).def("clear_dummy", &nop);
  m.def("debug_write_log",
        [logger = DomainLogger("SiglusDbg")](std::string msg) {
          logger(Severity::Info) << std::move(msg);
        });
  m.def("get_lang", +[]() { return "ja"; });
};

}  // namespace libsiglus::binding
