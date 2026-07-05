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

#include "libsiglus/sgvm_factory.hpp"

#include "core/asset_scanner.hpp"
#include "core/gameexe.hpp"
#include "core/stage.hpp"
#include "libsiglus/archive.hpp"
#include "libsiglus/bindings/loader.hpp"
#include "libsiglus/bindings/registry.hpp"
#include "libsiglus/gexedat.hpp"
#include "libsiglus/intern_name.hpp"
#include "libsiglus/siglus_scene_renderer.hpp"
#include "log/domain_logger.hpp"
#include "m6/vm_factory.hpp"
#include "srbind/module.hpp"
#include "systems/event_system.hpp"
#include "systems/graphics_system.hpp"
#include "systems/system.hpp"
#include "systems/text_system.hpp"
#include "utilities/file.hpp"
#include "utilities/mapped_file.hpp"
#include "utilities/string_utilities.hpp"
#include "vm/exception.hpp"
#include "vm/object.hpp"
#include "vm/string.hpp"
#include "vm/vm.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <format>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace libsiglus {
namespace chr = std::chrono;
namespace fs = std::filesystem;
namespace sr = serilang;
namespace sb = srbind;

static DomainLogger logger("SiglusFactory");

namespace {

constexpr int kSiglusDefaultMwndCount = 2;
constexpr int kSiglusMaxMwndCount = 256;

void SetIntVecIfMissing(Gameexe& gexe,
                        std::string_view key,
                        std::initializer_list<int> values) {
  if (gexe.Exists(key))
    return;

  std::vector<GexeVal> gexe_values;
  gexe_values.reserve(values.size());
  for (int value : values)
    gexe_values.emplace_back(value);
  gexe.SetAt(key, std::move(gexe_values));
}

void SetIntVec(Gameexe& gexe, std::string_view key, std::vector<int> values) {
  std::vector<GexeVal> gexe_values;
  gexe_values.reserve(values.size());
  for (int value : values)
    gexe_values.emplace_back(value);
  gexe.SetAt(key, std::move(gexe_values));
}

std::optional<int> ReadInt(Gameexe& gexe, std::string_view key) {
  auto result = gexe(std::string(key)).Int();
  if (!result)
    return std::nullopt;
  return result.value();
}

std::optional<std::vector<int>> ReadIntVec(Gameexe& gexe,
                                           std::string_view key) {
  auto result = gexe(std::string(key)).IntVec();
  if (!result)
    return std::nullopt;
  return std::move(result.value());
}

std::string MwndKey(int index, std::string_view suffix, bool padded) {
  if (padded)
    return std::format("MWND.{:03}.{}", index, suffix);
  return std::format("MWND.{}.{}", index, suffix);
}

std::string WindowKey(int index, std::string_view suffix) {
  return std::format("WINDOW.{:03}.{}", index, suffix);
}

std::optional<int> ReadMwndInt(Gameexe& gexe,
                               int index,
                               std::string_view suffix) {
  std::array keys{MwndKey(index, suffix, true), MwndKey(index, suffix, false)};
  for (const auto& key : keys) {
    if (auto value = ReadInt(gexe, key))
      return value;
  }
  return std::nullopt;
}

std::optional<std::vector<int>> ReadMwndIntVec(Gameexe& gexe,
                                               int index,
                                               std::string_view suffix) {
  std::array keys{MwndKey(index, suffix, true), MwndKey(index, suffix, false)};
  for (const auto& key : keys) {
    if (auto value = ReadIntVec(gexe, key))
      return value;
  }
  return std::nullopt;
}

void ApplyPair(std::optional<std::vector<int>> values, int& x, int& y) {
  if (!values || values->size() < 2)
    return;

  x = values->at(0);
  y = values->at(1);
}

void ApplyRect(std::optional<std::vector<int>> values,
               int& left,
               int& top,
               int& right,
               int& bottom) {
  if (!values || values->size() < 4)
    return;

  left = values->at(0);
  top = values->at(1);
  right = values->at(2);
  bottom = values->at(3);
}

struct SiglusMwndSub {
  int extend_type = 0;
  int window_x = 50;
  int window_y = 400;
  int window_width = 700;
  int window_height = 150;
  int message_x = 20;
  int message_y = 20;
  int margin_left = 20;
  int margin_top = 20;
  int margin_right = 20;
  int margin_bottom = 20;
  int moji_count_x = 26;
  int moji_count_y = 3;
  int moji_size = 25;
  int moji_space_x = -1;
  int moji_space_y = 10;
  int ruby_size = 10;
  int waku_no = 0;
  int name_disp_mode = 0;
  int name_extend_type = 0;
  int name_window_x = 0;
  int name_window_y = -100;
  int name_message_x = 8;
  int name_message_y = 8;
  int name_margin_left = 8;
  int name_margin_top = 8;
  int name_margin_right = 8;
  int name_margin_bottom = 8;
  int name_moji_size = 16;
  int name_moji_space_x = -1;
  int name_moji_space_y = 8;
  int name_moji_count = 8;
  int name_waku_no = -1;
};

SiglusMwndSub LoadSiglusMwndSub(Gameexe& gexe, int index) {
  SiglusMwndSub sub;

  if (auto value = ReadMwndInt(gexe, index, "EXTEND_TYPE"))
    sub.extend_type = *value;
  ApplyPair(ReadMwndIntVec(gexe, index, "WINDOW_POS"), sub.window_x,
            sub.window_y);
  ApplyPair(ReadMwndIntVec(gexe, index, "WINDOW_SIZE"), sub.window_width,
            sub.window_height);
  ApplyPair(ReadMwndIntVec(gexe, index, "MESSAGE_POS"), sub.message_x,
            sub.message_y);
  ApplyRect(ReadMwndIntVec(gexe, index, "MESSAGE_MARGIN"), sub.margin_left,
            sub.margin_top, sub.margin_right, sub.margin_bottom);
  ApplyPair(ReadMwndIntVec(gexe, index, "MOJI_CNT"), sub.moji_count_x,
            sub.moji_count_y);
  if (auto value = ReadMwndInt(gexe, index, "MOJI_SIZE"))
    sub.moji_size = *value;
  ApplyPair(ReadMwndIntVec(gexe, index, "MOJI_SPACE"), sub.moji_space_x,
            sub.moji_space_y);
  if (auto value = ReadMwndInt(gexe, index, "RUBY_SIZE"))
    sub.ruby_size = *value;
  if (auto value = ReadMwndInt(gexe, index, "WAKU_NO"))
    sub.waku_no = *value;

  if (auto value = ReadMwndInt(gexe, index, "NAME_DISP_MODE"))
    sub.name_disp_mode = *value;
  if (auto value = ReadMwndInt(gexe, index, "NAME_EXTEND_TYPE"))
    sub.name_extend_type = *value;
  ApplyPair(ReadMwndIntVec(gexe, index, "NAME_WINDOW_POS"), sub.name_window_x,
            sub.name_window_y);
  ApplyPair(ReadMwndIntVec(gexe, index, "NAME_MESSAGE_POS"), sub.name_message_x,
            sub.name_message_y);
  ApplyRect(ReadMwndIntVec(gexe, index, "NAME_MESSAGE_MARGIN"),
            sub.name_margin_left, sub.name_margin_top, sub.name_margin_right,
            sub.name_margin_bottom);
  if (auto value = ReadMwndInt(gexe, index, "NAME_MOJI_SIZE"))
    sub.name_moji_size = *value;
  ApplyPair(ReadMwndIntVec(gexe, index, "NAME_MOJI_SPACE"),
            sub.name_moji_space_x, sub.name_moji_space_y);
  if (auto value = ReadMwndInt(gexe, index, "NAME_MOJI_CNT"))
    sub.name_moji_count = *value;
  if (auto value = ReadMwndInt(gexe, index, "NAME_WAKU_NO"))
    sub.name_waku_no = *value;

  return sub;
}

int LogicalTextWidth(const SiglusMwndSub& sub) {
  const int pitch = std::max(sub.moji_size + sub.moji_space_x, 0);
  return std::max(sub.moji_count_x, 0) * pitch;
}

int LogicalTextHeight(const SiglusMwndSub& sub) {
  const int pitch =
      std::max(sub.moji_size + sub.moji_space_y + sub.ruby_size, 0);
  return std::max(sub.moji_count_y, 0) * pitch;
}

std::vector<int> BuildMojiPos(const SiglusMwndSub& sub) {
  if (sub.extend_type == 1) {
    return {sub.margin_top, sub.margin_bottom, sub.margin_left,
            sub.margin_right};
  }

  const int top = std::max(sub.message_y, 0);
  const int left = std::max(sub.message_x, 0);
  const int right =
      std::max(sub.window_width - sub.message_x - LogicalTextWidth(sub), 0);
  const int bottom =
      std::max(sub.window_height - sub.message_y - LogicalTextHeight(sub), 0);
  return {top, bottom, left, right};
}

int ConvertNameMod(const SiglusMwndSub& sub) {
  switch (sub.name_disp_mode) {
    case 0:
      return sub.name_waku_no >= 0 ? 1 : 0;
    case 1:
      return 0;
    case 2:
      return 2;
    default:
      return 0;
  }
}

std::vector<int> BuildNameMojiPos(const SiglusMwndSub& sub) {
  if (sub.name_extend_type == 1)
    return {sub.name_margin_left, sub.name_margin_top};
  return {sub.name_message_x, sub.name_message_y};
}

void WriteRealliveWindow(Gameexe& gexe, int index, const SiglusMwndSub& sub) {
  SetIntVec(gexe, WindowKey(index, "ATTR_MOD"), {0});
  SetIntVec(gexe, WindowKey(index, "ATTR"), {255, 255, 255, 255, 0});
  SetIntVec(gexe, WindowKey(index, "MOJI_SIZE"), {sub.moji_size});
  SetIntVec(gexe, WindowKey(index, "MOJI_CNT"),
            {sub.moji_count_x, sub.moji_count_y});
  SetIntVec(gexe, WindowKey(index, "MOJI_REP"),
            {sub.moji_space_x, sub.moji_space_y});
  SetIntVec(gexe, WindowKey(index, "LUBY_SIZE"), {sub.ruby_size});
  SetIntVec(gexe, WindowKey(index, "MOJI_POS"), BuildMojiPos(sub));
  SetIntVec(gexe, WindowKey(index, "POS"), {0, sub.window_x, sub.window_y});
  SetIntVec(gexe, WindowKey(index, "INDENT_USE"), {1});
  SetIntVec(gexe, WindowKey(index, "NAME_MOD"), {ConvertNameMod(sub)});
  SetIntVec(gexe, WindowKey(index, "KEYCUR_MOD"), {0, 0, 0});
  SetIntVec(gexe, WindowKey(index, "R_COMMAND_MOD"), {0});
  SetIntVec(gexe, WindowKey(index, "WAKU_SETNO"), {sub.waku_no});

  SetIntVec(gexe, WindowKey(index, "NAME_WAKU_SETNO"), {sub.name_waku_no});
  SetIntVec(gexe, WindowKey(index, "NAME_MOJI_REP"), {sub.name_moji_space_x});
  SetIntVec(gexe, WindowKey(index, "NAME_MOJI_POS"), BuildNameMojiPos(sub));
  SetIntVec(gexe, WindowKey(index, "NAME_POS"),
            {sub.name_window_x, sub.name_window_y});
  SetIntVec(gexe, WindowKey(index, "NAME_WAKU_DIR"), {0});
  SetIntVec(gexe, WindowKey(index, "NAME_CENTERING"), {0});
  SetIntVec(gexe, WindowKey(index, "NAME_MOJI_MIN"), {sub.name_moji_count});
  SetIntVec(gexe, WindowKey(index, "NAME_MOJI_SIZE"), {sub.name_moji_size});
}

detail::SiglusMwndConfig EnsureSiglusTextDefaults(Gameexe& gexe) {
  SetIntVecIfMissing(gexe, "WINDOW_ATTR", {255, 255, 255, 255, 0});
  SetIntVecIfMissing(gexe, "COLOR_TABLE.000", {255, 255, 255});
  SetIntVecIfMissing(gexe, "COLOR_TABLE.001", {0, 0, 0});
  SetIntVecIfMissing(gexe, "COLOR_TABLE.002", {255, 0, 0});
  SetIntVecIfMissing(gexe, "COLOR_TABLE.003", {0, 255, 0});
  SetIntVecIfMissing(gexe, "COLOR_TABLE.004", {0, 0, 255});
  SetIntVecIfMissing(gexe, "COLOR_TABLE.005", {255, 255, 0});
  SetIntVecIfMissing(gexe, "COLOR_TABLE.006", {255, 0, 255});
  SetIntVecIfMissing(gexe, "COLOR_TABLE.007", {0, 255, 255});
  SetIntVecIfMissing(gexe, "COLOR_TABLE.254", {255, 255, 255});
  return detail::NormalizeSiglusMwndConfig(gexe);
}

}  // namespace

namespace detail {

SiglusMwndConfig NormalizeSiglusMwndConfig(Gameexe& gexe) {
  SiglusMwndConfig config;
  config.default_mwnd_no =
      ReadInt(gexe, "MWND.DEFAULT_MWND_NO").value_or(config.default_mwnd_no);
  config.default_sel_mwnd_no = ReadInt(gexe, "MWND.DEFAULT_SEL_MWND_NO")
                                   .value_or(config.default_sel_mwnd_no);

  const int configured_count =
      ReadInt(gexe, "MWND.CNT").value_or(kSiglusDefaultMwndCount);
  const int min_required_count =
      std::max({0, config.default_mwnd_no + 1, config.default_sel_mwnd_no + 1});
  const int window_count = std::clamp(
      std::max(configured_count, min_required_count), 0, kSiglusMaxMwndCount);

  for (int i = 0; i < window_count; ++i)
    WriteRealliveWindow(gexe, i, LoadSiglusMwndSub(gexe, i));

  gexe.SetIntAt("DEFAULT_SEL_WINDOW", config.default_sel_mwnd_no);
  return config;
}

}  // namespace detail

// Load Gameexe.ini config
static Gameexe LoadGameexe(std::shared_ptr<AssetScanner> scanner) {
  auto pth = scanner->FindFile("Gameexe", {"dat", "ini"});
  if (!pth.has_value()) {
    logger(Severity::Error) << "Gameexe.dat not found: " << pth.error().what();
    return {};
  }

  if (pth->extension() == ".ini") {
    auto gexe = Gameexe::FromFile(pth.value());
    if (gexe.has_value())
      return gexe.value();
    logger(Severity::Error) << "Error while loading " << pth->string() << ": "
                            << gexe.error().message;
  } else {
    try {
      return CreateGexe(pth.value());
    } catch (std::exception& e) {
      logger(Severity::Error)
          << "Error while loading " << pth->string() << ": " << e.what();
    }
  }
  return {};
}

SiglusRuntime SGVMFactory::Create() {
  SiglusRuntime rt;
  rt.vm = std::make_unique<sr::VM>(m6::VMFactory::Create());
  sr::VM& vm = *rt.vm;
  std::shared_ptr<sr::GarbageCollector> gc = vm.gc_;

  fs::path seen_path = CorrectPathCase(base_path_ / "scene.pck");
  MappedFile archive_mf(seen_path);
  rt.archive = std::make_shared<Archive>(Archive::Create(archive_mf.Read()));
  rt.loader = std::make_unique<binding::Loader>(*rt.archive, vm, debug_);

  rt.base_pth = base_path_;
  rt.save_pth = rt.base_pth / "save";
  rt.asset_scanner = std::make_shared<AssetScanner>();
  rt.asset_scanner->IndexDirectory(rt.base_pth);

  rt.gameexe = std::make_shared<Gameexe>(LoadGameexe(rt.asset_scanner));
  Gameexe& gexe = *rt.gameexe;
  const detail::SiglusMwndConfig mwnd_config = EnsureSiglusTextDefaults(gexe);
  gexe.SetStringAt("CAPTION", "SiglusTest");
  gexe.SetStringAt("REGNAME", "sjis: SIGLUS\\TEST");
  gexe.SetIntAt("NAME_ENC", 0);
  gexe.SetIntAt("SUBTITLE", 0);
  gexe.SetIntAt("MOUSE_CURSOR", 0);
  gexe.SetStringAt("__GAMEPATH", base_path_.string());
  gexe.parseLine("#SCREENSIZE_MOD=999,1920,1080");

  // Init sdl system
  SystemOptions system_options;
  system_options.fast_forward = fast_forward_;
  rt.system = std::make_unique<System>(gexe, rt.asset_scanner, system_options);
  rt.system->text().set_active_window(mwnd_config.default_mwnd_no);
  rt.stage =
      std::make_unique<Stage>(rt.system->graphics().GetObjectLayerSize());
  rt.renderer = std::make_shared<SiglusSceneRenderer>(*rt.stage, *rt.system);
  rt.system->graphics().BindSceneRenderer(rt.renderer);

  rt.local_config = std::make_shared<Gameexe>();
  rt.global_config = std::make_shared<Gameexe>();

  struct SystemEventListener : public EventListener {
    sr::VM& vm;
    System& sys;
    SystemEventListener(sr::VM& v, System& s) : vm(v), sys(s) {}
    void OnEvent(std::shared_ptr<Event> event) override {
      if (std::visit(
              [&](auto& event) -> bool {
                using T = std::decay_t<decltype(event)>;
                if constexpr (std::same_as<T, Quit>) {
                  vm.RequestStop();
                  return true;
                }
                if constexpr (std::same_as<T, VideoExpose>) {
                  sys.graphics().ForceRefresh();
                  return true;
                }
                if constexpr (std::same_as<T, VideoResize>) {
                  sys.graphics().Resize(event.size);
                  return true;
                }
                if constexpr (std::same_as<T, MouseMotion>) {
                  const auto& graphics_sys = sys.graphics();
                  const auto aspect_ratio_w =
                      1.0f * graphics_sys.GetDisplaySize().width() /
                      graphics_sys.screen_size().width();
                  const auto aspect_ratio_h =
                      1.0f * graphics_sys.GetDisplaySize().height() /
                      graphics_sys.screen_size().height();
                  event.pos.set_x(event.pos.x() / aspect_ratio_w);
                  event.pos.set_y(event.pos.y() / aspect_ratio_h);
                  return false;
                }
                return false;
              },
              *event))
        *event = std::monostate();
    }
  };
  rt.system_event_listener =
      std::make_shared<SystemEventListener>(vm, *rt.system);
  rt.system->event().AddListener(20, rt.system_event_listener);

  for (auto it = binding::SiglusBindingRegistry::cbegin();
       it != binding::SiglusBindingRegistry::cend(); ++it) {
    it->second(rt);
  }
  sb::module_ m(gc.get(), vm.globals_.get());

  m.def("__builtin_streq", [](sr::Value lhs, sr::Value rhs) -> sr::Value {
    const auto* lstr = lhs.Get_if<sr::String>();
    const auto* rstr = rhs.Get_if<sr::String>();
    if (lstr && rstr) {
      [[likely]]
      if (lstr->str_.size() != rstr->str_.size())
        return false;
      for (std::size_t i = 0, n = lstr->str_.size(); i < n; ++i) {
        unsigned char c1 = lstr->str_[i], c2 = rstr->str_[i];
        if (std::tolower(c1) != std::tolower(c2))
          return false;
      }
      return true;
    }
    return lhs.Hash() == rhs.Hash();
  });
  m.def("__builtin_dbgvalue", [](sr::VM& vm, sr::Value value) -> sr::Value {
    std::string s;
    if (const auto* str = value.Get_if<sr::String>())
      s = '"' + EncodeText(str->str_) + '"';
    else
      s = value.Str();
    return vm.gc_->Allocate<sr::String>(std::move(s));
  });
  m.def(
      "__builtin_dbgprint",
      [](std::vector<sr::Value> args) {
        for (const auto& it : args)
          std::cerr << it.Str();
        std::cerr << std::endl;
      },
      sb::vararg);
  m.def("__builtin_load_scn",
        [loader = rt.loader.get()](int scnid) -> sr::Value {
          sr::Module* mod = loader->Load(scnid);
          return sr::Value(mod);
        });

  m.def("__builtin_farcall",
        [loader = rt.loader.get()](std::string scn, int zlabel) -> sr::Value {
          for (auto& c : scn)
            c = std::tolower(c);
          const std::string zname = GetZlabelId(zlabel);
          const std::string dbgname = std::format("SCENE{}@{}", scn, zlabel);
          sr::Module* mod = loader->Load(scn);

          if (!mod)
            throw sr::RuntimeError(
                std::format("Farcall {} could not load scene", dbgname));

          auto it = mod->globals->find(zname);
          if (it == mod->globals->cend())
            it = mod->globals->find("%%script");
          return it->second;
        });
  m.def("__builtin_usrcmd",
        [loader = rt.loader.get()](int scn, int entry,
                                   std::string name) -> sr::Value {
          const std::string cmdname = GetUsercmdId(entry);
          const std::string dbgname = std::format("{}:{}@{}", scn, entry, name);

          sr::Module* mod = loader->Load(scn);
          if (!mod) {
            throw sr::RuntimeError(std::format(
                "User command {} could not load scene {}", dbgname, scn));
          }

          auto it = mod->globals->find(cmdname);
          if (it == mod->globals->cend()) {
            std::string errmsg =
                std::format("User command {} does not exist in {}:{}", dbgname,
                            scn, mod->name);
            throw sr::RuntimeError(std::move(errmsg));
          }
          return it->second;
        });
  m.def("savepoint", [] {
    // TODO: implement save/load and serialization support
    return 0;
  });
  m.def("capture", [] {
    // TODO: create capture thumb image
  });

  // abuse the vm scheduler to refresh sdl regularly
  auto cb_holder = std::make_shared<std::function<void()>>();
  *cb_holder = [cb_holder, vm = rt.vm.get(), system = rt.system.get()]() {
    constexpr auto period =
        chr::duration_cast<chr::steady_clock::duration>(chr::seconds(1)) / 60;
    auto next = chr::steady_clock::now() + period;

    system->Run();
    if (system->IsQuitRequested()) {
      vm->RequestStop();
      return;
    }

    vm->scheduler_.PushCallbackAt(*cb_holder, next);
  };
  rt.exec_sdl_callback = [cb_holder]() { (*cb_holder)(); };
  rt.vm->scheduler_.PushCallbackAfter(rt.exec_sdl_callback,
                                      chr::milliseconds(2));

  return rt;
}

}  // namespace libsiglus
