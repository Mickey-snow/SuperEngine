// -----------------------------------------------------------------------
//
// This file is part of libreallive, a dependency of RLVM.
//
// -----------------------------------------------------------------------
//
// Copyright (c) 2006, 2007 Peter Jolly
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// -----------------------------------------------------------------------

#include "libreallive/header.hpp"
#include "libreallive/alldefs.hpp"

#include <sstream>

namespace libreallive {

Metadata::Metadata() : encoding_(0) {}

void Metadata::Assign(const char* const input) {
  const int meta_len = read_i32(input), id_len = read_i32(input + 4) + 1;
  if (meta_len < id_len + 17)
    return;  // malformed metadata
  as_string_.assign(input, meta_len);
  encoding_ = input[id_len + 16];
}

void Metadata::Assign(const std::string_view& input) { Assign(input.data()); }

Header::Header(const char* const data, const size_t length) {
  if (length < 0x1d0)
    throw Error("not a RealLive bytecode file");

  std::string compiler = string(data, 4);

  // Check the version of the compiler.
  if (read_i32(data + 4) == 10002) {
    use_xor_2_ = false;
  } else if (read_i32(data + 4) == 110002) {
    use_xor_2_ = true;
  } else if (read_i32(data + 4) == 1110002) {
    use_xor_2_ = true;
  } else {
    // New xor key?
    std::ostringstream oss;
    oss << "Unsupported compiler version: " << read_i32(data + 4);
    throw Error(oss.str());
  }

  if (read_i32(data) != 0x1d0)
    throw Error("unsupported bytecode version");

  // Debug entrypoints
  z_minus_one_ = read_i32(data + 0x2c);
  z_minus_two_ = read_i32(data + 0x30);

  // Misc settings
  savepoint_message_ = read_i32(data + 0x1c4);
  savepoint_selcom_ = read_i32(data + 0x1c8);
  savepoint_seentop_ = read_i32(data + 0x1cc);

  // Dramatis personae
  int dplen = read_i32(data + 0x18);
  dramatis_personae_.reserve(dplen);
  int offs = read_i32(data + 0x14);
  while (dplen--) {
    int elen = read_i32(data + offs);
    dramatis_personae_.emplace_back(data + offs + 4, elen - 1);
    offs += elen + 4;
  }

  // If this scenario was compiled with RLdev, it may include a
  // potentially-useful metadata block.  Check for that and read it if
  // it's present.
  offs = read_i32(data + 0x14) + read_i32(data + 0x1c);
  if (offs != read_i32(data + 0x20)) {
    rldev_metadata_.Assign(data + offs);
  }
}

Header::~Header() {}

}  // namespace libreallive
