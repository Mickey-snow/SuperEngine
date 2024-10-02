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

#ifndef SRC_PLATFORMS_PLATFORM_FACTORY_H_
#define SRC_PLATFORMS_PLATFORM_FACTORY_H_

#include "platforms/implementor.h"

#include <functional>
#include <map>
#include <stdexcept>
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
 * if (!platform) {
 *   platform = PlatformFactory::CreateDefault();
 * }
 * @endcode
 */
class PlatformFactory {
 public:
  PlatformFactory() = delete;

  static PlatformImpl_t Create(const std::string& platform_name);

  static PlatformImpl_t CreateDefault();

  static void Reset();

  using const_iterator_t =
      typename std::map<std::string,
                        std::function<PlatformImpl_t()>>::const_iterator;
  static const_iterator_t cbegin();
  static const_iterator_t cend();

  /**
   * @class PlatformFactory::Registrar
   * @brief Helper class for registering platform implementations.
   *
   * The `Registrar` class is used to register platform implementations with
   * the factory. It associates a platform name with a constructor function
   * that creates an instance of the platform implementation.
   *
   * To add a new platform implementation:
   *
   * 1. Implement a subclass of `IPlatformImplementor`.
   * 2. Register the subclass using `PlatformFactory::Registrar`, by passing the
   * platform name and a functor that returns a new instance.
   *
   * Example:
   * @code
   * class MyPlatform : public IPlatformImplementor {
   *   // Implementation of MyPlatform
   * };
   *
   * namespace {
   *   PlatformFactory::Registrar registrar("my_platform_name", []() {
   *     return std::make_shared<MyPlatform>();
   *   });
   * }
   * @endcode
   */
  class Registrar {
   public:
    /**
     * @brief Constructor for registering a platform implementation.
     *
     * This constructor registers a new platform implementation with the
     * `PlatformFactory`. It associates the platform name with the constructor
     * function that will be used to create instances of the platform.
     *
     * @param name The unique name of the platform implementation.
     * @param constructor A function that returns an instance of the platform
     * implementation.
     */
    Registrar(const std::string& name,
              std::function<PlatformImpl_t()> constructor);
    ~Registrar() = default;
  };

 private:
  struct Context {
    std::map<std::string, std::function<PlatformImpl_t()>> map_;
  };

  static Context& GetContext();

  friend class Registrar;
};

#endif
