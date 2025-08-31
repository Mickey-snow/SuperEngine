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

namespace libsiglus::binding {
namespace sb = srbind;

class SiglusMwnd {
 public:
  bool msgblk_started = false;
  bool clear_when_ready = false;
};

void MWND::Bind(SiglusRuntime& runtime) {
  runtime.mwnd = std::make_shared<SiglusMwnd>();
  SiglusMwnd* mwnd = runtime.mwnd.get();

  sb::module_ m(*runtime.vm, "mwnd");

  m.def("msg_block", [mwnd]() {
    if (mwnd->msgblk_started)
      return;

    if (mwnd->clear_when_ready) {
      // TODO: クリア準備フラグが立っているならクリアする
      // 1. mwndのクリア
      // 2. シングルトンのフルメッセージのクリア
    }

    // TODO: いろいろクリア処理とセーブ
  });

  m.def("r", [mwnd]() { std::ignore = mwnd; });
}

}  // namespace libsiglus::binding
