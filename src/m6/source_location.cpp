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

#include <format>

namespace m6 {

SourceLocation::SourceLocation(size_t begin,
                               size_t end,
                               std::shared_ptr<SourceBuffer> src_in)
    : begin_offset(begin), end_offset(end), src(src_in) {}

SourceLocation SourceLocation::After() const {
  return SourceLocation(end_offset, end_offset, src);
}

SourceLocation SourceLocation::Combine(const SourceLocation& end) const {
  return SourceLocation(begin_offset, end.end_offset, src);
}

std::string SourceLocation::GetDebugString() const {
  return std::format("({},{})", begin_offset, end_offset);
}

}  // namespace m6
