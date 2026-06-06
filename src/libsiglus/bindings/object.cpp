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
#include "core/frame_counter.hpp"
#include "core/object_internal/drawer/colour_filter.hpp"
#include "core/object_internal/drawer/file.hpp"
#include "core/object_internal/drawer/movie.hpp"
#include "core/object_internal/object_mutator.hpp"
#include "libsiglus/bindings/registry.hpp"

#include "core/event_listener.hpp"
#include "core/object.hpp"
#include "core/stage.hpp"
#include "libsiglus/bindings/common.hpp"
#include "srbind/module.hpp"
#include "systems/event_system.hpp"
#include "systems/graphics_system.hpp"
#include "systems/system.hpp"
#include "utilities/overload.hpp"
#include "vm/dict.hpp"
#include "vm/list.hpp"
#include "vm/string.hpp"
#include "vm/value.hpp"
#include "vm/vm.hpp"

#include <algorithm>
#include <charconv>
#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
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

std::shared_ptr<FrameCounter> MakeSiglusFrameCounter(
    int duration,
    int delay,
    int start_val,
    int end_val,
    int type,
    std::shared_ptr<Clock> clock) {
  std::shared_ptr<FrameCounter> fc;
  switch (type) {
    case 1:
      fc = std::make_shared<AcceleratingFrameCounter>(
          std::move(clock), start_val, end_val, duration);
      break;
    case 2:
      fc = std::make_shared<DeceleratingFrameCounter>(
          std::move(clock), start_val, end_val, duration);
      break;
    case 0:
    default:
      fc = std::make_shared<SimpleFrameCounter>(std::move(clock), start_val,
                                                end_val, duration);
      break;
  }
  fc->BeginTimer(std::chrono::milliseconds(delay));
  return fc;
}

struct CallPacket {
  std::optional<int> overload_id;
  std::vector<sr::Value> args;
  const sr::Dict* kwargs = nullptr;
};

std::optional<int> ParseKeywordId(const sr::Value& key) {
  const sr::String* str = key.Get_if<sr::String>();
  if (!str)
    return std::nullopt;

  std::string_view text = str->str_;
  if (!text.empty() && text.front() == '_')
    text.remove_prefix(1);
  if (text.empty())
    return std::nullopt;

  int result = 0;
  const char* begin = text.data();
  const char* end = begin + text.size();
  const auto [ptr, ec] = std::from_chars(begin, end, result);
  if (ec != std::errc() || ptr != end)
    return std::nullopt;
  return result;
}

CallPacket DecodePacket(std::vector<sr::Value> raw) {
  if (raw.size() == 3 && raw[1].Get_if<sr::List>() &&
      raw[2].Get_if<sr::Dict>()) {
    const sr::List* args = raw[1].Get_if<sr::List>();
    return CallPacket{.overload_id = AsInt(raw[0]),
                      .args = args->items,
                      .kwargs = raw[2].Get_if<sr::Dict>()};
  }

  return CallPacket{.args = std::move(raw)};
}

struct MovieCreateParams {
  std::string file_name;
  std::optional<int> display;
  std::optional<int> x;
  std::optional<int> y;
  bool loop = false;
  bool wait = false;
  bool key_skip = false;
  bool auto_free = true;
  bool real_time = true;
  bool ready_only = false;
};

}  // namespace

class SiglusObject {
 public:
  Stage* stage_ = nullptr;
  std::shared_ptr<GraphicsSystem> graphics_;
  std::shared_ptr<EventSystem> event_;
  std::shared_ptr<AssetScanner> asset_scanner_;
  int layer_ = OBJ_FG;
  int object_id_ = 0;

  GraphicsObject& object() {
    if (!stage_)
      throw std::runtime_error("Object requires a stage buffer");
    if (object_id_ < 0)
      throw std::runtime_error("Invalid object number");

    switch (layer_) {
      case OBJ_FG:
        return stage_->foreground_objects[object_id_];
      case OBJ_BG:
        return stage_->background_objects[object_id_];
      case OBJ_NEXT:
        return stage_->next_objects[object_id_];
      default:
        throw std::runtime_error("Invalid object layer");
    }
  }

  const GraphicsObject& object() const {
    return const_cast<SiglusObject*>(this)->object();
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

  SiglusObject() = default;
  SiglusObject(Stage* stage,
               std::shared_ptr<GraphicsSystem> graphics,
               std::shared_ptr<EventSystem> event,
               std::shared_ptr<AssetScanner> asset_scanner,
               int layer,
               int object_id)
      : stage_(stage),
        graphics_(std::move(graphics)),
        event_(std::move(event)),
        asset_scanner_(std::move(asset_scanner)),
        layer_(layer),
        object_id_(object_id) {}

  void create(std::string filename) {
    if (!graphics_)
      throw std::runtime_error("Object.create requires a graphics system");

    if (filename.empty())
      throw std::runtime_error("Object.create filename is empty");

    GraphicsObject& obj = object();
    obj.FreeDataAndInitializeParams();
    auto surface = graphics_->GetSurfaceNamed(std::move(filename));
    obj.SetObjectData(std::make_unique<GraphicsObjectOfFile>(surface));
  }

  MovieCreateParams ParseCreateMovie(std::vector<sr::Value> raw_args,
                                     bool loop,
                                     bool wait,
                                     bool key_skip) {
    CallPacket packet = DecodePacket(std::move(raw_args));
    const std::vector<sr::Value>& args = packet.args;
    if (args.size() != 1 && args.size() != 2 && args.size() != 4) {
      throw std::runtime_error(
          "Object.create_movie expects 1, 2, or 4 positional args");
    }

    MovieCreateParams params;
    params.loop = loop;
    params.wait = wait;
    params.key_skip = key_skip;
    params.file_name = AsString(args[0]);
    if (params.file_name.empty())
      throw std::runtime_error("Object.create_movie filename is empty");

    if (args.size() >= 2)
      params.display = RequiredInt(args[1], "disp");
    if (args.size() >= 4) {
      params.x = RequiredInt(args[2], "x");
      params.y = RequiredInt(args[3], "y");
    }

    if (packet.kwargs) {
      for (const auto& [key, value] : packet.kwargs->map) {
        const std::optional<int> id = ParseKeywordId(key);
        if (!id)
          continue;

        switch (*id) {
          case 0:
            params.auto_free = AsInt(value).value_or(0) != 0;
            break;
          case 1:
            params.real_time = AsInt(value).value_or(0) != 0;
            break;
          case 2:
            params.ready_only = AsInt(value).value_or(0) != 0;
            break;
          default:
            break;
        }
      }
    }

    return params;
  }

  ObjectMovieData* movie_data() {
    if (!object().has_object_data())
      return nullptr;
    return dynamic_cast<ObjectMovieData*>(&object().GetObjectData());
  }

  const ObjectMovieData* movie_data() const {
    if (!object().has_object_data())
      return nullptr;
    return dynamic_cast<const ObjectMovieData*>(&object().GetObjectData());
  }

  void create_movie_common(std::vector<sr::Value> raw_args,
                           bool loop,
                           bool wait,
                           bool key_skip) {
    if (!graphics_)
      throw std::runtime_error(
          "Object.create_movie requires a graphics system");
    if (!asset_scanner_)
      throw std::runtime_error("Object.create_movie requires an asset scanner");

    MovieCreateParams params =
        ParseCreateMovie(std::move(raw_args), loop, wait, key_skip);
    auto movie_path = asset_scanner_->FindFile(params.file_name, {"omv"});
    if (!movie_path) {
      throw std::runtime_error("Object.create_movie could not find " +
                               params.file_name +
                               ".omv: " + movie_path.error().what());
    }

    GraphicsObject& obj = object();
    obj.FreeDataAndInitializeParams();
    obj.SetObjectData(std::make_unique<ObjectMovieData>(
        movie_path.value(), params.loop, params.auto_free, params.real_time,
        params.ready_only, graphics_->GetBackend(),
        event_ ? event_->GetClock() : std::make_shared<Clock>()));

    if (params.display)
      obj.Param().SetVisible(*params.display);
    if (params.x)
      obj.Param().SetX(*params.x);
    if (params.y)
      obj.Param().SetY(*params.y);

    if (params.wait && !params.ready_only)
      wait_movie_impl(params.key_skip);
  }

  void create_movie(std::vector<sr::Value> args) {
    create_movie_common(std::move(args), false, false, false);
  }

  void create_movie_loop(std::vector<sr::Value> args) {
    create_movie_common(std::move(args), true, false, false);
  }

  void create_movie_wait(std::vector<sr::Value> args) {
    create_movie_common(std::move(args), false, true, false);
  }

  void create_movie_waitkey(std::vector<sr::Value> args) {
    create_movie_common(std::move(args), false, true, true);
  }

  void PumpGraphicsOnce() {
    if (graphics_) {
      for (GraphicsObject& obj : graphics_->GetForegroundObjects()) {
        obj.Execute();
        obj.ExecuteMutators();
      }
      for (GraphicsObject& obj : graphics_->GetBackgroundObjects()) {
        obj.Execute();
        obj.ExecuteMutators();
      }
      graphics_->RenderFrame(true);
    }
    if (event_)
      event_->ExecuteEventSystem();
  }

  int wait_movie_impl(bool key_skip) {
    ObjectMovieData* data = movie_data();
    if (!data)
      return 0;

    struct MovieWaitListener : public EventListener {
      bool triggered = false;
      void OnEvent(std::shared_ptr<Event> event) override {
        if (!event)
          return;

        const bool consumed =
            std::visit(overload([](const KeyDown&) { return true; },
                                [](const MouseDown&) { return true; },
                                [](const auto&) { return false; }),
                       *event);
        if (consumed) {
          triggered = true;
          *event = std::monostate();
        }
      }
    };

    std::shared_ptr<MovieWaitListener> listener;
    if (key_skip && event_) {
      listener = std::make_shared<MovieWaitListener>();
      event_->AddListener(listener);
    }

    while ((data = movie_data()) && data->CheckMovie()) {
      PumpGraphicsOnce();
      if (listener && listener->triggered)
        break;
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    if (listener && event_)
      event_->RemoveListener(listener);

    return listener && listener->triggered ? 1 : 0;
  }

  void pause_movie() {
    if (ObjectMovieData* data = movie_data())
      data->Pause();
  }

  void resume_movie() {
    if (ObjectMovieData* data = movie_data())
      data->Resume();
  }

  void seek_movie(std::vector<sr::Value> raw_args) {
    if (ObjectMovieData* data = movie_data()) {
      CallPacket packet = DecodePacket(std::move(raw_args));
      if (!packet.args.empty())
        data->Seek(AsInt(packet.args[0]).value_or(0));
    }
  }

  int get_movie_seek_time() const {
    if (const ObjectMovieData* data = movie_data())
      return data->GetSeekTime();
    return 0;
  }

  int check_movie() const {
    if (const ObjectMovieData* data = movie_data())
      return data->CheckMovie() ? 1 : 0;
    return 0;
  }

  int wait_movie(std::vector<sr::Value>) { return wait_movie_impl(false); }

  int wait_movie_key(std::vector<sr::Value>) { return wait_movie_impl(true); }

  void end_movie_loop() {
    if (ObjectMovieData* data = movie_data())
      data->EndLoop();
  }

  void set_movie_auto_free(std::vector<sr::Value> raw_args) {
    if (ObjectMovieData* data = movie_data()) {
      CallPacket packet = DecodePacket(std::move(raw_args));
      if (!packet.args.empty())
        data->SetAutoFree(AsInt(packet.args[0]).value_or(0) != 0);
    }
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
};

void BindObject(Context&, SiglusRuntime& runtime) {
  auto& vm = *runtime.vm;
  sb::module_ m(vm.gc_.get(), vm.globals_.get());
  sb::class_<SiglusObject> obj(m, "Object");

  Stage* stage = runtime.stage.get();
  auto graphics = runtime.system ? runtime.system->graphics_ptr() : nullptr;
  auto event = runtime.system ? runtime.system->event_ptr() : nullptr;
  auto asset_scanner = runtime.asset_scanner;

  obj.def(sb::init([stage, graphics, event, asset_scanner](
                       int layer, int object_id) -> SiglusObject* {
            return new SiglusObject(stage, graphics, event, asset_scanner,
                                    layer, object_id);
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
  BindObjectMember.template operator()<&ObjectParameter::wipe_erase>(
      "wipe_erase");
  BindObjectMember.template operator()<&ObjectParameter::click_disable>(
      "click_disable");
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

  BindObjectProperty(
      "clip_use",
      [](const SiglusObject* obj) {
        return obj->param().has_clip_rect() ? 1 : 0;
      },
      [](SiglusObject* obj, int value) {
        if (value) {
          if (!obj->param().has_clip_rect())
            obj->param().SetClipRect(Rect::GRP(0, 0, 0, 0));
        } else {
          obj->param().ClearClipRect();
        }
      });
  BindObjectProperty(
      "clip_left",
      [](const SiglusObject* obj) { return obj->param().clip_rect().x(); },
      [](SiglusObject* obj, int value) {
        obj->SetClipRectValue(&Rect::set_x, value);
      });
  BindObjectProperty(
      "clip_top",
      [](const SiglusObject* obj) { return obj->param().clip_rect().y(); },
      [](SiglusObject* obj, int value) {
        obj->SetClipRectValue(&Rect::set_y, value);
      });
  BindObjectProperty(
      "clip_right",
      [](const SiglusObject* obj) { return obj->param().clip_rect().x2(); },
      [](SiglusObject* obj, int value) {
        obj->SetClipRectValue(&Rect::set_x2, value);
      });
  BindObjectProperty(
      "clip_bottom",
      [](const SiglusObject* obj) { return obj->param().clip_rect().y2(); },
      [](SiglusObject* obj, int value) {
        obj->SetClipRectValue(&Rect::set_y2, value);
      });

  BindObjectProperty(
      "src_clip_use",
      [](const SiglusObject* obj) {
        return obj->param().has_own_clip_rect() ? 1 : 0;
      },
      [](SiglusObject* obj, int value) {
        if (value) {
          if (!obj->param().has_own_clip_rect())
            obj->param().SetOwnClipRect(Rect::GRP(0, 0, 0, 0));
        } else {
          obj->param().ClearOwnClipRect();
        }
      });
  BindObjectProperty(
      "src_clip_left",
      [](const SiglusObject* obj) { return obj->param().own_clip_rect().x(); },
      [](SiglusObject* obj, int value) {
        obj->SetOwnClipRectValue(&Rect::set_x, value);
      });
  BindObjectProperty(
      "src_clip_top",
      [](const SiglusObject* obj) { return obj->param().own_clip_rect().y(); },
      [](SiglusObject* obj, int value) {
        obj->SetOwnClipRectValue(&Rect::set_y, value);
      });
  BindObjectProperty(
      "src_clip_right",
      [](const SiglusObject* obj) { return obj->param().own_clip_rect().x2(); },
      [](SiglusObject* obj, int value) {
        obj->SetOwnClipRectValue(&Rect::set_x2, value);
      });
  BindObjectProperty(
      "src_clip_bottom",
      [](const SiglusObject* obj) { return obj->param().own_clip_rect().y2(); },
      [](SiglusObject* obj, int value) {
        obj->SetOwnClipRectValue(&Rect::set_y2, value);
      });

  BindObjectMember.template operator()<&ObjectParameter::alpha_source>("tr");
  BindObjectMember.template operator()<&ObjectParameter::monochrome_transform>(
      "mono");
  BindObjectMember.template operator()<&ObjectParameter::invert_transform>(
      "reverse");

  BindObjectProperty(
      "color_r",
      [](const SiglusObject* obj) { return obj->param().colour_red(); },
      [](SiglusObject* obj, int value) { obj->param().SetColourRed(value); });
  BindObjectProperty(
      "color_g",
      [](const SiglusObject* obj) { return obj->param().colour_green(); },
      [](SiglusObject* obj, int value) { obj->param().SetColourGreen(value); });
  BindObjectProperty(
      "color_b",
      [](const SiglusObject* obj) { return obj->param().colour_blue(); },
      [](SiglusObject* obj, int value) { obj->param().SetColourBlue(value); });
  BindObjectProperty(
      "color_rate",
      [](const SiglusObject* obj) { return obj->param().colour_level(); },
      [](SiglusObject* obj, int value) { obj->param().SetColourLevel(value); });
  BindObjectProperty(
      "color_add_r",
      [](const SiglusObject* obj) { return obj->param().tint_red(); },
      [](SiglusObject* obj, int value) { obj->param().SetTintRed(value); });
  BindObjectProperty(
      "color_add_g",
      [](const SiglusObject* obj) { return obj->param().tint_green(); },
      [](SiglusObject* obj, int value) { obj->param().SetTintGreen(value); });
  BindObjectProperty(
      "color_add_b",
      [](const SiglusObject* obj) { return obj->param().tint_blue(); },
      [](SiglusObject* obj, int value) { obj->param().SetTintBlue(value); });

  BindObjectMember.template operator()<&ObjectParameter::mask_no>("mask_no");
  BindObjectMember.template operator()<&ObjectParameter::tonecurve_no>(
      "tonecurve_no");
  BindObjectMember.template operator()<&ObjectParameter::culling>("culling");
  BindObjectMember.template operator()<&ObjectParameter::alpha_test>(
      "alpha_test");
  BindObjectMember.template operator()<&ObjectParameter::alpha_blend>(
      "alpha_blend");
  BindObjectMember.template operator()<&ObjectParameter::light_no>("light_no");
  BindObjectMember.template operator()<&ObjectParameter::fog_use>("fog_use");
  BindObjectProperty(
      "blend",
      [](const SiglusObject* obj) { return obj->param().composite_mode; },
      [](SiglusObject* obj, int value) {
        obj->param().SetCompositeMode(std::clamp(value, 0, 4));
      });

  obj.def("init", [](SiglusObject* obj) {
    obj->object().FreeDataAndInitializeParams();
  });
  obj.def("init_param",
          [](SiglusObject* obj) { obj->object().InitializeParams(); });
  obj.def("free", [](SiglusObject* obj) { obj->object().FreeObjectData(); });
  obj.def(
      "create",
      [](SiglusObject* obj, std::vector<sr::Value> args) {
        if (args.size() != 1 && args.size() != 2 && args.size() != 4 &&
            args.size() != 5) {
          throw std::runtime_error("Object.create expects 1, 2, 4, or 5 args");
        }
        std::string filename = AsString(args[0]);
        if (filename.empty())
          throw std::runtime_error("Object.create filename is empty");

        std::optional<int> visible, x, y, pattern;
        if (args.size() >= 2)
          visible = RequiredInt(args[1], "disp");
        if (args.size() >= 4)
          x = RequiredInt(args[2], "x"), y = RequiredInt(args[3], "y");
        if (args.size() == 5)
          pattern = RequiredInt(args[4], "pat");

        obj->create(std::move(filename));
        if (visible)
          obj->param().SetVisible(*visible);
        if (x)
          obj->param().SetX(*x);
        if (y)
          obj->param().SetY(*y);
        if (pattern)
          obj->param().SetPattNo(*pattern);
      },
      sb::vararg);
  obj.def("create_movie", &SiglusObject::create_movie, sb::vararg);
  obj.def("create_movie_loop", &SiglusObject::create_movie_loop, sb::vararg);
  obj.def("create_movie_wait", &SiglusObject::create_movie_wait, sb::vararg);
  obj.def("create_movie_waitkey", &SiglusObject::create_movie_waitkey,
          sb::vararg);
  obj.def("create_rect", &SiglusObject::create_rect);
  obj.def(
      "get_size_x",
      [](const SiglusObject* obj, int cut_no) {
        return obj->object().PixelWidth();
      },
      sb::arg("cut_no") = 0);
  obj.def(
      "get_size_y",
      [](const SiglusObject* obj, int cut_no) {
        return obj->object().PixelHeight();
      },
      sb::arg("cut_no") = 0);
  obj.def("set_center_rep", &SiglusObject::set_center_rep);
  obj.def("set_scale", &SiglusObject::set_scale);
  obj.def("set_pos", &SiglusObject::set_pos);
  obj.def("pause_movie", &SiglusObject::pause_movie);
  obj.def("resume_movie", &SiglusObject::resume_movie);
  obj.def("seek_movie", &SiglusObject::seek_movie, sb::vararg);
  obj.def("get_movie_seek_time", &SiglusObject::get_movie_seek_time);
  obj.def("check_movie", &SiglusObject::check_movie);
  obj.def("wait_movie", &SiglusObject::wait_movie, sb::vararg);
  obj.def("wait_movie_key", &SiglusObject::wait_movie_key, sb::vararg);
  obj.def("end_movie_loop", &SiglusObject::end_movie_loop);
  obj.def("set_movie_auto_free", &SiglusObject::set_movie_auto_free,
          sb::vararg);

  // ------------------------------------------------------------------------------
  // Object Events
  struct ObjEve {
    SiglusObject* parent;
    std::shared_ptr<GraphicsSystem> graphics_;
    std::shared_ptr<EventSystem> event_;
    std::string name;
    std::function<int(const ObjectParameter&)> getter_;
    std::function<void(ObjectParameter&, int)> setter_;
    void verify() {
      if (!graphics_)
        throw std::runtime_error("ObjEve requires a graphics system");
      if (!event_)
        throw std::runtime_error("ObjEve requires an event system");
    }
  };
  sb::class_<ObjEve> oe(m, "ObjectEvent", false);
  oe.def(
      "set",
      [](ObjEve* oe, int end_value, int duration_time, int delay, int type) {
        oe->verify();
        GraphicsObject& obj = oe->parent->object();
        std::shared_ptr<Clock> clock = oe->event_->GetClock();

        obj.EndObjectMutatorMatching(-1, oe->name, 0);
        const int start = oe->getter_(obj.Param());
        const int end = end_value / 10;
        Mutator mutator{.setter_ = oe->setter_,
                        .fc_ = MakeSiglusFrameCounter(duration_time, delay,
                                                      start, end, type, clock)};
        obj.AddObjectMutator(ObjectMutator({std::move(mutator)}, -1, oe->name));
      },
      sb::arg("end_value"), sb::arg("duration_time"), sb::arg("delay"),
      sb::arg("type"));
  oe.def("end", [](ObjEve* oe) {
    oe->verify();
    GraphicsObject& obj = oe->parent->object();
    return obj.EndObjectMutatorMatching(-1, oe->name, 0);
  });
  oe.def("check", [](ObjEve* oe) {
    oe->verify();
    GraphicsObject& obj = oe->parent->object();
    bool ret = obj.IsMutatorRunningMatching(-1, oe->name);
    return ret ? 1 : 0;
  });

  // register objeve
  auto BindObjeve = [graphics, event, &oe, &obj](
                        std::string name,
                        std::function<int(const ObjectParameter&)> getter,
                        std::function<void(ObjectParameter&, int)> setter) {
    std::string field_name = name;
    obj.subcls(
        field_name, oe,
        [name = std::move(name), graphics, event, getter = std::move(getter),
         setter = std::move(setter)](SiglusObject* parent) {
          auto ret = std::make_unique<ObjEve>();
          ret->parent = parent;
          ret->graphics_ = graphics;
          ret->event_ = event;
          ret->name = name;
          ret->getter_ = getter;
          ret->setter_ = setter;
          return ret;
        });
  };
  auto BindObjeveMember = [&BindObjeve]<auto member>(std::string name) {
    BindObjeve(std::move(name), CreateGetter<member>(), CreateSetter<member>());
  };
  auto SetClipRectValue = [](ObjectParameter& param, void (Rect::*setter)(int),
                             int value) {
    Rect rect =
        param.has_clip_rect() ? param.clip_rect() : Rect::GRP(0, 0, 0, 0);
    (rect.*setter)(value);
    param.SetClipRect(rect);
  };
  auto SetOwnClipRectValue = [](ObjectParameter& param,
                                void (Rect::*setter)(int), int value) {
    Rect rect = param.has_own_clip_rect() ? param.own_clip_rect()
                                          : Rect::GRP(0, 0, 0, 0);
    (rect.*setter)(value);
    param.SetOwnClipRect(rect);
  };

  BindObjeveMember.template operator()<&ObjectParameter::pattern_number>(
      "patno_eve");
  BindObjeveMember.template operator()<&ObjectParameter::position_x>("x_eve");
  BindObjeveMember.template operator()<&ObjectParameter::position_y>("y_eve");
  BindObjeveMember.template operator()<&ObjectParameter::z_depth>("z_eve");
  BindObjeveMember.template operator()<&ObjectParameter::origin_x>(
      "center_x_eve");
  BindObjeveMember.template operator()<&ObjectParameter::origin_y>(
      "center_y_eve");
  BindObjeveMember.template operator()<&ObjectParameter::repetition_origin_x>(
      "center_rep_x_eve");
  BindObjeveMember.template operator()<&ObjectParameter::repetition_origin_y>(
      "center_rep_y_eve");
  BindObjeveMember.template operator()<&ObjectParameter::scale_x_percent>(
      "scale_x_eve");
  BindObjeveMember.template operator()<&ObjectParameter::scale_y_percent>(
      "scale_y_eve");
  BindObjeveMember.template operator()<&ObjectParameter::rotation_div10>(
      "rotate_z_eve");

  BindObjeve(
      "clip_left_eve",
      [](const ObjectParameter& param) { return param.clip_rect().x(); },
      [SetClipRectValue](ObjectParameter& param, int value) {
        SetClipRectValue(param, &Rect::set_x, value);
      });
  BindObjeve(
      "clip_top_eve",
      [](const ObjectParameter& param) { return param.clip_rect().y(); },
      [SetClipRectValue](ObjectParameter& param, int value) {
        SetClipRectValue(param, &Rect::set_y, value);
      });
  BindObjeve(
      "clip_right_eve",
      [](const ObjectParameter& param) { return param.clip_rect().x2(); },
      [SetClipRectValue](ObjectParameter& param, int value) {
        SetClipRectValue(param, &Rect::set_x2, value);
      });
  BindObjeve(
      "clip_bottom_eve",
      [](const ObjectParameter& param) { return param.clip_rect().y2(); },
      [SetClipRectValue](ObjectParameter& param, int value) {
        SetClipRectValue(param, &Rect::set_y2, value);
      });
  BindObjeve(
      "src_clip_left_eve",
      [](const ObjectParameter& param) { return param.own_clip_rect().x(); },
      [SetOwnClipRectValue](ObjectParameter& param, int value) {
        SetOwnClipRectValue(param, &Rect::set_x, value);
      });
  BindObjeve(
      "src_clip_top_eve",
      [](const ObjectParameter& param) { return param.own_clip_rect().y(); },
      [SetOwnClipRectValue](ObjectParameter& param, int value) {
        SetOwnClipRectValue(param, &Rect::set_y, value);
      });
  BindObjeve(
      "src_clip_right_eve",
      [](const ObjectParameter& param) { return param.own_clip_rect().x2(); },
      [SetOwnClipRectValue](ObjectParameter& param, int value) {
        SetOwnClipRectValue(param, &Rect::set_x2, value);
      });
  BindObjeve(
      "src_clip_bottom_eve",
      [](const ObjectParameter& param) { return param.own_clip_rect().y2(); },
      [SetOwnClipRectValue](ObjectParameter& param, int value) {
        SetOwnClipRectValue(param, &Rect::set_y2, value);
      });

  BindObjeveMember.template operator()<&ObjectParameter::alpha_source>(
      "tr_eve");
  BindObjeve(
      "tr_rep_eve",
      [](const ObjectParameter& param) { return param.alpha_adjustment(0); },
      [](ObjectParameter& param, int value) {
        param.SetAlphaAdjustment(0, value);
      });
  BindObjeveMember.template operator()<&ObjectParameter::monochrome_transform>(
      "mono_eve");
  BindObjeveMember.template operator()<&ObjectParameter::invert_transform>(
      "reverse_eve");
  BindObjeveMember.template operator()<&ObjectParameter::light_level>(
      "bright_eve");
  BindObjeve(
      "dark_eve",
      [](const ObjectParameter& param) { return -param.light_level; },
      [](ObjectParameter& param, int value) { param.light_level = -value; });

  BindObjeve(
      "color_r_eve",
      [](const ObjectParameter& param) { return param.colour_red(); },
      [](ObjectParameter& param, int value) { param.SetColourRed(value); });
  BindObjeve(
      "color_g_eve",
      [](const ObjectParameter& param) { return param.colour_green(); },
      [](ObjectParameter& param, int value) { param.SetColourGreen(value); });
  BindObjeve(
      "color_b_eve",
      [](const ObjectParameter& param) { return param.colour_blue(); },
      [](ObjectParameter& param, int value) { param.SetColourBlue(value); });
  BindObjeve(
      "color_rate_eve",
      [](const ObjectParameter& param) { return param.colour_level(); },
      [](ObjectParameter& param, int value) { param.SetColourLevel(value); });
  BindObjeve(
      "color_add_r_eve",
      [](const ObjectParameter& param) { return param.tint_red(); },
      [](ObjectParameter& param, int value) { param.SetTintRed(value); });
  BindObjeve(
      "color_add_g_eve",
      [](const ObjectParameter& param) { return param.tint_green(); },
      [](ObjectParameter& param, int value) { param.SetTintGreen(value); });
  BindObjeve(
      "color_add_b_eve",
      [](const ObjectParameter& param) { return param.tint_blue(); },
      [](ObjectParameter& param, int value) { param.SetTintBlue(value); });
}

RLVM_REGISTER(SiglusBindingRegistry, "0_object", BindObject)

}  // namespace libsiglus::binding
