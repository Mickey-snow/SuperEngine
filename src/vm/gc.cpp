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

#include "vm/gc.hpp"

#include "log/domain_logger.hpp"
#include "vm/chunk.hpp"
#include "vm/object.hpp"
#include "vm/upvalue.hpp"
#include "vm/value.hpp"
#include "vm/vm.hpp"

#include <memory>

namespace serilang {

GarbageCollector::~GarbageCollector() {
  while (gc_list_) {
    IObject* head = gc_list_;
    gc_list_ = gc_list_->hdr_.next;
    delete head;
  }
}

size_t GarbageCollector::AllocatedBytes() const { return allocated_bytes_; }

void GarbageCollector::TrackObject(IObject* obj) {
  GCHeader& hdr = obj->hdr_;
  hdr.next = gc_list_;
  hdr.marked = false;
  gc_list_ = obj;
}

void GarbageCollector::UnmarkAll() const {
  for (IObject* obj = gc_list_; obj; obj = obj->hdr_.next)
    obj->hdr_.marked = false;
}

// Marks all reachable objects starting from the VM roots
void GarbageCollector::MarkRoot(VM& vm) {
  for (auto& [name, val] : vm.globals_) {
    MarkRoot(val);
  }

  MarkRoot(vm.main_fiber_);
  MarkRoot(vm.last_);

  for (auto* f : vm.fibres_) {
    MarkRoot(f);
  }
}

void GarbageCollector::MarkRoot(Value& v) {
  if (auto obj = v.Get_if<IObject>()) {
    MarkRoot(obj);
  }
}

void GarbageCollector::MarkRoot(IObject* obj) {
  if (!obj || obj->hdr_.marked)
    return;
  obj->hdr_.marked = true;

  switch (obj->Type()) {
    case ObjType::List: {
      auto listObj = static_cast<List*>(obj);
      for (auto& item : listObj->items)
        MarkRoot(item);
    } break;

    case ObjType::Dict: {
      auto dictObj = static_cast<Dict*>(obj);
      for (auto& [k, val] : dictObj->map)
        MarkRoot(val);
    } break;

    case ObjType::Closure: {
      auto closureObj = static_cast<Closure*>(obj);
      if (auto chunk = closureObj->chunk) {
        for (auto& c : chunk->const_pool)
          MarkRoot(c);
      }
    } break;

    case ObjType::Fiber: {
      auto f = static_cast<Fiber*>(obj);

      MarkRoot(f->last);
      for (auto& item : f->stack)
        MarkRoot(item);
      for (auto& frame : f->frames)
        MarkRoot(frame.closure);
      for (auto& uv : f->open_upvalues) {
        if (uv->location == nullptr)
          continue;
        MarkRoot(*uv->location);
      }
    } break;

    case ObjType::Class: {
      auto klass = static_cast<Class*>(obj);
      for (auto& [name, method] : klass->methods)
        MarkRoot(method);
    } break;
    case ObjType::Instance: {
      auto inst = static_cast<Instance*>(obj);
      MarkRoot(inst->klass);
      for (auto& [name, field] : inst->fields)
        MarkRoot(field);
    } break;

    case ObjType::Native:
      break;

    case ObjType::Dummy:  // for testing, do nothing here
      break;

    default: {
      static DomainLogger logger("GarbageCollector::MarkRoot");
      logger(Severity::Error)
          << "Object '" << obj->Desc() << "' fields not marked.";
    } break;
  }
}

void GarbageCollector::Sweep() {
  IObject** cur = &gc_list_;
  while (*cur) {
    IObject* obj = *cur;
    if (!obj->hdr_.marked) {
      *cur = obj->hdr_.next;
      allocated_bytes_ -= obj->Size();
      delete obj;
    } else {
      obj->hdr_.marked = false;
      cur = &obj->hdr_.next;
    }
  }
}

}  // namespace serilang
