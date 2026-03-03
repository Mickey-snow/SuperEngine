// -----------------------------------------------------------------------
//
// This file is part of RLVM
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
// -----------------------------------------------------------------------

#include "libsiglus/xorkey.hpp"

#include <array>

namespace libsiglus {

const std::vector<XorKey const*> keyring{&empty_key, &stella_key, &sprb_key,
                                         &ab1b_key};

const XorKey empty_key = {.exekey = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                     0x00, 0x00}};

const XorKey sprb_key = {.exekey = {0x08, 0x63, 0x80, 0x0b, 0x79, 0xe9, 0x51,
                                    0xf1, 0xe7, 0x2a, 0xe7, 0x60, 0xaf, 0x38,
                                    0xdb, 0x4e}};

const XorKey stella_key = {.exekey = {0x4E, 0x2A, 0xB3, 0x14, 0x4C, 0x8C, 0x7D,
                                      0xED, 0x9C, 0x3F, 0x88, 0x0D, 0xC0, 0x0D,
                                      0x98, 0x2B}};

const XorKey ab1b_key = {.exekey = {0x5D, 0x2C, 0x99, 0x2C, 0x62, 0x94, 0x38,
                                    0xD5, 0x8C, 0x79, 0x9E, 0x3C, 0xA0, 0x1A,
                                    0xDB, 0x0B}};
}  // namespace libsiglus
