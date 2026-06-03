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

#include "core/colour.hpp"
#include "core/object_internal/drawer/colour_filter.hpp"
#include "core/object_internal/drawer/file.hpp"
#include "libsiglus/bindings/registry.hpp"

#include "core/object.hpp"
#include "libsiglus/bindings/common.hpp"
#include "srbind/module.hpp"
#include "systems/graphics_system.hpp"
#include "systems/system.hpp"
#include "vm/value.hpp"
#include "vm/vm.hpp"

#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace libsiglus::binding {
namespace sb = srbind;
namespace sr = serilang;

namespace {

int RequiredInt(const sr::Value& value, std::string_view name) {
  std::optional<int> result = AsInt(value);
  if (!result)
    throw std::runtime_error("Object.create expected int for " +
                             std::string(name));
  return *result;
}

}  // namespace

class SiglusObject {
  std::shared_ptr<GraphicsSystem> graphics_;
  int layer_ = OBJ_FG;
  int object_id_ = 0;
  GraphicsObject owned_;

  GraphicsObject& object() {
    if (graphics_)
      return graphics_->GetObject(layer_, object_id_);
    return owned_;
  }

  const GraphicsObject& object() const {
    if (graphics_)
      return graphics_->GetObject(layer_, object_id_);
    return owned_;
  }

  ObjectParameter& param() { return object().Param(); }
  const ObjectParameter& param() const { return object().Param(); }

  void SetClipRectValue(void (Rect::*setter)(int), int value) {
    Rect rect =
        param().has_clip_rect() ? param().clip_rect() : Rect::GRP(0, 0, 0, 0);
    (rect.*setter)(value);
    param().SetClipRect(rect);
  }

  void SetOwnClipRectValue(void (Rect::*setter)(int), int value) {
    Rect rect = param().has_own_clip_rect() ? param().own_clip_rect()
                                            : Rect::GRP(0, 0, 0, 0);
    (rect.*setter)(value);
    param().SetOwnClipRect(rect);
  }

 public:
  SiglusObject() = default;
  SiglusObject(std::shared_ptr<GraphicsSystem> graphics,
               int layer,
               int object_id)
      : graphics_(std::move(graphics)), layer_(layer), object_id_(object_id) {}

  void init() { object().FreeDataAndInitializeParams(); }

  void create(std::vector<sr::Value> args) {
    if (args.size() != 1 && args.size() != 2 && args.size() != 4 &&
        args.size() != 5) {
      throw std::runtime_error("Object.create expects 1, 2, 4, or 5 args");
    }

    if (!graphics_)
      throw std::runtime_error("Object.create requires a graphics system");

    const std::string filename = AsString(args[0]);
    if (filename.empty())
      throw std::runtime_error("Object.create filename is empty");

    std::optional<int> visible;
    std::optional<int> x;
    std::optional<int> y;
    std::optional<int> pattern;

    if (args.size() >= 2)
      visible = RequiredInt(args[1], "disp");
    if (args.size() >= 4) {
      x = RequiredInt(args[2], "x");
      y = RequiredInt(args[3], "y");
    }
    if (args.size() == 5)
      pattern = RequiredInt(args[4], "pat");

    GraphicsObject& obj = object();
    obj.FreeDataAndInitializeParams();
    auto surface = graphics_->GetSurfaceNamed(filename);
    obj.SetObjectData(std::make_unique<GraphicsObjectOfFile>(surface));

    if (visible)
      obj.Param().SetVisible(*visible);
    if (x)
      obj.Param().SetX(*x);
    if (y)
      obj.Param().SetY(*y);
    if (pattern)
      obj.Param().SetPattNo(*pattern);
  }

  void create_rect(int left,
                   int top,
                   int right,
                   int down,
                   int r,
                   int g,
                   int b,
                   int alpha,
                   int display) {
    auto rect = Rect::GRP(left, top, right, down);
    param().blend_colour = RGBAColour(r, g, b, alpha);
    object().SetObjectData(std::make_unique<ColourFilterObjectData>(rect));
    param().SetVisible(display);
  }

  int get_size_x(int cut_no) const { return object().PixelWidth(); }
  int get_size_y(int cut_no) const { return object().PixelHeight(); }

  void set_center_rep(int x, int y) {
    param().SetRepOriginX(x);
    param().SetRepOriginY(y);
  }

  void set_scale(int x, int y) {
    param().SetScaleX(x / 10);
    param().SetScaleY(y / 10);
  }

  void set_pos(int x, int y) {
    param().SetX(x);
    param().SetY(y);
  }

  template <auto member>
  int get_member() const {
    return static_cast<int>(param().*member);
  }

  template <auto member>
  void set_member(int value) {
    using member_type = ObjectParameterMemberType<member>;
    param().*member = static_cast<member_type>(value);
  }

  int get_clip_use() const { return param().has_clip_rect(); }
  void set_clip_use(int value) {
    if (value) {
      if (!param().has_clip_rect())
        param().SetClipRect(Rect::GRP(0, 0, 0, 0));
    } else {
      param().ClearClipRect();
    }
  }

  int get_clip_left() const { return param().clip_rect().x(); }
  void set_clip_left(int value) { SetClipRectValue(&Rect::set_x, value); }
  int get_clip_top() const { return param().clip_rect().y(); }
  void set_clip_top(int value) { SetClipRectValue(&Rect::set_y, value); }
  int get_clip_right() const { return param().clip_rect().x2(); }
  void set_clip_right(int value) { SetClipRectValue(&Rect::set_x2, value); }
  int get_clip_bottom() const { return param().clip_rect().y2(); }
  void set_clip_bottom(int value) { SetClipRectValue(&Rect::set_y2, value); }

  int get_src_clip_use() const { return param().has_own_clip_rect(); }
  void set_src_clip_use(int value) {
    if (value) {
      if (!param().has_own_clip_rect())
        param().SetOwnClipRect(Rect::GRP(0, 0, 0, 0));
    } else {
      param().ClearOwnClipRect();
    }
  }

  int get_src_clip_left() const { return param().own_clip_rect().x(); }
  void set_src_clip_left(int value) {
    SetOwnClipRectValue(&Rect::set_x, value);
  }
  int get_src_clip_top() const { return param().own_clip_rect().y(); }
  void set_src_clip_top(int value) { SetOwnClipRectValue(&Rect::set_y, value); }
  int get_src_clip_right() const { return param().own_clip_rect().x2(); }
  void set_src_clip_right(int value) {
    SetOwnClipRectValue(&Rect::set_x2, value);
  }
  int get_src_clip_bottom() const { return param().own_clip_rect().y2(); }
  void set_src_clip_bottom(int value) {
    SetOwnClipRectValue(&Rect::set_y2, value);
  }

  int get_color_r() const { return param().colour_red(); }
  void set_color_r(int value) { param().SetColourRed(value); }
  int get_color_g() const { return param().colour_green(); }
  void set_color_g(int value) { param().SetColourGreen(value); }
  int get_color_b() const { return param().colour_blue(); }
  void set_color_b(int value) { param().SetColourBlue(value); }
  int get_color_rate() const { return param().colour_level(); }
  void set_color_rate(int value) { param().SetColourLevel(value); }

  int get_color_add_r() const { return param().tint_red(); }
  void set_color_add_r(int value) { param().SetTintRed(value); }
  int get_color_add_g() const { return param().tint_green(); }
  void set_color_add_g(int value) { param().SetTintGreen(value); }
  int get_color_add_b() const { return param().tint_blue(); }
  void set_color_add_b(int value) { param().SetTintBlue(value); }
};

void BindObject(Context&, SiglusRuntime& runtime) {
  auto& vm = *runtime.vm;
  sb::module_ m(vm.gc_.get(), vm.globals_.get());
  sb::class_<SiglusObject> obj(m, "Object");

  auto graphics = runtime.system ? runtime.system->graphics_ptr() : nullptr;
  obj.def(sb::init([graphics](int layer, int object_id) -> SiglusObject* {
            return new SiglusObject(graphics, layer, object_id);
          }),
          sb::arg("layer") = static_cast<int>(OBJ_FG),
          sb::arg("object_id") = 0);

  auto BindObjectMember = [&obj]<auto member>(const char* name) {
    const std::string setter_name = std::string("set_") + name;
    obj.def(name, &SiglusObject::get_member<member>);
    obj.def(setter_name.c_str(), &SiglusObject::set_member<member>,
            sb::arg("value"));
  };
  auto BindObjectProperty = [&obj](const char* name, auto getter, auto setter) {
    const std::string setter_name = std::string("set_") + name;
    obj.def(name, getter);
    obj.def(setter_name.c_str(), setter, sb::arg("value"));
  };

  BindObjectMember.template operator()<&ObjectParameter::wipe_copy>(
      "wipe_copy");
  BindObjectMember.template operator()<&ObjectParameter::is_visible>("disp");
  BindObjectMember.template operator()<&ObjectParameter::pattern_number>(
      "patno");
  BindObjectMember.template operator()<&ObjectParameter::z_order>("order");
  BindObjectMember.template operator()<&ObjectParameter::z_layer>("layer");
  BindObjectMember.template operator()<&ObjectParameter::position_x>("x");
  BindObjectMember.template operator()<&ObjectParameter::position_y>("y");
  BindObjectMember.template operator()<&ObjectParameter::z_depth>("z");
  BindObjectMember.template operator()<&ObjectParameter::origin_x>("center_x");
  BindObjectMember.template operator()<&ObjectParameter::origin_y>("center_y");
  BindObjectMember.template operator()<&ObjectParameter::repetition_origin_x>(
      "center_rep_x");
  BindObjectMember.template operator()<&ObjectParameter::repetition_origin_y>(
      "center_rep_y");
  BindObjectMember.template operator()<&ObjectParameter::scale_x_percent>(
      "scale_x");
  BindObjectMember.template operator()<&ObjectParameter::scale_y_percent>(
      "scale_y");
  BindObjectMember.template operator()<&ObjectParameter::rotation_div10>(
      "rotate_z");

  BindObjectProperty("clip_use", &SiglusObject::get_clip_use,
                     &SiglusObject::set_clip_use);
  BindObjectProperty("clip_left", &SiglusObject::get_clip_left,
                     &SiglusObject::set_clip_left);
  BindObjectProperty("clip_top", &SiglusObject::get_clip_top,
                     &SiglusObject::set_clip_top);
  BindObjectProperty("clip_right", &SiglusObject::get_clip_right,
                     &SiglusObject::set_clip_right);
  BindObjectProperty("clip_bottom", &SiglusObject::get_clip_bottom,
                     &SiglusObject::set_clip_bottom);

  BindObjectProperty("src_clip_use", &SiglusObject::get_src_clip_use,
                     &SiglusObject::set_src_clip_use);
  BindObjectProperty("src_clip_left", &SiglusObject::get_src_clip_left,
                     &SiglusObject::set_src_clip_left);
  BindObjectProperty("src_clip_top", &SiglusObject::get_src_clip_top,
                     &SiglusObject::set_src_clip_top);
  BindObjectProperty("src_clip_right", &SiglusObject::get_src_clip_right,
                     &SiglusObject::set_src_clip_right);
  BindObjectProperty("src_clip_bottom", &SiglusObject::get_src_clip_bottom,
                     &SiglusObject::set_src_clip_bottom);

  BindObjectMember.template operator()<&ObjectParameter::alpha_source>("tr");
  BindObjectMember.template operator()<&ObjectParameter::monochrome_transform>(
      "mono");
  BindObjectMember.template operator()<&ObjectParameter::invert_transform>(
      "reverse");

  BindObjectProperty("color_r", &SiglusObject::get_color_r,
                     &SiglusObject::set_color_r);
  BindObjectProperty("color_g", &SiglusObject::get_color_g,
                     &SiglusObject::set_color_g);
  BindObjectProperty("color_b", &SiglusObject::get_color_b,
                     &SiglusObject::set_color_b);
  BindObjectProperty("color_rate", &SiglusObject::get_color_rate,
                     &SiglusObject::set_color_rate);
  BindObjectProperty("color_add_r", &SiglusObject::get_color_add_r,
                     &SiglusObject::set_color_add_r);
  BindObjectProperty("color_add_g", &SiglusObject::get_color_add_g,
                     &SiglusObject::set_color_add_g);
  BindObjectProperty("color_add_b", &SiglusObject::get_color_add_b,
                     &SiglusObject::set_color_add_b);

  BindObjectMember.template operator()<&ObjectParameter::composite_mode>(
      "blend");

  obj.def("init", &SiglusObject::init);
  obj.def("create", &SiglusObject::create, sb::vararg);
  obj.def("create_rect", &SiglusObject::create_rect);
  obj.def("get_size_x", &SiglusObject::get_size_x, sb::arg("cut_no") = 0)
      .def("get_size_y", &SiglusObject::get_size_y, sb::arg("cut_no") = 0);
  obj.def("set_center_rep", &SiglusObject::set_center_rep);
  obj.def("set_scale", &SiglusObject::set_scale);
  obj.def("set_pos", &SiglusObject::set_pos);
}

RLVM_REGISTER(SiglusBindingRegistry, "0_object", BindObject)

}  // namespace libsiglus::binding
