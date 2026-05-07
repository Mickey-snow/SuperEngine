// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
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
//
// -----------------------------------------------------------------------

#pragma once

#include "core/gameexe.hpp"
#include "libsiglus/bindings/loader.hpp"
#include "vm/value.hpp"

#include <functional>
#include <memory>

class AssetScanner;
class System;

namespace serilang {
class VM;
};  // namespace serilang

namespace libsiglus {
class Archive;
namespace binding {
class SiglusMwnd;
}

struct SiglusRuntime {
  std::shared_ptr<Gameexe> gameexe;
  std::shared_ptr<Archive> archive;
  std::unique_ptr<serilang::VM> vm;
  std::unique_ptr<binding::Loader> loader;
  std::shared_ptr<AssetScanner> asset_scanner;

  std::shared_ptr<binding::SiglusMwnd> mwnd;
  std::unique_ptr<System> system;

  std::function<void()> exec_sdl_callback;

  SiglusRuntime() = default;
  ~SiglusRuntime();
  SiglusRuntime(const SiglusRuntime&) = delete;
  SiglusRuntime& operator=(const SiglusRuntime&) = delete;
  SiglusRuntime(SiglusRuntime&&) noexcept = default;
  SiglusRuntime& operator=(SiglusRuntime&&) noexcept = default;
};

}  // namespace libsiglus
