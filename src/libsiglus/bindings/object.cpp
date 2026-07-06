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

#include "core/avdec/gan.hpp"
#include "core/colour.hpp"
#include "core/frame_counter.hpp"
#include "core/object_internal/drawer/colour_filter.hpp"
#include "core/object_internal/drawer/file.hpp"
#include "core/object_internal/drawer/gan.hpp"
#include "core/object_internal/drawer/movie.hpp"
#include "core/object_internal/object_mutator.hpp"
#include "core/object_internal/object_parameter.hpp"
#include "libsiglus/bindings/registry.hpp"

#include "core/object.hpp"
#include "core/stage.hpp"
#include "libsiglus/bindings/util.hpp"
#include "libsiglus/bindings/wait_helpers.hpp"
#include "srbind/module.hpp"
#include "systems/event_system.hpp"
#include "systems/graphics_system.hpp"
#include "systems/system.hpp"
#include "utilities/file.hpp"
#include "vm/exception.hpp"
#include "vm/string.hpp"
#include "vm/value.hpp"
#include "vm/vm.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <memory>
#include <optional>
#include <set>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace libsiglus::binding {
namespace sb = srbind;
namespace sr = serilang;

namespace {

constexpr int kBtnNormal = 0;
constexpr int kBtnSelect = 3;
constexpr int kBtnDisable = 4;

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

void ApplyObjectFileSuffix(std::string& filename, ObjectParameter& param) {
  const std::size_t pos = filename.find('?');
  if (pos == std::string::npos)
    return;

  const std::string tone_curve = filename.substr(pos + 1);
  filename.erase(pos);

  try {
    std::size_t parsed = 0;
    int value = std::stoi(tone_curve, &parsed);
    if (parsed == tone_curve.size() && value > 0)
      param.tonecurve_no = value;
  } catch (const std::exception&) {
  }
}

}  // namespace

struct ObjectReference {
  Stage* stage_ = nullptr;
  int layer_ = OBJ_FG;
  int object_id_ = 0;
  std::vector<std::size_t> child_path_;

  ObjectReference() = default;
  ObjectReference(Stage* stage, int layer, int id)
      : stage_(stage), layer_(layer), object_id_(id) {}
  ObjectReference(Stage* stage,
                  int layer,
                  int id,
                  std::vector<std::size_t> child_path)
      : stage_(stage),
        layer_(layer),
        object_id_(id),
        child_path_(std::move(child_path)) {}

  GraphicsObject& get() {
    if (!stage_)
      throw std::runtime_error("Object requires a stage buffer");
    if (object_id_ < 0) {
      throw sr::RuntimeError("Invalid object number: " +
                             std::to_string(object_id_));
    }

    GraphicsObject* current = &stage_->GetObject(layer_, object_id_);
    for (const std::size_t child_index : child_path_) {
      if (child_index >= current->GetChildren().size()) {
        throw sr::RuntimeError(
            "object.child index out of range: " + std::to_string(child_index) +
            " for size " + std::to_string(current->GetChildren().size()));
      }
      current = &current->TouchChild(child_index);
    }
    return *current;
  }
  const GraphicsObject& get() const {
    return const_cast<ObjectReference*>(this)->get();
  }

  ObjectReference child(std::size_t index) const {
    std::vector<std::size_t> path = child_path_;
    path.emplace_back(index);
    return ObjectReference(stage_, layer_, object_id_, std::move(path));
  }
};

namespace {

sr::Value WaitForObjectMutator(sr::VM& vm,
                               ObjectReference ref,
                               int repno,
                               std::string name,
                               EventSystem* event,
                               bool key_skip) {
  auto done = [ref = std::move(ref), repno, name = std::move(name)]() mutable {
    return !ref.get().IsMutatorRunningMatching(repno, name);
  };
  return MakePollingWaitFuture(vm, std::move(done), key_skip, event);
}

}  // namespace

class SiglusObject {
 public:
  ObjectReference ref_;
  std::shared_ptr<GraphicsSystem> graphics_;
  std::shared_ptr<EventSystem> event_;
  std::shared_ptr<AssetScanner> asset_scanner_;

  inline GraphicsObject& object() { return ref_.get(); }
  inline const GraphicsObject& object() const { return ref_.get(); }

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
      : ref_(stage, layer, object_id),
        graphics_(std::move(graphics)),
        event_(std::move(event)),
        asset_scanner_(std::move(asset_scanner)) {}
  SiglusObject(ObjectReference ref,
               std::shared_ptr<GraphicsSystem> graphics,
               std::shared_ptr<EventSystem> event,
               std::shared_ptr<AssetScanner> asset_scanner)
      : ref_(std::move(ref)),
        graphics_(std::move(graphics)),
        event_(std::move(event)),
        asset_scanner_(std::move(asset_scanner)) {}

  void create(std::string filename) {
    if (!graphics_)
      throw std::runtime_error("Object.create requires a graphics system");

    if (filename.empty())
      throw std::runtime_error("Object.create filename is empty");

    GraphicsObject& obj = object();
    obj.FreeDataAndInitializeParams();
    ApplyObjectFileSuffix(filename, obj.Param());
    if (filename.empty())
      throw std::runtime_error("Object.create filename is empty");

    if (IsCompositeObjectName(filename)) {
      std::vector<CompositeGraphicsObjectLayer> layers;
      for (const CompositeObjectPart& part :
           ParseCompositeObjectName(filename)) {
        layers.push_back(
            {.surface = graphics_->LoadSurfaceFromFile(part.file_name),
             .offset = Point(part.x, part.y),
             .cut_no = part.cut_no,
             .blend_type = part.blend_type});
      }
      obj.SetDrawer(
          std::make_unique<CompositeGraphicsObject>(std::move(layers)));
    } else {
      auto surface = graphics_->GetSurfaceNamed(filename);
      obj.SetDrawer(std::make_unique<GraphicsObjectOfFile>(surface));
    }
    obj.SetFilePath(std::move(filename));
  }

  void load_gan(std::string filename) {
    if (filename.empty())
      return;
    if (!graphics_)
      throw std::runtime_error("Object.load_gan requires a graphics system");
    if (!asset_scanner_)
      throw std::runtime_error("Object.load_gan requires an asset scanner");

    auto gan_path = asset_scanner_->FindFile(filename, {"gan"});
    if (!gan_path) {
      throw std::runtime_error("Object.load_gan could not find " + filename +
                               ".gan: " + gan_path.error().what());
    }

    GanDecoder decoder(LoadFile(*gan_path));
    const std::string image_name =
        decoder.raw_file_name.empty() ? filename : decoder.raw_file_name;
    auto image = graphics_->GetSurfaceNamed(image_name);
    auto data = std::make_unique<GanGraphicsObjectData>(
        image, std::move(decoder.animation_sets),
        event_ ? event_->GetClock() : std::make_shared<Clock>());
    if (data->HasSet(0))
      data->PrimeSet(0);
    object().SetDrawer(std::move(data));
  }

  void start_gan(std::vector<sr::Value> raw_args) {
    CallPacket packet = CallPacket::DecodeFrom(std::move(raw_args));
    const std::vector<sr::Value>& args = packet.args;
    if (args.size() > 3)
      throw std::runtime_error("Object.start_gan expects 0 to 3 args");

    int set_no = 0;
    bool loop = true;
    bool real_time = false;
    if (args.size() >= 1)
      set_no = RequireInt(args[0], "Object.start_gan set");
    if (args.size() >= 2)
      loop = RequireInt(args[1], "Object.start_gan loop") != 0;
    if (args.size() >= 3)
      real_time = RequireInt(args[2], "Object.start_gan real_time") != 0;
    (void)real_time;

    if (auto* data = object().GetDrawer<GanGraphicsObjectData>()) {
      data->PlaySet(set_no);
      data->GetAnimator()->SetAfterAction(loop ? AFTER_LOOP : AFTER_NONE);
    }
  }

  MovieCreateParams ParseCreateMovie(std::vector<sr::Value> raw_args,
                                     bool loop,
                                     bool wait,
                                     bool key_skip) {
    CallPacket packet = CallPacket::DecodeFrom(std::move(raw_args));
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

    ForEachKeywordId(packet.kwargs, [&](int id, const sr::Value& value) {
      switch (id) {
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
    });

    return params;
  }

  inline ObjectMovieData* movie_data() {
    return object().GetDrawer<ObjectMovieData>();
  }
  inline const ObjectMovieData* movie_data() const {
    return object().GetDrawer<const ObjectMovieData>();
  }

  bool create_movie_common(std::vector<sr::Value> raw_args,
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
    obj.SetDrawer(std::make_unique<ObjectMovieData>(
        movie_path.value(), params.loop, params.auto_free, params.real_time,
        params.ready_only, graphics_->GetBackend(),
        event_ ? event_->GetClock() : std::make_shared<Clock>()));
    obj.SetFilePath(params.file_name);

    if (params.display)
      obj.Param().SetVisible(*params.display);
    if (params.x)
      obj.Param().SetX(*params.x);
    if (params.y)
      obj.Param().SetY(*params.y);

    return params.wait && !params.ready_only;
  }

  void create_movie(std::vector<sr::Value> args) {
    create_movie_common(std::move(args), false, false, false);
  }

  void create_movie_loop(std::vector<sr::Value> args) {
    create_movie_common(std::move(args), true, false, false);
  }

  sr::Value create_movie_wait(sr::VM& vm, std::vector<sr::Value> args) {
    if (create_movie_common(std::move(args), false, true, false))
      return wait_movie_impl(vm, false);
    return MakeResolvedFuture(*vm.gc_);
  }

  sr::Value create_movie_waitkey(sr::VM& vm, std::vector<sr::Value> args) {
    if (create_movie_common(std::move(args), false, true, true))
      return wait_movie_impl(vm, true);
    return MakeResolvedFuture(*vm.gc_);
  }

  sr::Value wait_movie_impl(sr::VM& vm, bool key_skip) {
    auto done = [this] {
      const ObjectMovieData* data = movie_data();
      return !data || !data->CheckMovie();
    };
    return MakePollingWaitFuture(vm, std::move(done), key_skip, event_.get());
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
      CallPacket packet = CallPacket::DecodeFrom(std::move(raw_args));
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

  sr::Value wait_movie(sr::VM& vm, std::vector<sr::Value>) {
    return wait_movie_impl(vm, false);
  }

  sr::Value wait_movie_key(sr::VM& vm, std::vector<sr::Value>) {
    return wait_movie_impl(vm, true);
  }

  void end_movie_loop() {
    if (ObjectMovieData* data = movie_data())
      data->EndLoop();
  }

  void set_movie_auto_free(std::vector<sr::Value> raw_args) {
    if (ObjectMovieData* data = movie_data()) {
      CallPacket packet = CallPacket::DecodeFrom(std::move(raw_args));
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
    object().SetDrawer(std::make_unique<ColourFilterObjectData>(rect));
    object().ClearFilePath();
    param().SetVisible(display);
  }

  void set_center_rep(int x, int y) {
    param().SetRepOriginX(x);
    param().SetRepOriginY(y);
  }

  void set_scale(int x, int y) {
    param().SetHqScaleX(x);
    param().SetHqScaleY(y);
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

class ObjectEvent {
 public:
  ObjectReference ref_;
  std::shared_ptr<GraphicsSystem> graphics_;
  std::shared_ptr<EventSystem> event_;
  std::string name;
  std::function<int(const ObjectParameter&)> getter_;
  std::function<void(ObjectParameter&, int)> setter_;

  void set(int end_value, int duration_time, int delay, int type) {
    Verify();
    GraphicsObject& obj = ref_.get();
    std::shared_ptr<Clock> clock = event_->GetClock();

    obj.EndObjectMutatorMatching(-1, name, 0);
    const int start = getter_(obj.Param());
    Mutator mutator{.setter_ = setter_,
                    .fc_ = MakeSiglusFrameCounter(duration_time, delay, start,
                                                  end_value, type, clock)};
    obj.AddObjectMutator(ObjectMutator({std::move(mutator)}, -1, name));
  }

  void end() {
    Verify();
    ref_.get().EndObjectMutatorMatching(-1, name, 0);
  }

  int check() {
    Verify();
    return ref_.get().IsMutatorRunningMatching(-1, name) ? 1 : 0;
  }

  sr::Value wait(sr::VM& vm, std::vector<sr::Value>) {
    return WaitForObjectMutator(vm, ref_, -1, name, event_.get(), false);
  }

  sr::Value wait_key(sr::VM& vm, std::vector<sr::Value>) {
    return WaitForObjectMutator(vm, ref_, -1, name, event_.get(), true);
  }

 private:
  void Verify() {
    if (!graphics_)
      throw std::runtime_error("ObjEve requires a graphics system");
    if (!event_)
      throw std::runtime_error("ObjEve requires an event system");
  }
};

class ObjectRepnoEvent {
 public:
  using Getter = std::function<int(const ObjectParameter&, int)>;
  using Setter = std::function<void(ObjectParameter&, int, int)>;

  ObjectReference ref_;
  int repno_ = 0;
  std::shared_ptr<GraphicsSystem> graphics_;
  std::shared_ptr<EventSystem> event_;
  std::string name;
  Getter getter_;
  Setter setter_;

  ObjectRepnoEvent(ObjectReference ref,
                   int repno,
                   std::shared_ptr<GraphicsSystem> graphics,
                   std::shared_ptr<EventSystem> event,
                   std::string name,
                   Getter getter,
                   Setter setter)
      : ref_(std::move(ref)),
        repno_(repno),
        graphics_(std::move(graphics)),
        event_(std::move(event)),
        name(std::move(name)),
        getter_(std::move(getter)),
        setter_(std::move(setter)) {}

  void set(int end_value, int duration_time, int delay, int type) {
    Verify();
    GraphicsObject& obj = ref_.get();
    std::shared_ptr<Clock> clock = event_->GetClock();

    obj.EndObjectMutatorMatching(repno_, name, 0);
    const int start = getter_(obj.Param(), repno_);
    Mutator mutator{.setter_ = [setter = setter_, repno = repno_](
                                   ObjectParameter& param,
                                   int value) { setter(param, repno, value); },
                    .fc_ = MakeSiglusFrameCounter(duration_time, delay, start,
                                                  end_value, type, clock)};
    obj.AddObjectMutator(ObjectMutator({std::move(mutator)}, repno_, name));
  }

  void end() {
    Verify();
    ref_.get().EndObjectMutatorMatching(repno_, name, 0);
  }

  int check() {
    Verify();
    return ref_.get().IsMutatorRunningMatching(repno_, name) ? 1 : 0;
  }

  sr::Value wait(sr::VM& vm, std::vector<sr::Value>) {
    return WaitForObjectMutator(vm, ref_, repno_, name, event_.get(), false);
  }

  sr::Value wait_key(sr::VM& vm, std::vector<sr::Value>) {
    return WaitForObjectMutator(vm, ref_, repno_, name, event_.get(), true);
  }

 private:
  void Verify() {
    if (!graphics_)
      throw std::runtime_error("ObjectRepnoEvent requires a graphics system");
    if (!event_)
      throw std::runtime_error("ObjectRepnoEvent requires an event system");
  }
};

class ObjectChild {
 public:
  using Factory = std::function<sr::Value(ObjectReference)>;

  ObjectReference parent_;
  Factory make_object_;

  ObjectChild() = default;
  ObjectChild(ObjectReference parent, Factory make_object)
      : parent_(std::move(parent)), make_object_(std::move(make_object)) {}

  sr::Value get(int idx) {
    if (idx < 0) {
      throw sr::RuntimeError("object.child index is negative: " +
                             std::to_string(idx));
    }
    const std::size_t index = static_cast<std::size_t>(idx);
    GraphicsObject& parent = parent_.get();
    const std::size_t size = parent.GetChildren().size();
    if (index >= size) {
      throw sr::RuntimeError(
          "object.child index out of range: " + std::to_string(index) +
          " for size " + std::to_string(size));
    }
    if (!make_object_)
      throw std::runtime_error("ObjectChild requires an object factory");
    return make_object_(parent_.child(index));
  }

  void resize(int size) {
    if (size < 0) {
      throw sr::RuntimeError("object.child size is negative: " +
                             std::to_string(size));
    }
    parent_.get().ResetChildren(static_cast<std::size_t>(size));
  }

  int size() {
    const std::size_t size = parent_.get().GetChildren().size();
    if (size > static_cast<std::size_t>(std::numeric_limits<int>::max()))
      throw sr::RuntimeError("object.child size exceeds script integer range");
    return static_cast<int>(size);
  }
};

class ObjectIntList {
 public:
  ObjectReference ref_;
  std::size_t default_size_ = 0;

  ObjectIntList() = default;
  explicit ObjectIntList(ObjectReference ref) : ref_(std::move(ref)) {}

  int get(int idx) {
    return storage().Get(CheckIndex(idx, "object.F"));
  }

  void set(int idx, int value) {
    storage().Set(CheckIndex(idx, "object.F"), value);
  }

  void Set(int idx, std::vector<sr::Value> values) {
    const std::size_t begin = CheckIndex(idx, "object.F Set");
    if (values.empty())
      return;

    const std::size_t end =
        CheckedEnd(begin, values.size(), "object.F Set");
    if (end > storage().GetSize()) {
      throw sr::RuntimeError("object.F Set range [" +
                             std::to_string(begin) + ", " +
                             std::to_string(end) +
                             ") out of range for size " +
                             std::to_string(storage().GetSize()));
    }

    for (std::size_t i = 0; i < values.size(); ++i)
      storage().Set(begin + i, RequireInt(values[i], "object.F Set"));
  }

  void resize(int size) { storage().Resize(CheckSize(size, "object.F")); }

  int size() const {
    const std::size_t size = storage().GetSize();
    if (size > static_cast<std::size_t>(std::numeric_limits<int>::max()))
      throw sr::RuntimeError("object.F size exceeds script integer range");
    return static_cast<int>(size);
  }

  void fill(int begin, int end, int value) {
    const auto [begin_index, end_index] = CheckFillRange(begin, end);
    storage().Fill(begin_index, end_index, value);
  }

  void init() {
    storage().Resize(default_size_);
    storage().Fill(0, default_size_, 0);
  }

  int b1(int idx) { return get_bits(idx, 1); }
  void write_b1(int idx, int value) { set_bits(idx, value, 1); }
  int b2(int idx) { return get_bits(idx, 2); }
  void write_b2(int idx, int value) { set_bits(idx, value, 2); }
  int b4(int idx) { return get_bits(idx, 4); }
  void write_b4(int idx, int value) { set_bits(idx, value, 4); }
  int b8(int idx) { return get_bits(idx, 8); }
  void write_b8(int idx, int value) { set_bits(idx, value, 8); }
  int b16(int idx) { return get_bits(idx, 16); }
  void write_b16(int idx, int value) { set_bits(idx, value, 16); }

 private:
  IntBankStorage& storage() { return ref_.get().Param().siglus_f; }
  const IntBankStorage& storage() const { return ref_.get().Param().siglus_f; }

  static std::size_t CheckIndex(int idx, std::string_view where) {
    if (idx < 0)
      throw sr::RuntimeError(std::string(where) +
                             " index is negative: " + std::to_string(idx));
    return static_cast<std::size_t>(idx);
  }

  static std::size_t CheckSize(int size, std::string_view where) {
    if (size < 0)
      throw sr::RuntimeError(std::string(where) +
                             " size is negative: " + std::to_string(size));
    return static_cast<std::size_t>(size);
  }

  static std::size_t CheckedEnd(std::size_t begin,
                                std::size_t count,
                                std::string_view where) {
    if (count > std::numeric_limits<std::size_t>::max() - begin)
      throw sr::RuntimeError(std::string(where) + " index overflow");
    return begin + count;
  }

  std::pair<std::size_t, std::size_t> CheckFillRange(int begin, int end) {
    const std::size_t begin_index = CheckIndex(begin, "object.F fill");
    const std::size_t end_index = CheckIndex(end, "object.F fill");
    if (begin_index > end_index)
      throw sr::RuntimeError("object.F has invalid fill range");
    if (end_index > storage().GetSize()) {
      throw sr::RuntimeError("object.F fill end " + std::to_string(end_index) +
                             " out of range for size " +
                             std::to_string(storage().GetSize()));
    }
    return {begin_index, end_index};
  }

  std::size_t CheckBitIndex(int idx, std::uint8_t bits) {
    const std::size_t index = CheckIndex(idx, "object.F bit access");
    const std::size_t per_word = 32 / bits;
    const std::size_t words = storage().GetSize();
    if (words > std::numeric_limits<std::size_t>::max() / per_word)
      throw sr::RuntimeError("object.F bit access size overflow");
    const std::size_t logical_size = words * per_word;
    if (index >= logical_size) {
      throw sr::RuntimeError("object.F bit access index " +
                             std::to_string(index) +
                             " out of range for size " +
                             std::to_string(logical_size));
    }
    return index;
  }

  int get_bits(int idx, std::uint8_t bits) {
    return storage().Get(CheckBitIndex(idx, bits), bits);
  }

  void set_bits(int idx, int value, std::uint8_t bits) {
    storage().Set(CheckBitIndex(idx, bits), value, bits);
  }
};

class ObjectRepnoEventList {
 public:
  using Getter = ObjectRepnoEvent::Getter;
  using Setter = ObjectRepnoEvent::Setter;
  using Factory = std::function<
      sr::Value(ObjectReference, int, std::string, Getter, Setter)>;

  ObjectReference ref_;
  std::string name_;
  Getter getter_;
  Setter setter_;
  Factory make_event_;

  ObjectRepnoEventList() = default;
  ObjectRepnoEventList(ObjectReference ref,
                       std::string name,
                       Getter getter,
                       Setter setter,
                       Factory make_event)
      : ref_(std::move(ref)),
        name_(std::move(name)),
        getter_(std::move(getter)),
        setter_(std::move(setter)),
        make_event_(std::move(make_event)) {}

  sr::Value get(int idx) {
    if (idx < 0 || idx >= 8) {
      throw sr::RuntimeError("object rep event index out of range: " +
                             std::to_string(idx));
    }
    if (!make_event_)
      throw std::runtime_error(
          "ObjectRepnoEventList requires an event factory");
    return make_event_(ref_, idx, name_, getter_, setter_);
  }

  int size() { return 8; }
  void resize(int) {}
};

struct ObjectRepnoParam {
  using repno_t = std::array<int, 8>;
  ObjectReference ref_;
  std::function<repno_t&(ObjectParameter&)> fn_;
  ObjectRepnoParam(ObjectReference ref,
                   std::function<repno_t&(ObjectParameter&)> fn)
      : ref_(std::move(ref)), fn_(std::move(fn)) {}

  int size() { return 8; }
  void resize(int) {}
  sr::Value get(int idx) {
    GraphicsObject& obj = ref_.get();
    auto& arr = fn_(obj.Param());
    return arr.at(idx);
  }
  void set(int idx, int val) {
    GraphicsObject& obj = ref_.get();
    auto& arr = fn_(obj.Param());
    arr.at(idx) = val;
  }
};

struct DirectObjectPropertyBinder {
  sb::class_<SiglusObject>& obj;

  template <auto member>
  void Member(const char* name) {
    const std::string setter_name = std::string("set_") + name;
    obj.def(name, &SiglusObject::get_member<member>);
    obj.def(setter_name.c_str(), &SiglusObject::set_member<member>,
            sb::arg("value"));
  }

  template <typename Getter, typename Setter>
  void Property(const char* name, Getter getter, Setter setter) {
    const std::string setter_name = std::string("set_") + name;
    obj.def(name, [getter = std::move(getter)](const SiglusObject* object) {
      return getter(object);
    });
    obj.def(
        setter_name.c_str(),
        [setter = std::move(setter)](SiglusObject* object, int value) {
          setter(object, value);
        },
        sb::arg("value"));
  }

  void Bind() {
    Member<&ObjectParameter::wipe_copy>("wipe_copy");
    Member<&ObjectParameter::wipe_erase>("wipe_erase");
    Member<&ObjectParameter::click_disable>("click_disable");
    Member<&ObjectParameter::is_visible>("disp");
    Member<&ObjectParameter::pattern_number>("patno");
    Member<&ObjectParameter::z_order>("order");
    Member<&ObjectParameter::z_layer>("layer");
    Member<&ObjectParameter::position_x>("x");
    Member<&ObjectParameter::position_y>("y");
    Member<&ObjectParameter::z_depth>("z");
    Member<&ObjectParameter::origin_x>("center_x");
    Member<&ObjectParameter::origin_y>("center_y");
    Member<&ObjectParameter::repetition_origin_x>("center_rep_x");
    Member<&ObjectParameter::repetition_origin_y>("center_rep_y");
    Member<&ObjectParameter::high_quality_scale_x_percent>("scale_x");
    Member<&ObjectParameter::high_quality_scale_y_percent>("scale_y");
    Member<&ObjectParameter::rotation_div10>("rotate_z");

    Property(
        "clip_use",
        [](const auto* obj) { return obj->param().has_clip_rect() ? 1 : 0; },
        [](auto* obj, int value) {
          if (value) {
            if (!obj->param().has_clip_rect())
              obj->param().SetClipRect(Rect::GRP(0, 0, 0, 0));
          } else {
            obj->param().ClearClipRect();
          }
        });
    Property(
        "clip_left",
        [](const auto* obj) { return obj->param().clip_rect().x(); },
        [](auto* obj, int value) {
          obj->SetClipRectValue(&Rect::set_x, value);
        });
    Property(
        "clip_top",
        [](const auto* obj) { return obj->param().clip_rect().y(); },
        [](auto* obj, int value) {
          obj->SetClipRectValue(&Rect::set_y, value);
        });
    Property(
        "clip_right",
        [](const auto* obj) { return obj->param().clip_rect().x2(); },
        [](auto* obj, int value) {
          obj->SetClipRectValue(&Rect::set_x2, value);
        });
    Property(
        "clip_bottom",
        [](const auto* obj) { return obj->param().clip_rect().y2(); },
        [](auto* obj, int value) {
          obj->SetClipRectValue(&Rect::set_y2, value);
        });

    Property(
        "src_clip_use",
        [](const auto* obj) {
          return obj->param().has_own_clip_rect() ? 1 : 0;
        },
        [](auto* obj, int value) {
          if (value) {
            if (!obj->param().has_own_clip_rect())
              obj->param().SetOwnClipRect(Rect::GRP(0, 0, 0, 0));
          } else {
            obj->param().ClearOwnClipRect();
          }
        });
    Property(
        "src_clip_left",
        [](const auto* obj) { return obj->param().own_clip_rect().x(); },
        [](auto* obj, int value) {
          obj->SetOwnClipRectValue(&Rect::set_x, value);
        });
    Property(
        "src_clip_top",
        [](const auto* obj) { return obj->param().own_clip_rect().y(); },
        [](auto* obj, int value) {
          obj->SetOwnClipRectValue(&Rect::set_y, value);
        });
    Property(
        "src_clip_right",
        [](const auto* obj) { return obj->param().own_clip_rect().x2(); },
        [](auto* obj, int value) {
          obj->SetOwnClipRectValue(&Rect::set_x2, value);
        });
    Property(
        "src_clip_bottom",
        [](const auto* obj) { return obj->param().own_clip_rect().y2(); },
        [](auto* obj, int value) {
          obj->SetOwnClipRectValue(&Rect::set_y2, value);
        });

    Member<&ObjectParameter::alpha_source>("tr");
    Member<&ObjectParameter::monochrome_transform>("mono");
    Member<&ObjectParameter::invert_transform>("reverse");
    Property(
        "bright", [](const auto* obj) { return obj->param().Bright(); },
        [](auto* obj, int value) { obj->param().SetBright(value); });
    Property(
        "dark", [](const auto* obj) { return obj->param().Dark(); },
        [](auto* obj, int value) { obj->param().SetDark(value); });

    Property(
        "color_r", [](const auto* obj) { return obj->param().colour_red(); },
        [](auto* obj, int value) { obj->param().SetColourRed(value); });
    Property(
        "color_g", [](const auto* obj) { return obj->param().colour_green(); },
        [](auto* obj, int value) { obj->param().SetColourGreen(value); });
    Property(
        "color_b", [](const auto* obj) { return obj->param().colour_blue(); },
        [](auto* obj, int value) { obj->param().SetColourBlue(value); });
    Property(
        "color_rate",
        [](const auto* obj) { return obj->param().colour_level(); },
        [](auto* obj, int value) { obj->param().SetColourLevel(value); });
    Property(
        "color_add_r", [](const auto* obj) { return obj->param().tint_red(); },
        [](auto* obj, int value) { obj->param().SetTintRed(value); });
    Property(
        "color_add_g",
        [](const auto* obj) { return obj->param().tint_green(); },
        [](auto* obj, int value) { obj->param().SetTintGreen(value); });
    Property(
        "color_add_b", [](const auto* obj) { return obj->param().tint_blue(); },
        [](auto* obj, int value) { obj->param().SetTintBlue(value); });

    Member<&ObjectParameter::mask_no>("mask_no");
    Member<&ObjectParameter::tonecurve_no>("tonecurve_no");
    Member<&ObjectParameter::culling>("culling");
    Member<&ObjectParameter::alpha_test>("alpha_test");
    Member<&ObjectParameter::alpha_blend>("alpha_blend");
    Member<&ObjectParameter::light_no>("light_no");
    Member<&ObjectParameter::fog_use>("fog_use");
    Property(
        "blend", [](const auto* obj) { return obj->param().composite_mode; },
        [](auto* obj, int value) {
          obj->param().SetCompositeMode(std::clamp(value, 0, 4));
        });
  }
};

struct ObjectEventPropertyBinder {
  sb::class_<SiglusObject>& obj;
  sb::class_<ObjectEvent>& event_class;
  std::shared_ptr<GraphicsSystem> graphics;
  std::shared_ptr<EventSystem> event;

  template <auto member>
  void Member(std::string name) {
    Property(std::move(name), CreateGetter<member>(), CreateSetter<member>());
  }

  template <typename Getter, typename Setter>
  void Property(std::string name, Getter getter, Setter setter) {
    std::function<int(const ObjectParameter&)> property_getter =
        std::move(getter);
    std::function<void(ObjectParameter&, int)> property_setter =
        std::move(setter);
    std::string field_name = name;
    obj.subcls(field_name, event_class,
               [name = std::move(name), graphics = graphics, event = event,
                getter = std::move(property_getter),
                setter = std::move(property_setter)](SiglusObject* parent) {
                 auto ret = std::make_unique<ObjectEvent>();
                 ret->ref_ = parent->ref_;
                 ret->graphics_ = graphics;
                 ret->event_ = event;
                 ret->name = name;
                 ret->getter_ = getter;
                 ret->setter_ = setter;
                 return ret;
               });
  }

  static void SetClipRectValue(ObjectParameter& param,
                               void (Rect::*setter)(int),
                               int value) {
    Rect rect =
        param.has_clip_rect() ? param.clip_rect() : Rect::GRP(0, 0, 0, 0);
    (rect.*setter)(value);
    param.SetClipRect(rect);
  }

  static void SetOwnClipRectValue(ObjectParameter& param,
                                  void (Rect::*setter)(int),
                                  int value) {
    Rect rect = param.has_own_clip_rect() ? param.own_clip_rect()
                                          : Rect::GRP(0, 0, 0, 0);
    (rect.*setter)(value);
    param.SetOwnClipRect(rect);
  }

  void Bind() {
    Member<&ObjectParameter::pattern_number>("patno_eve");
    Member<&ObjectParameter::position_x>("x_eve");
    Member<&ObjectParameter::position_y>("y_eve");
    Member<&ObjectParameter::z_depth>("z_eve");
    Member<&ObjectParameter::origin_x>("center_x_eve");
    Member<&ObjectParameter::origin_y>("center_y_eve");
    Member<&ObjectParameter::repetition_origin_x>("center_rep_x_eve");
    Member<&ObjectParameter::repetition_origin_y>("center_rep_y_eve");
    Member<&ObjectParameter::high_quality_scale_x_percent>("scale_x_eve");
    Member<&ObjectParameter::high_quality_scale_y_percent>("scale_y_eve");
    Member<&ObjectParameter::rotation_div10>("rotate_z_eve");

    Property(
        "clip_left_eve",
        [](const ObjectParameter& param) { return param.clip_rect().x(); },
        [](ObjectParameter& param, int value) {
          SetClipRectValue(param, &Rect::set_x, value);
        });
    Property(
        "clip_top_eve",
        [](const ObjectParameter& param) { return param.clip_rect().y(); },
        [](ObjectParameter& param, int value) {
          SetClipRectValue(param, &Rect::set_y, value);
        });
    Property(
        "clip_right_eve",
        [](const ObjectParameter& param) { return param.clip_rect().x2(); },
        [](ObjectParameter& param, int value) {
          SetClipRectValue(param, &Rect::set_x2, value);
        });
    Property(
        "clip_bottom_eve",
        [](const ObjectParameter& param) { return param.clip_rect().y2(); },
        [](ObjectParameter& param, int value) {
          SetClipRectValue(param, &Rect::set_y2, value);
        });
    Property(
        "src_clip_left_eve",
        [](const ObjectParameter& param) { return param.own_clip_rect().x(); },
        [](ObjectParameter& param, int value) {
          SetOwnClipRectValue(param, &Rect::set_x, value);
        });
    Property(
        "src_clip_top_eve",
        [](const ObjectParameter& param) { return param.own_clip_rect().y(); },
        [](ObjectParameter& param, int value) {
          SetOwnClipRectValue(param, &Rect::set_y, value);
        });
    Property(
        "src_clip_right_eve",
        [](const ObjectParameter& param) { return param.own_clip_rect().x2(); },
        [](ObjectParameter& param, int value) {
          SetOwnClipRectValue(param, &Rect::set_x2, value);
        });
    Property(
        "src_clip_bottom_eve",
        [](const ObjectParameter& param) { return param.own_clip_rect().y2(); },
        [](ObjectParameter& param, int value) {
          SetOwnClipRectValue(param, &Rect::set_y2, value);
        });

    Member<&ObjectParameter::alpha_source>("tr_eve");
    Member<&ObjectParameter::monochrome_transform>("mono_eve");
    Member<&ObjectParameter::invert_transform>("reverse_eve");
    Property(
        "bright_eve",
        [](const ObjectParameter& param) { return param.Bright(); },
        [](ObjectParameter& param, int value) { param.SetBright(value); });
    Property(
        "dark_eve", [](const ObjectParameter& param) { return param.Dark(); },
        [](ObjectParameter& param, int value) { param.SetDark(value); });

    Property(
        "color_r_eve",
        [](const ObjectParameter& param) { return param.colour_red(); },
        [](ObjectParameter& param, int value) { param.SetColourRed(value); });
    Property(
        "color_g_eve",
        [](const ObjectParameter& param) { return param.colour_green(); },
        [](ObjectParameter& param, int value) { param.SetColourGreen(value); });
    Property(
        "color_b_eve",
        [](const ObjectParameter& param) { return param.colour_blue(); },
        [](ObjectParameter& param, int value) { param.SetColourBlue(value); });
    Property(
        "color_rate_eve",
        [](const ObjectParameter& param) { return param.colour_level(); },
        [](ObjectParameter& param, int value) { param.SetColourLevel(value); });
    Property(
        "color_add_r_eve",
        [](const ObjectParameter& param) { return param.tint_red(); },
        [](ObjectParameter& param, int value) { param.SetTintRed(value); });
    Property(
        "color_add_g_eve",
        [](const ObjectParameter& param) { return param.tint_green(); },
        [](ObjectParameter& param, int value) { param.SetTintGreen(value); });
    Property(
        "color_add_b_eve",
        [](const ObjectParameter& param) { return param.tint_blue(); },
        [](ObjectParameter& param, int value) { param.SetTintBlue(value); });
  }
};

void BindObject(SiglusRuntime& runtime) {
  auto& vm = *runtime.vm;
  sb::module_ m(vm.gc_.get(), vm.globals_.get());
  sb::class_<SiglusObject> obj(m, "Object");
  sb::class_<ObjectChild> child(m, "ObjectChild", false);
  sb::class_<ObjectIntList> object_int_list(m, "ObjectIntList", false);
  sb::class_<ObjectRepnoParam> repno(m, "ObjectRepnoParam", false);
  sb::class_<ObjectRepnoEvent> repno_event(m, "ObjectRepnoEvent", false);
  sb::class_<ObjectRepnoEventList> repno_event_list(m, "ObjectRepnoEventList",
                                                    false);
  repno_event_list.add_gc_root(repno_event);

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

  // child object
  child.def("__getitem__", &ObjectChild::get, sb::arg("idx"));
  child.def("resize", &ObjectChild::resize, sb::arg("size"));
  child.def("size", &ObjectChild::size);

  ObjectChild::Factory make_child_object =
      [object_class = obj, graphics, event,
       asset_scanner](ObjectReference ref) mutable -> sr::Value {
    return sr::Value(
        object_class.make_inst(std::move(ref), graphics, event, asset_scanner));
  };
  obj.subcls("child", child,
             [make_child_object](
                 SiglusObject* parent) -> std::unique_ptr<ObjectChild> {
               return std::make_unique<ObjectChild>(parent->ref_,
                                                    make_child_object);
             });

  object_int_list.def("__getitem__", &ObjectIntList::get, sb::arg("idx"));
  object_int_list.def("__setitem__", &ObjectIntList::set, sb::arg("idx"),
                      sb::arg("val"));
  object_int_list.def("Set", &ObjectIntList::Set, sb::arg("idx"), sb::vararg);
  object_int_list.def("resize", &ObjectIntList::resize, sb::arg("size"));
  object_int_list.def("size", &ObjectIntList::size);
  object_int_list.def("fill", &ObjectIntList::fill, sb::arg("begin"),
                      sb::arg("end"), sb::arg("val"));
  object_int_list.def("init", &ObjectIntList::init);
  object_int_list.def("b1", &ObjectIntList::b1, sb::arg("idx"));
  object_int_list.def("write_b1", &ObjectIntList::write_b1, sb::arg("idx"),
                      sb::arg("val"));
  object_int_list.def("b2", &ObjectIntList::b2, sb::arg("idx"));
  object_int_list.def("write_b2", &ObjectIntList::write_b2, sb::arg("idx"),
                      sb::arg("val"));
  object_int_list.def("b4", &ObjectIntList::b4, sb::arg("idx"));
  object_int_list.def("write_b4", &ObjectIntList::write_b4, sb::arg("idx"),
                      sb::arg("val"));
  object_int_list.def("b8", &ObjectIntList::b8, sb::arg("idx"));
  object_int_list.def("write_b8", &ObjectIntList::write_b8, sb::arg("idx"),
                      sb::arg("val"));
  object_int_list.def("b16", &ObjectIntList::b16, sb::arg("idx"));
  object_int_list.def("write_b16", &ObjectIntList::write_b16, sb::arg("idx"),
                      sb::arg("val"));
  obj.subcls("F", object_int_list,
             [](SiglusObject* parent) -> std::unique_ptr<ObjectIntList> {
               return std::make_unique<ObjectIntList>(parent->ref_);
             });

  // direct properties
  DirectObjectPropertyBinder direct_properties{obj};
  direct_properties.Bind();

  repno.def("__getitem__", &ObjectRepnoParam::get, sb::arg("idx"));
  repno.def("__setitem__", &ObjectRepnoParam::set, sb::arg("idx"),
            sb::arg("val"));
  repno.def("resize", &ObjectRepnoParam::resize, sb::arg("size"));
  repno.def("size", &ObjectRepnoParam::size);

  obj.subcls("x_rep", repno,
             [](SiglusObject* obj) -> std::unique_ptr<ObjectRepnoParam> {
               auto fn = +[](ObjectParameter& param) -> std::array<int, 8>& {
                 return param.adjustment_offsets_x;
               };
               return std::make_unique<ObjectRepnoParam>(obj->ref_,
                                                         std::move(fn));
             });
  obj.subcls("y_rep", repno,
             [](SiglusObject* obj) -> std::unique_ptr<ObjectRepnoParam> {
               auto fn = +[](ObjectParameter& param) -> std::array<int, 8>& {
                 return param.adjustment_offsets_y;
               };
               return std::make_unique<ObjectRepnoParam>(obj->ref_,
                                                         std::move(fn));
             });
  obj.subcls("tr_rep", repno,
             [](SiglusObject* obj) -> std::unique_ptr<ObjectRepnoParam> {
               auto fn = +[](ObjectParameter& param) -> std::array<int, 8>& {
                 return param.adjustment_alphas;
               };
               return std::make_unique<ObjectRepnoParam>(obj->ref_,
                                                         std::move(fn));
             });

  repno_event.def("set", &ObjectRepnoEvent::set, sb::arg("end_value"),
                  sb::arg("duration_time"), sb::arg("delay"), sb::arg("type"));
  repno_event.def("end", &ObjectRepnoEvent::end);
  repno_event.def("check", &ObjectRepnoEvent::check);
  repno_event.def("wait", &ObjectRepnoEvent::wait, sb::vararg);
  repno_event.def("wait_key", &ObjectRepnoEvent::wait_key, sb::vararg);

  repno_event_list.def("__getitem__", &ObjectRepnoEventList::get,
                       sb::arg("idx"));
  repno_event_list.def("resize", &ObjectRepnoEventList::resize,
                       sb::arg("size"));
  repno_event_list.def("size", &ObjectRepnoEventList::size);

  ObjectRepnoEventList::Factory make_repno_event =
      [event_class = repno_event, graphics, event](
          ObjectReference ref, int repno, std::string name,
          ObjectRepnoEventList::Getter getter,
          ObjectRepnoEventList::Setter setter) mutable -> sr::Value {
    return sr::Value(event_class.make_inst(
        std::move(ref), repno, graphics, event, std::move(name),
        std::move(getter), std::move(setter)));
  };

  auto bind_repno_event_list = [&](const char* name,
                                   ObjectRepnoEventList::Getter getter,
                                   ObjectRepnoEventList::Setter setter) {
    obj.subcls(
        name, repno_event_list,
        [name = std::string(name), getter = std::move(getter),
         setter = std::move(setter), make_repno_event](SiglusObject* parent) {
          return std::make_unique<ObjectRepnoEventList>(
              parent->ref_, name, getter, setter, make_repno_event);
        });
  };
  bind_repno_event_list("x_rep_eve",
                        CreateGetter<&ObjectParameter::adjustment_offsets_x>(),
                        CreateSetter<&ObjectParameter::adjustment_offsets_x>());
  bind_repno_event_list("y_rep_eve",
                        CreateGetter<&ObjectParameter::adjustment_offsets_y>(),
                        CreateSetter<&ObjectParameter::adjustment_offsets_y>());
  bind_repno_event_list("tr_rep_eve",
                        CreateGetter<&ObjectParameter::adjustment_alphas>(),
                        CreateSetter<&ObjectParameter::adjustment_alphas>());

  obj.def("init", [](SiglusObject* obj) {
    obj->object().FreeDataAndInitializeParams();
  });
  obj.def("init_param",
          [](SiglusObject* obj) { obj->object().InitializeParams(); });
  obj.def("free", [](SiglusObject* obj) { obj->object().FreeObjectData(); });
  obj.def(
      "create",
      [](SiglusObject* obj, std::vector<sr::Value> args) -> void {
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
  obj.def("load_gan", &SiglusObject::load_gan);
  obj.def("start_gan", &SiglusObject::start_gan, sb::vararg);
  obj.def("create_rect", &SiglusObject::create_rect);
  obj.def("exist_type",
          [](SiglusObject* obj) { return obj->object().HasDrawer() ? 1 : 0; });
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
  obj.def("get_file_path",
          [](SiglusObject* obj) { return obj->object().FilePath(); });
  obj.def("set_center_rep", &SiglusObject::set_center_rep);
  obj.def("set_scale", &SiglusObject::set_scale);
  obj.def("set_pos", &SiglusObject::set_pos);
  obj.def("clear_button", [](SiglusObject* obj) {
    ButtonProperties button;
    button.action = -1;
    button.se = -1;
    obj->param().SetButtonProperty(button);
  });
  obj.def(
      "set_button",
      [](SiglusObject* obj, std::vector<sr::Value> raw_args) {
        CallPacket packet = CallPacket::DecodeFrom(std::move(raw_args));
        const std::vector<sr::Value>& args = packet.args;
        if (args.empty() || args.size() > 4) {
          throw std::runtime_error("Object.set_button expects 1 to 4 args");
        }

        int button_no = 0;
        int group_no = 0;
        int action_no = 0;
        int se_no = 0;
        if (args.size() >= 1)
          button_no = RequireInt(args[0], "Object.set_button button_no");
        if (args.size() >= 2)
          group_no = RequireInt(args[1], "Object.set_button group_no");
        if (args.size() >= 3)
          action_no = RequireInt(args[2], "Object.set_button action_no");
        if (args.size() >= 4)
          se_no = RequireInt(args[3], "Object.set_button se_no");

        obj->param().SetButtonOpts(action_no, se_no, group_no, button_no);
      },
      sb::vararg);
  obj.def(
      "set_button_group",
      [](SiglusObject* obj, std::vector<sr::Value> raw_args) {
        CallPacket packet = CallPacket::DecodeFrom(std::move(raw_args));
        const std::vector<sr::Value>& args = packet.args;
        if (args.size() != 1)
          throw std::runtime_error("Object.set_button_group expects 1 arg");

        ButtonProperties button = obj->param().ButtonProperty();
        button.group = RequireInt(args[0], "Object.set_button_group group_no");
        obj->param().SetButtonProperty(button);
      },
      sb::vararg);
  obj.def("set_button_state_normal",
          [](SiglusObject* obj) { obj->param().SetButtonState(kBtnNormal); });
  obj.def("set_button_state_select",
          [](SiglusObject* obj) { obj->param().SetButtonState(kBtnSelect); });
  obj.def("set_button_state_disable",
          [](SiglusObject* obj) { obj->param().SetButtonState(kBtnDisable); });
  obj.def("get_button_state",
          [](SiglusObject* obj) { return obj->param().GetButtonState(); });
  obj.def("get_button_hit_state", [](SiglusObject* obj) {
    const int state = obj->param().GetButtonState();
    if (state == kBtnSelect || state == kBtnDisable)
      return state;
    return kBtnNormal;
  });
  obj.def("get_button_real_state", [](SiglusObject* obj) {
    const int state = obj->param().GetButtonState();
    if (state == kBtnSelect || state == kBtnDisable)
      return state;
    return kBtnNormal;
  });
  obj.def("clear_button_call", [](SiglusObject*) {});
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

  sb::class_<ObjectEvent> oe(m, "ObjectEvent", false);
  oe.def("set", &ObjectEvent::set, sb::arg("end_value"),
         sb::arg("duration_time"), sb::arg("delay"), sb::arg("type"));
  oe.def("end", &ObjectEvent::end);
  oe.def("check", &ObjectEvent::check);
  oe.def("wait", &ObjectEvent::wait, sb::vararg);
  oe.def("wait_key", &ObjectEvent::wait_key, sb::vararg);

  ObjectEventPropertyBinder event_properties{obj, oe, graphics, event};
  event_properties.Bind();
}

RLVM_REGISTER(SiglusBindingRegistry, "0_object", BindObject)

}  // namespace libsiglus::binding
