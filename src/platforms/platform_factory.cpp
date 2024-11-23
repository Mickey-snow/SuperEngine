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

#include "platforms/platform_factory.hpp"

#include <filesystem>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;

PlatformImpl_t PlatformFactory::Create(const std::string& platform_name) {
  if (platform_name != "default") {
    const Context& ctx = GetContext();
    auto it = ctx.map_.find(platform_name);
    if (it != ctx.map_.cend())
      return std::invoke(it->second);

    std::cerr << "[WARNING] Constructor for platform " << platform_name
              << " not found.";
  }

  struct FakePlatform : public IPlatformImplementor {
    fs::path SelectGameDirectory() override { return fs::path("gamedir"); }
    void ReportFatalError(const std::string& message_text,
                          const std::string& informative_text) override {
      std::cerr << "ReportFatalError:" << message_text << '\n'
                << informative_text << std::endl;
    }
    bool AskUserPrompt(const std::string& message_text,
                       const std::string& informative_text,
                       const std::string& true_button,
                       const std::string& false_button) override {
      return false;
    }
  };
  return std::make_shared<FakePlatform>();
}

void PlatformFactory::Reset() { GetContext().map_.clear(); }

using const_iterator_t = PlatformFactory::const_iterator_t;
const_iterator_t PlatformFactory::cbegin() {
  return GetContext().map_.cbegin();
}
const_iterator_t PlatformFactory::cend() { return GetContext().map_.cend(); }

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
