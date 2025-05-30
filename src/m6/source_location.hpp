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

#include <cstddef>
#include <memory>
#include <string>

namespace m6 {

struct Token;
class SourceBuffer;

class SourceLocation {
 public:
  size_t begin_offset, end_offset;
  std::shared_ptr<SourceBuffer> src;

  explicit SourceLocation(size_t begin = 0,
                          size_t end = 0,
                          std::shared_ptr<SourceBuffer> src = nullptr);

  [[deprecated]]
  static SourceLocation After(Token* tok);
  SourceLocation After() const;
  [[deprecated]]
  static SourceLocation Range(Token* begin, Token* end);
  SourceLocation Combine(const SourceLocation& end) const;

  explicit operator std::string() const;
};

}  // namespace m6
