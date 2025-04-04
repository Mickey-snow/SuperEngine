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

SourceLocation::SourceLocation(const Token& tok)
    : begin_offset(tok.offset), end_offset(begin_offset + 1) {}
SourceLocation::SourceLocation(const Token& begin, const Token& end)
    : begin_offset(begin.offset), end_offset(end.offset) {}

SourceLocation::SourceLocation(Token* tok)
    : begin_offset(tok->offset), end_offset(begin_offset + 1) {}
SourceLocation::SourceLocation(Token* begin, Token* end) {
  while (begin < end && end->HoldsAlternative<tok::WS>())
    --end;
  while (begin < end && begin->HoldsAlternative<tok::WS>())
    ++begin;
  begin_offset = begin->offset;
  end_offset = end->offset;
}

}  // namespace m6
