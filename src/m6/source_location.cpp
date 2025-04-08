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

#include "m6/source_location.hpp"
#include "m6/token.hpp"

namespace m6 {

SourceLocation::SourceLocation(size_t begin, size_t end)
    : begin_offset(begin), end_offset(end) {}
SourceLocation::SourceLocation(Token* tok) { *this = tok->loc_; }

SourceLocation SourceLocation ::At(size_t pos) {
  return SourceLocation(pos, pos);
}
SourceLocation SourceLocation ::After(Token* tok) {
  return SourceLocation::At(tok->loc_.end_offset);
}
SourceLocation SourceLocation ::Range(Token* begin, Token* end) {
  if (begin >= end)
    return SourceLocation(begin);
  --end;
  return SourceLocation(begin->loc_.begin_offset, end->loc_.end_offset);
}

SourceLocation::operator std::string() const {
  return '(' + std::to_string(begin_offset) + ',' + std::to_string(end_offset) +
         ')';
}

}  // namespace m6
