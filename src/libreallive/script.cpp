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

#include "libreallive/script.hpp"

#include "core/compression.hpp"
#include "libreallive/alldefs.hpp"
#include "libreallive/bytecode_table.hpp"
#include "libreallive/elements/bytecode.hpp"
#include "libreallive/header.hpp"
#include "libreallive/parser.hpp"
#include "libreallive/xorkey.hpp"

#include <algorithm>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

namespace libreallive {

Script::Script(
    std::vector<std::pair<unsigned long, std::shared_ptr<BytecodeElement>>>
        elements,
    std::map<int, unsigned long> entrypoints)
    : elements_(std::move(elements)), entrypoints_(std::move(entrypoints)) {}

Script::~Script() {}

Script ParseScript(const Header& hdr,
                   const std::string_view& data_view,
                   const std::string& regname,
                   bool use_xor_2,
                   const XorKey* second_level_xor_key) {
  const char* const data = data_view.data();

  // Kidoku/entrypoint table
  const int kidoku_offs = read_i32(data + 0x08);
  const size_t kidoku_length = read_i32(data + 0x0c);
  std::shared_ptr<BytecodeTable> ctable = std::make_shared<BytecodeTable>();
  ctable->kidoku_table.resize(kidoku_length);
  for (size_t i = 0; i < kidoku_length; ++i)
    ctable->kidoku_table[i] = read_i32(data + kidoku_offs + i * 4);

  const XorKey* key = NULL;
  if (use_xor_2) {
    if (second_level_xor_key) {
      key = second_level_xor_key;
    } else {
      throw std::runtime_error(
          "Can not read game script for " + regname +
          ".\nSome games require individual reverse engineering. This game can "
          "not be played until someone has figured out how the game script "
          "is encoded.");
    }
  }

  auto compressed =
      std::string(data + read_i32(data + 0x20), read_i32(data + 0x28));
  int idx = 0;
  std::transform(compressed.begin(), compressed.end(), compressed.begin(),
                 [&](auto x) { return x ^ xor_mask[idx++ & 0xff]; });

  std::string decompressed = Decompress_lzss(compressed);

  if (key != nullptr) {
    for (auto mykey = key; mykey->xor_offset != -1; ++mykey) {
      idx = mykey->xor_offset;
      for (int i = 0; i < mykey->xor_length && idx < decompressed.size();
           ++i, ++idx)
        decompressed[idx] ^= mykey->xor_key[i & 0xf];
    }
  }

  // Read bytecode
  const char* stream = decompressed.data();
  const size_t dlen = decompressed.size();
  const char* end = stream + dlen;
  size_t pos = 0;

  std::vector<std::pair<unsigned long, std::shared_ptr<BytecodeElement>>>
      elements;
  std::map<int, unsigned long> entrypoints;

  Parser parser(ctable);
  while (pos < dlen) {
    // Parse bytecode element
    const auto elm = parser.ParseBytecode(stream, end);
    elements.emplace_back(pos, elm);

    // Keep track of the entrypoints
    int entrypoint = elm->GetEntrypoint();
    if (entrypoint != BytecodeElement::kInvalidEntrypoint) {
      entrypoints.emplace(entrypoint, pos);
    }

    // Advance
    size_t l = elm->GetBytecodeLength();
    if (l <= 0)
      l = 1;  // Failsafe: always advance at least one byte.
    stream += l;
    pos += l;
  }

  return Script(std::move(elements), std::move(entrypoints));
}

}  // namespace libreallive
