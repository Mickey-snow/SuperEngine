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

#include "libsiglus/bindings/registry.hpp"

#include "libsiglus/bindings/wait_helpers.hpp"
#include "libsiglus/mask.hpp"
#include "srbind/srbind.hpp"
#include "systems/event_system.hpp"
#include "systems/graphics_system.hpp"
#include "systems/system.hpp"
#include "vm/value.hpp"
#include "vm/vm.hpp"

#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace libsiglus::binding {
namespace sb = srbind;
namespace sr = serilang;

namespace {

class MaskHandle {
 public:
  MaskHandle(std::shared_ptr<MaskList> list, int index)
      : list_(std::move(list)), index_(index) {
    (void)Get();
  }

  MaskElement& Get() { return list_->At(index_); }
  const MaskElement& Get() const { return list_->At(index_); }

 private:
  std::shared_ptr<MaskList> list_;
  int index_;
};

class MaskEventHandle {
 public:
  MaskEventHandle(MaskValue* value, EventSystem* event)
      : value_(value), event_(event) {}

  void Set(int value, int duration, int delay, int speed_type) {
    Get().SetEvent(value, duration, delay, speed_type);
  }
  void Loop(int start, int end, int duration, int delay, int speed_type) {
    Get().LoopEvent(start, end, duration, delay, speed_type);
  }
  void Turn(int start, int end, int duration, int delay, int speed_type) {
    Get().TurnEvent(start, end, duration, delay, speed_type);
  }
  void End() { Get().EndEvent(); }
  int Check() const { return Get().CheckEvent() ? 1 : 0; }

  sr::Value Wait(sr::VM& vm, std::vector<sr::Value>) {
    MaskValue* value = value_;
    return MakePollingWaitFuture(
        vm, [value] { return !value || !value->CheckEvent(); }, false, event_);
  }
  sr::Value WaitKey(sr::VM& vm, std::vector<sr::Value>) {
    MaskValue* value = value_;
    return MakePollingWaitFuture(
        vm, [value] { return !value || !value->CheckEvent(); }, true, event_);
  }

 private:
  MaskValue& Get() {
    if (!value_)
      throw std::runtime_error("mask event has no value");
    return *value_;
  }
  const MaskValue& Get() const {
    if (!value_)
      throw std::runtime_error("mask event has no value");
    return *value_;
  }

  MaskValue* value_;
  EventSystem* event_;
};

class MaskListHandle {
 public:
  using Factory = std::function<sr::Value(int)>;

  MaskListHandle(std::shared_ptr<MaskList> list, Factory factory)
      : list_(std::move(list)), factory_(std::move(factory)) {}

  sr::Value Get(int index) {
    (void)list_->At(index);
    return factory_(index);
  }
  int Size() const { return static_cast<int>(list_->size()); }

 private:
  std::shared_ptr<MaskList> list_;
  Factory factory_;
};

}  // namespace

void BindMask(SiglusRuntime& runtime) {
  if (!runtime.mask_list)
    throw std::runtime_error("mask binding requires mask state");

  sr::VM& vm = *runtime.vm;
  sb::module_ m(vm.gc_.get(), vm.globals_.get());
  sb::class_<MaskEventHandle> event_class(m, "MaskEvent", false);
  sb::class_<MaskHandle> mask_class(m, "Mask", false);
  sb::class_<MaskListHandle> list_class(m, "MaskList", false);

  EventSystem* event = runtime.system ? &runtime.system->event() : nullptr;
  GraphicsSystem* graphics =
      runtime.system ? &runtime.system->graphics() : nullptr;
  std::shared_ptr<MaskList> list = runtime.mask_list;

  event_class.def("set", &MaskEventHandle::Set, sb::arg("value"),
                  sb::arg("duration"), sb::arg("delay"), sb::arg("speed_type"));
  event_class.def("set_real", &MaskEventHandle::Set, sb::arg("value"),
                  sb::arg("duration"), sb::arg("delay"), sb::arg("speed_type"));
  event_class.def("loop", &MaskEventHandle::Loop, sb::arg("start"),
                  sb::arg("end"), sb::arg("duration"), sb::arg("delay"),
                  sb::arg("speed_type"));
  event_class.def("loop_real", &MaskEventHandle::Loop, sb::arg("start"),
                  sb::arg("end"), sb::arg("duration"), sb::arg("delay"),
                  sb::arg("speed_type"));
  event_class.def("turn", &MaskEventHandle::Turn, sb::arg("start"),
                  sb::arg("end"), sb::arg("duration"), sb::arg("delay"),
                  sb::arg("speed_type"));
  event_class.def("turn_real", &MaskEventHandle::Turn, sb::arg("start"),
                  sb::arg("end"), sb::arg("duration"), sb::arg("delay"),
                  sb::arg("speed_type"));
  event_class.def("end", &MaskEventHandle::End);
  event_class.def("check", &MaskEventHandle::Check);
  event_class.def("wait", &MaskEventHandle::Wait, sb::vararg);
  event_class.def("wait_key", &MaskEventHandle::WaitKey, sb::vararg);

  mask_class.def("init", [](MaskHandle* mask) { mask->Get().Reset(); });
  mask_class.def(
      "create",
      [graphics](MaskHandle* mask, std::string filename) {
        if (!graphics)
          throw std::runtime_error("mask.create requires a graphics system");
        mask->Get().Reset();
        std::shared_ptr<SDLSurface> surface =
            graphics->GetSurfaceNamed(filename);
        mask->Get().Create(std::move(filename), std::move(surface));
      },
      sb::arg("filename"));
  mask_class.def(
      "x", [](const MaskHandle* mask) { return mask->Get().x().GetValue(); });
  mask_class.def("set_x", [](MaskHandle* mask, int value) {
    mask->Get().x().SetValue(value);
  });
  mask_class.def(
      "y", [](const MaskHandle* mask) { return mask->Get().y().GetValue(); });
  mask_class.def("set_y", [](MaskHandle* mask, int value) {
    mask->Get().y().SetValue(value);
  });
  mask_class.subcls("x_eve", event_class, [event](MaskHandle* mask) {
    return std::make_unique<MaskEventHandle>(&mask->Get().x(), event);
  });
  mask_class.subcls("y_eve", event_class, [event](MaskHandle* mask) {
    return std::make_unique<MaskEventHandle>(&mask->Get().y(), event);
  });

  list_class.add_gc_root(mask_class);
  MaskListHandle::Factory make_mask = [mask_class,
                                       list](int index) mutable -> sr::Value {
    return sr::Value(mask_class.make_inst(list, index));
  };
  auto mask_list = list_class.inst("mask", list, std::move(make_mask));
  mask_list.def("__getitem__", &MaskListHandle::Get, sb::arg("index"));
  mask_list.def("size", &MaskListHandle::Size);
}

RLVM_REGISTER(SiglusBindingRegistry, "1_mask", BindMask)

}  // namespace libsiglus::binding
