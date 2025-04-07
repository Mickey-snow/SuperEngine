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

#include "m6/line_table.hpp"

#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

namespace m6 {

class ErrorFormatter {
 public:
  // Constructor
  explicit ErrorFormatter(std::string_view src);

  // Appends a message with a newline.
  ErrorFormatter& Pushline(const std::string& msg);

  // Highlights the source between positions [begin, end) with a message.
  ErrorFormatter& Highlight(size_t begin, size_t end, const std::string& msg);

  // Returns the formatted error and resets the internal buffer.
  std::string Flush();

 private:
  std::string_view src_;
  LineTable index_;
  std::ostringstream oss;
};

}  // namespace m6
