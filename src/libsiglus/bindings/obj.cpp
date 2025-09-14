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

#include "libsiglus/bindings/obj.hpp"
#include "srbind/srbind.hpp"

#include "core/frame_counter.hpp"
#include "core/rect.hpp"
#include "object/drawer/file.hpp"
#include "object/object_mutator.hpp"
#include "systems/base/graphics_object.hpp"
#include "systems/sdl/sdl_graphics_system.hpp"
#include "systems/sdl/sdl_system.hpp"
#include "systems/sdl_surface.hpp"

#include <set>

namespace libsiglus::binding {
namespace sr = serilang;
namespace sb = srbind;

class Object {
  std::shared_ptr<SDLGraphicsSystem> graphics_;
  size_t objid;
  GraphicsObject& obj;
  std::shared_ptr<Clock> clock = std::make_shared<Clock>();

 public:
  Object(std::shared_ptr<SDLGraphicsSystem> g)
      : graphics_(g),
        objid(g->GetFreeObjectId(OBJ_FG)),
        obj(g->GetObject(OBJ_FG, objid)) {}
  ~Object() { graphics_->RemoveObject(OBJ_FG, objid); }

  void Init() { obj.FreeDataAndInitializeParams(); }

  void Create(std::string filename, bool disp) {
    std::shared_ptr<Surface> surface = graphics_->LoadSurfaceFromFile(filename);

    auto obj_data = std::make_unique<GraphicsObjectOfFile>(surface);
    obj.SetObjectData(std::move(obj_data));
    obj.Param().SetVisible(disp);
  }

  void Render() {
    obj.ExecuteMutators();
    graphics_->RenderFrame(true);
  }

  int GetSizeX(int cut_no) { return obj.PixelWidth(); }
  int GetSizeY(int cut_no) { return obj.PixelHeight(); }

  void SetCenterRep(int x, int y) {
    obj.Param().SetRepOriginX(x);
    obj.Param().SetRepOriginY(y);
  }

  void SetScale(int x, int y) {
    obj.Param().SetScaleX(x / 10);
    obj.Param().SetScaleY(y / 10);
  }

  void SetPos(int x, int y) {
    obj.Param().SetX(x);
    obj.Param().SetY(y);
  }

  void SetXEve(int value, int total_time, int delay_time, int speed_type) {
    int x = obj.Param().Get<ObjectProperty::PositionX>();
    auto fc = std::make_shared<SimpleFrameCounter>(clock, x, value, total_time);
    fc->BeginTimer(std::chrono::milliseconds(delay_time));
    Mutator mutator{.setter_ = CreateSetter<ObjectProperty::PositionX>(),
                    .fc_ = fc};
    obj.AddObjectMutator(ObjectMutator({std::move(mutator)}, -1, "x_eve"));
  }

  void SetYEve(int value, int total_time, int delay_time, int speed_type) {
    int y = obj.Param().Get<ObjectProperty::PositionY>();
    auto fc = std::make_shared<SimpleFrameCounter>(clock, y, value, total_time);
    fc->BeginTimer(std::chrono::milliseconds(delay_time));
    Mutator mutator{.setter_ = CreateSetter<ObjectProperty::PositionY>(),
                    .fc_ = fc};
    obj.AddObjectMutator(ObjectMutator({std::move(mutator)}, -1, "y_eve"));
  }
};

void Obj::Bind(SiglusRuntime& runtime) {
  sr::VM& vm = *runtime.vm;

  sb::module_ m(vm.gc_.get(), vm.globals_);
  sb::class_<Object> o(m, "Object");

  o.def(sb::init([gs = runtime.system->graphics_system_]() -> Object* {
     return new Object(gs);
   })).def("init", &Object::Init);
  o.def("create", &Object::Create);
  o.def("get_size_x", &Object::GetSizeX, sb::arg("cut_no") = 0)
      .def("get_size_y", &Object::GetSizeY, sb::arg("cut_no") = 0);
  o.def("set_center_rep", &Object::SetCenterRep);
  o.def("set_scale", &Object::SetScale);
  o.def("set_pos", &Object::SetPos);
  o.def("set_xeve", &Object::SetXEve);
  o.def("set_yeve", &Object::SetYEve);

  o.def("render", &Object::Render);
}

}  // namespace libsiglus::binding
