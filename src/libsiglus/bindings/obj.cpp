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

#include <GL/gl.h>

#include "libsiglus/bindings/obj.hpp"
#include "srbind/srbind.hpp"

#include "core/avdec/anm.hpp"
#include "core/avdec/image_decoder.hpp"
#include "core/frame_counter.hpp"
#include "core/rect.hpp"
#include "object/drawer/file.hpp"
#include "object/object_mutator.hpp"
#include "systems/base/graphics_object.hpp"
#include "systems/screen_canvas.hpp"
#include "systems/sdl/sdl_system.hpp"
#include "systems/sdl/shaders.hpp"
#include "systems/sdl_surface.hpp"
#include "utilities/mapped_file.hpp"

#include <SDL/SDL.h>
#include <set>

namespace libsiglus::binding {
namespace sr = serilang;
namespace sb = srbind;

class Object {
 public:
  enum MaskType { NO_MASK, ALPHA_MASK, COLOR_MASK };

  Object(std::shared_ptr<AssetScanner> s) : scanner(s) {}

  void Init() { obj.FreeDataAndInitializeParams(); }

  void Create(std::string filename, bool disp) {
    fs::path pth = ResolvePath(filename);
    if (pth.empty() || !fs::exists(pth))
      throw std::runtime_error(
          std::format("Could not find Object compatible file '{}'", filename));

    std::unique_ptr<GraphicsObjectOfFile> obj_data = LoadObjData(pth);
    obj.SetObjectData(std::move(obj_data));
    obj.Param().SetVisible(disp);
  }

  void Render() {
    obj.ExecuteMutators();

    glRenderer renderer;
    renderer.SetUp();
    renderer.ClearBuffer(std::make_shared<ScreenCanvas>(Size(1920, 1080)),
                         RGBAColour(0, 0, 0, 255));

    glViewport(0, 0, 1920, 1080);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, (GLdouble)1920, (GLdouble)1080, 0.0, 0.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    obj.Render(0, nullptr);

    glFlush();
    SDL_GL_SwapBuffers();
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

 private:
  fs::path ResolvePath(std::string const& filename) {
    static const std::set<std::string> OBJ_FILETYPES = {"anm", "g00", "pdt"};
    if (auto f = scanner->FindFile(filename, OBJ_FILETYPES); f.has_value())
      return f.value();
    else
      throw f.error();
  }

  std::unique_ptr<GraphicsObjectOfFile> LoadObjData(fs::path const& pth) {
    const std::string filename = pth.string();

    if (!(pth.extension() == ".g00" || pth.extension() == ".pdt")) {
      throw std::runtime_error(std::format(
          "Don't know how to handle object file '{}'", pth.string()));
    }

    MappedFile file(pth);
    ImageDecoder dec(file.Read());
    const auto w = dec.width, h = dec.height;

    char* mem = dec.mem.data();
    MaskType mask = dec.ismask ? ALPHA_MASK : NO_MASK;
    if (mask == ALPHA_MASK) {
      int len = w * h;
      uint32_t* d = reinterpret_cast<uint32_t*>(mem);
      int i;
      for (i = 0; i < len; i++) {
        if ((*d & 0xff000000) != 0xff000000)
          break;
        d++;
      }
      if (i == len) {
        mask = NO_MASK;
      }
    }

    SDL_Surface *s, *tmp = SDL_CreateRGBSurfaceFrom(
                        mem, w, h, 32, w * 4, 0xff0000, 0xff00, 0xff,
                        mask == ALPHA_MASK ? 0xff000000 : 0x0);
    Uint32 flags =
        mask == ALPHA_MASK
            ? (tmp->flags & (SDL_SRCALPHA | SDL_RLEACCELOK))
            : (tmp->flags & (SDL_SRCCOLORKEY | SDL_SRCALPHA | SDL_RLEACCELOK));
    s = SDL_ConvertSurface(tmp, tmp->format, flags);
    SDL_FreeSurface(tmp);

    // Grab the Type-2 information out of the converter or create one
    // default region if none exist
    std::vector<GrpRect> region_table = dec.region_table;
    if (region_table.empty()) {
      GrpRect rect;
      rect.rect = Rect(Point(0, 0), Size(w, h));
      rect.originX = 0;
      rect.originY = 0;
      region_table.push_back(rect);
    }

    auto surface = std::make_shared<SDLSurface>(s, std::move(region_table));
    // TODO: handle tone curve effect loading

    return std::make_unique<GraphicsObjectOfFile>(surface);
  }

  std::shared_ptr<AssetScanner> scanner;
  GraphicsObject obj;
  std::shared_ptr<Clock> clock = std::make_shared<Clock>();
};

void Obj::Bind(SiglusRuntime& runtime) {
  sr::VM& vm = *runtime.vm;

  sb::module_ m(vm.gc_.get(), vm.globals_);
  sb::class_<Object> o(m, "Object");

  o.def(sb::init([scanner = ctx.asset_scanner]() -> Object* {
     return new Object(scanner);
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
