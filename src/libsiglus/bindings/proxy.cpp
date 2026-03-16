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

#include "libsiglus/bindings/proxy.hpp"

#include "srbind/module.hpp"

namespace libsiglus::binding {

namespace sr = serilang;
namespace sb = srbind;

sr::Value MakeProxy(sr::GarbageCollector* gc,
                    std::function<sr::Value()> getter,
                    std::function<sr::Value(sr::Value)> setter) {
  struct Proxy {
    std::function<sr::Value()> getter;
    std::function<sr::Value(sr::Value)> setter;
    sr::Value get() const { return getter(); }
    sr::Value set(sr::Value v) const { return setter(v); }
  };

  Proxy* px = new Proxy(std::move(getter), std::move(setter));
  auto [cls, inst] = sb::create_instance("proxy", px, gc);
  sb::instance_<Proxy> in(gc, cls, inst);
  in.def("get", &Proxy::get).def("set", &Proxy::set, sb::arg("value"));
  return sr::Value(inst);
}

}  // namespace libsiglus::binding
