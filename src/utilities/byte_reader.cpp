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

#include "utilities/byte_reader.hpp"

ByteReader::ByteReader(const char* begin, const char* end)
    : begin_(begin), current_(begin), end_(end) {
  if (end < begin) {
    throw std::invalid_argument(
        "End pointer must be greater than or equal to the begin pointer.");
  }
}

ByteReader::ByteReader(const char* begin, size_t N)
    : begin_(begin), current_(begin), end_(begin + N) {
  if (N > 0 && begin == nullptr) {
    throw std::invalid_argument(
        "Null pointer is not allowed for a non-zero length.");
  }
}

ByteReader::ByteReader(std::string_view sv)
    : begin_(sv.data()), current_(sv.data()), end_(sv.data() + sv.size()) {}

uint64_t ByteReader::ReadBytes(int count) {
  if (count < 0 || count > 8) {
    throw std::invalid_argument("Count must be between 0 and 8.");
  }

  if (current_ + count > end_) {
    throw std::out_of_range(
        "Attempt to read beyond the end of the byte stream.");
  }

  uint64_t result = 0;
  for (int i = 0; i < count; ++i) {
    result |= static_cast<uint64_t>(static_cast<unsigned char>(current_[i]))
              << (8 * i);
  }
  return result;
}

uint64_t ByteReader::PopBytes(int count) {
  uint64_t result = ReadBytes(count);
  Proceed(count);
  return result;
}

void ByteReader::Proceed(int count) {
  if (current_ + count > end_ || current_ + count < begin_) {
    throw std::out_of_range(
        "Attempt to move the pointer outside the byte stream bounds.");
  }
  current_ += count;
}

size_t ByteReader::Size() const noexcept { return end_ - begin_; }

size_t ByteReader::Position() const noexcept { return current_ - begin_; }

void ByteReader::Seek(int loc) {
  if (loc < 0 || loc > Size()) {
    throw std::out_of_range("Seek position is outside the byte stream bounds.");
  }
  current_ = begin_ + loc;
}
