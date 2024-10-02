// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2024 Serina Sakurai
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

#include "platforms/platform_factory.h"

#include <stdexcept>

PlatformImpl_t PlatformFactory::Create(const std::string& platform_name) {
  const Context& ctx = GetContext();
  auto it = ctx.map_.find(platform_name);
  if (it == ctx.map_.cend())
    throw std::runtime_error("Constructor for platform " + platform_name +
                             " not found.");
  return std::invoke(it->second);
}

PlatformImpl_t PlatformFactory::CreateDefault() {
  const Context& ctx = GetContext();
  if (ctx.map_.empty())
    return nullptr;
  return std::invoke(ctx.map_.begin()->second);
}

void PlatformFactory::Reset() { GetContext().map_.clear(); }

PlatformFactory::Registrar::Registrar(
    const std::string& name,
    std::function<PlatformImpl_t()> constructor) {
  auto result = GetContext().map_.try_emplace(name, constructor);
  if (!result.second)
    throw std::invalid_argument("Platform " + name + " registered twice.");
}

PlatformFactory::Context& PlatformFactory::GetContext() {
  static Context ctx;
  return ctx;
}
