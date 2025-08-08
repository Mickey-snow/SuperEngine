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

#include "libsiglus/bindings/common.hpp"
#include "libsiglus/bindings/obj.hpp"
#include "libsiglus/bindings/sdl.hpp"
#include "libsiglus/bindings/system.hpp"

#include "m6/vm_factory.hpp"
#include "vm/gc.hpp"

#include <filesystem>

namespace libsiglus {
namespace sr = serilang;
namespace fs = std::filesystem;

sr::VM SGVMFactory::Create() {
  auto gc = std::make_shared<sr::GarbageCollector>();
  sr::VM vm = m6::VMFactory::Create();

  binding::Context ctx;
  ctx.base_pth = fs::temp_directory_path() / "game";
  ctx.save_pth = ctx.base_pth / "save";
  ctx.asset_scanner = std::make_shared<AssetScanner>();
  ctx.asset_scanner->IndexDirectory(ctx.base_pth);

  // add bindings here
  binding::SDL(ctx).Bind(vm);
  binding::System(ctx).Bind(vm);

  return vm;
}

}  // namespace libsiglus
