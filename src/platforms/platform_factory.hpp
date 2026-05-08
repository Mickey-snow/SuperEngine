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

#pragma once

#include "platforms/implementor.hpp"
#include "utilities/static_registry.hpp"

#include <functional>
#include <string>

/**
 * @class PlatformFactory
 * @brief Factory class for creating platform-specific implementations.
 *
 * The `PlatformFactory` is responsible for registering and creating instances
 * of platform-specific implementations, based on the platform name provided at
 * runtime. Each platform implementation must be registered with the factory
 * using the `PlatformFactory::Registrar` class, which ties a platform name to
 * its corresponding constructor function.
 *
 * Example usage:
 * @code
 * PlatformImpl_t platform = PlatformFactory::Create("my_platform_name");
 * @endcode
 */
class PlatformFactory {
  struct RegistryTag;
  using Registry =
      StaticRegistry<RegistryTag, std::string, std::function<PlatformImpl_t()>>;

 public:
  PlatformFactory() = delete;

  static PlatformImpl_t Create(const std::string& platform_name);

  static void Reset();

  using const_iterator_t = typename Registry::const_iterator;
  static const_iterator_t cbegin();
  static const_iterator_t cend();

  using Registrar = typename Registry::Registrar;
};
