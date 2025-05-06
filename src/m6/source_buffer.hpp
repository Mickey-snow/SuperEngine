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
#include "m6/source_location.hpp"

#include <memory>
#include <string>
#include <utility>

namespace m6 {

struct Token;

class SourceBuffer : public std::enable_shared_from_this<SourceBuffer> {
 public:
  static std::shared_ptr<SourceBuffer> Create(std::string src,
                                              std::string file);

  std::tuple<size_t, size_t> GetLineColumn(size_t offset) const;
  std::string_view GetLine(size_t idx) const;

  std::string GetStr() const;
  std::string GetFile() const;
  std::string_view GetView() const;

  SourceLocation GetReference(size_t begin, size_t end);
  SourceLocation GetReferenceAt(size_t pos);

 private:
  explicit SourceBuffer(std::string src,
                        std::string file);  // please use `Create` instead

  std::string file_;
  std::string src_;
  LineTable line_table_;
};

}  // namespace m6
