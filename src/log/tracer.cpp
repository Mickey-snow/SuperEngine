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

#include "log/tracer.hpp"

#include "libreallive/elements/command.hpp"
#include "machine/rloperation.hpp"
#include "memory/location.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>

void Tracer::Log(int scene,
                 int line,
                 RLOperation* op,
                 const libreallive::CommandElement& f) {
  std::ostringstream oss;
  oss << "(SEEN" << std::setw(4) << std::setfill('0') << scene << ")(Line "
      << std::setw(4) << std::setfill('0') << line << "): ";

  if (op == nullptr)
    oss << "???";
  else
    oss << op->name();

  auto PrintParamterString =
      [](std::ostream& oss,
         const std::vector<libreallive::Expression>& params) {
        bool first = true;
        oss << "(";
        for (auto const& param : params) {
          if (!first) {
            oss << ", ";
          }
          first = false;

          oss << param->GetDebugString();
        }
        oss << ")";
      };
  PrintParamterString(oss, f.GetParsedParameters());

  std::clog << oss.str() << std::endl;
}

void Tracer::Log(int scene, int line, IntMemoryLocation loc, int value) {
  std::ostringstream oss;
  oss << "(SEEN" << std::setw(4) << std::setfill('0') << scene << ")(Line "
      << std::setw(4) << std::setfill('0') << line << "): ";
  oss << loc << " = " << value;

  std::clog << oss.str() << std::endl;
}

void Tracer::Log(int scene,
                 int line,
                 StrMemoryLocation loc,
                 std::string value) {
  std::ostringstream oss;
  oss << "(SEEN" << std::setw(4) << std::setfill('0') << scene << ")(Line "
      << std::setw(4) << std::setfill('0') << line << "): ";
  oss << loc << " = " << std::move(value);

  std::clog << oss.str() << std::endl;
}
