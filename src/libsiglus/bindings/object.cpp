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
#include "libsiglus/bindings/registry.hpp"

#include "core/object.hpp"
#include "libsiglus/bindings/util.hpp"
#include "srbind/module.hpp"
#include "vm/vm.hpp"

#include <string>

namespace libsiglus::binding {
namespace sb = srbind;

class SiglusObject {
  GraphicsObject go;

  ObjectParameter& param() { return go.Param(); }
  const ObjectParameter& param() const { return go.Param(); }

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
    go.SetObjectData(std::make_unique<ColourFilterObjectData>(rect));
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

template <auto member>
void BindObjectProperty(sb::class_<SiglusObject>& obj, const char* name) {
  const std::string setter_name = std::string("set_") + name;
  obj.def(name, &SiglusObject::get_member<member>);
  obj.def(setter_name.c_str(), &SiglusObject::set_member<member>,
          sb::arg("value"));
}

template <typename Getter, typename Setter>
void BindObjectProperty(sb::class_<SiglusObject>& obj,
                        const char* name,
                        Getter getter,
                        Setter setter) {
  const std::string setter_name = std::string("set_") + name;
  obj.def(name, getter);
  obj.def(setter_name.c_str(), setter, sb::arg("value"));
}

void BindObject(Context&, SiglusRuntime& runtime) {
  auto& vm = *runtime.vm;
  sb::module_ m(vm.gc_.get(), vm.globals_.get());
  sb::class_<SiglusObject> obj(m, "Object");

  obj.def(sb::init([]() -> SiglusObject* { return new SiglusObject(); }));

  BindObjectProperty<&ObjectParameter::wipe_copy>(obj, "wipe_copy");
  BindObjectProperty<&ObjectParameter::is_visible>(obj, "disp");
  BindObjectProperty<&ObjectParameter::pattern_number>(obj, "patno");
  BindObjectProperty<&ObjectParameter::z_order>(obj, "order");
  BindObjectProperty<&ObjectParameter::z_layer>(obj, "layer");
  BindObjectProperty<&ObjectParameter::position_x>(obj, "x");
  BindObjectProperty<&ObjectParameter::position_y>(obj, "y");
  BindObjectProperty<&ObjectParameter::z_depth>(obj, "z");
  BindObjectProperty<&ObjectParameter::origin_x>(obj, "center_x");
  BindObjectProperty<&ObjectParameter::origin_y>(obj, "center_y");
  BindObjectProperty<&ObjectParameter::repetition_origin_x>(obj,
                                                            "center_rep_x");
  BindObjectProperty<&ObjectParameter::repetition_origin_y>(obj,
                                                            "center_rep_y");
  BindObjectProperty<&ObjectParameter::scale_x_percent>(obj, "scale_x");
  BindObjectProperty<&ObjectParameter::scale_y_percent>(obj, "scale_y");
  BindObjectProperty<&ObjectParameter::rotation_div10>(obj, "rotate_z");

  BindObjectProperty(obj, "clip_use", &SiglusObject::get_clip_use,
                     &SiglusObject::set_clip_use);
  BindObjectProperty(obj, "clip_left", &SiglusObject::get_clip_left,
                     &SiglusObject::set_clip_left);
  BindObjectProperty(obj, "clip_top", &SiglusObject::get_clip_top,
                     &SiglusObject::set_clip_top);
  BindObjectProperty(obj, "clip_right", &SiglusObject::get_clip_right,
                     &SiglusObject::set_clip_right);
  BindObjectProperty(obj, "clip_bottom", &SiglusObject::get_clip_bottom,
                     &SiglusObject::set_clip_bottom);

  BindObjectProperty(obj, "src_clip_use", &SiglusObject::get_src_clip_use,
                     &SiglusObject::set_src_clip_use);
  BindObjectProperty(obj, "src_clip_left", &SiglusObject::get_src_clip_left,
                     &SiglusObject::set_src_clip_left);
  BindObjectProperty(obj, "src_clip_top", &SiglusObject::get_src_clip_top,
                     &SiglusObject::set_src_clip_top);
  BindObjectProperty(obj, "src_clip_right", &SiglusObject::get_src_clip_right,
                     &SiglusObject::set_src_clip_right);
  BindObjectProperty(obj, "src_clip_bottom", &SiglusObject::get_src_clip_bottom,
                     &SiglusObject::set_src_clip_bottom);

  BindObjectProperty<&ObjectParameter::alpha_source>(obj, "tr");
  BindObjectProperty<&ObjectParameter::monochrome_transform>(obj, "mono");
  BindObjectProperty<&ObjectParameter::invert_transform>(obj, "reverse");

  BindObjectProperty(obj, "color_r", &SiglusObject::get_color_r,
                     &SiglusObject::set_color_r);
  BindObjectProperty(obj, "color_g", &SiglusObject::get_color_g,
                     &SiglusObject::set_color_g);
  BindObjectProperty(obj, "color_b", &SiglusObject::get_color_b,
                     &SiglusObject::set_color_b);
  BindObjectProperty(obj, "color_rate", &SiglusObject::get_color_rate,
                     &SiglusObject::set_color_rate);
  BindObjectProperty(obj, "color_add_r", &SiglusObject::get_color_add_r,
                     &SiglusObject::set_color_add_r);
  BindObjectProperty(obj, "color_add_g", &SiglusObject::get_color_add_g,
                     &SiglusObject::set_color_add_g);
  BindObjectProperty(obj, "color_add_b", &SiglusObject::get_color_add_b,
                     &SiglusObject::set_color_add_b);

  BindObjectProperty<&ObjectParameter::composite_mode>(obj, "blend");

  obj.def("create_rect", &SiglusObject::create_rect);
}

RLVM_REGISTER(SiglusBindingRegistry, "0_object", BindObject)

}  // namespace libsiglus::binding
