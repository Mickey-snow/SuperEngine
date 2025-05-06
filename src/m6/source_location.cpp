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

#include "log/domain_logger.hpp"
#include "m6/token.hpp"

namespace m6 {

SourceLocation::SourceLocation(size_t begin,
                               size_t end,
                               std::shared_ptr<SourceBuffer> src_in)
    : begin_offset(begin), end_offset(end), src(src_in) {}

SourceLocation SourceLocation ::After(Token* tok) {
  return SourceLocation(tok->loc_.end_offset, tok->loc_.end_offset,
                        tok->loc_.src);
}
SourceLocation SourceLocation ::Range(Token* begin, Token* end) {
  if (begin >= end)
    return begin->loc_;
  --end;
  return SourceLocation(begin->loc_.begin_offset, end->loc_.end_offset,
                        begin->loc_.src);
}

SourceLocation SourceLocation::After() const {
  return SourceLocation(end_offset, end_offset, src);
}

SourceLocation SourceLocation::Combine(const SourceLocation& end) const {
  if (src != end.src) {
    static DomainLogger logger("SourceLocation::Combine");
    logger(Severity::Warn) << "different reference differs";
  }

  return SourceLocation(begin_offset, end.end_offset, src);
}

SourceLocation::operator std::string() const {
  return '(' + std::to_string(begin_offset) + ',' + std::to_string(end_offset) +
         ')';
}

}  // namespace m6
