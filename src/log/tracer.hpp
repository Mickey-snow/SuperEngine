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

#include <iostream>
#include <memory>
#include <string>

class RLOperation;
namespace libreallive {
class CommandElement;
class ExpressionElement;
}  // namespace libreallive
class IntMemoryLocation;
class StrMemoryLocation;

class Tracer {
 public:
  Tracer();
  ~Tracer();

  void Log(int scene,
           int line,
           RLOperation* op,
           const libreallive::CommandElement& f);
  void Log(int scene, int line, const libreallive::ExpressionElement& f);

 private:
  struct Tracer_ctx;
  std::unique_ptr<Tracer_ctx> ctx_;
};