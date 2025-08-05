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

#include "m6/source_buffer.hpp"
#include "m6/token.hpp"

namespace m6 {

static constexpr size_t npos = std::string::npos;

std::shared_ptr<SourceBuffer> SourceBuffer::Create(std::string src,
                                                   std::string file) {
  SourceBuffer* sb = new SourceBuffer(std::move(src), std::move(file));
  return std::shared_ptr<SourceBuffer>(sb);
}

SourceBuffer::SourceBuffer(std::string src, std::string file)
    : file_(std::move(file)), src_(std::move(src)), line_table_(src_) {}

std::tuple<size_t, size_t> SourceBuffer::GetLineColumn(size_t offset) const {
  return line_table_.Find(offset);
}
std::string_view SourceBuffer::GetLine(size_t idx) const {
  return line_table_.LineText(idx);
}

std::string SourceBuffer::GetStr(size_t begin, size_t end) const {
  return static_cast<std::string>(GetView(begin, end));
}
std::string SourceBuffer::GetFile() const { return file_; }
std::string_view SourceBuffer::GetView(size_t begin, size_t end) const {
  std::string_view src_view = src_;
  return src_view.substr(begin, end >= Size() ? npos : (end - begin + 1));
}

SourceLocation SourceBuffer::GetReference(size_t begin, size_t end) {
  return SourceLocation(begin, end, shared_from_this());
}
SourceLocation SourceBuffer::GetReferenceAt(size_t pos) {
  return SourceLocation(pos, pos, shared_from_this());
}

}  // namespace m6
