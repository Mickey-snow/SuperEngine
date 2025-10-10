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

#include "libsiglus/bindings/mwnd.hpp"

#include "libsiglus/bindings/common.hpp"
#include "libsiglus/siglus_runtime.hpp"

#include "srbind/srbind.hpp"
#include "systems/base/text_page.hpp"
#include "systems/base/text_system.hpp"
#include "systems/sdl/sdl_system.hpp"
#include "systems/sdl/sdl_text_window.hpp"

#include <chrono>
#include <thread>

namespace libsiglus::binding {
namespace sb = srbind;
namespace sr = serilang;

class SiglusMwnd {
  std::shared_ptr<SDLTextWindow> text_win_;
  TextPage page;

 public:
  SiglusMwnd(std::shared_ptr<SDLTextWindow> wd, SDLSystem& sys)
      : text_win_(wd), page(sys, wd->window_number()) {}

  bool DisplayCharacter(std::string current, std::string rest) {
    return page.Character(current, rest);
  }
};

void MWND::Bind(SiglusRuntime& runtime) {
  sr::VM& vm = *runtime.vm;

  sb::module_ m(vm.gc_.get(), vm.globals_);
  sb::class_<SiglusMwnd> mwnd(m, "Mwnd");

  mwnd.def(sb::init([sys = runtime.system.get()](int id) -> SiglusMwnd* {
             std::shared_ptr<TextSystem> text = sys->text_system_;
             if (auto wd = std::dynamic_pointer_cast<SDLTextWindow>(
                     text->GetTextWindow(id))) {
               return new SiglusMwnd(wd, *sys);
             } else
               throw std::runtime_error("Failed to create text window " +
                                        std::to_string(id));
           }),
           sb::arg("id") = 0);
  mwnd.def("disp", &SiglusMwnd::DisplayCharacter, sb::arg("current"),
           sb::arg("rest") = "");

  m.def("msg_block", []() {
    bool msgblk_started = false;
    bool clear_when_ready = false;

    if (msgblk_started)
      return;

    if (clear_when_ready) {
      // TODO: クリア準備フラグが立っているならクリアする
      // 1. mwndのクリア
      // 2. シングルトンのフルメッセージのクリア
    }

    // TODO: いろいろクリア処理とセーブ
  });
}

}  // namespace libsiglus::binding
