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
const XorKey sprb_key = {
    .easykey =
        std::array<uint8_t, 256>{
            0x70, 0xf8, 0xa6, 0xb0, 0xa1, 0xa5, 0x28, 0x4f, 0xb5, 0x2f, 0x48,
            0xfa, 0xe1, 0xe9, 0x4b, 0xde, 0xb7, 0x4f, 0x62, 0x95, 0x8b, 0xe0,
            0x3,  0x80, 0xe7, 0xcf, 0xf,  0x6b, 0x92, 0x1,  0xeb, 0xf8, 0xa2,
            0x88, 0xce, 0x63, 0x4,  0x38, 0xd2, 0x6d, 0x8c, 0xd2, 0x88, 0x76,
            0xa7, 0x92, 0x71, 0x8f, 0x4e, 0xb6, 0x8d, 0x1,  0x79, 0x88, 0x83,
            0xa,  0xf9, 0xe9, 0x2c, 0xdb, 0x67, 0xdb, 0x91, 0x14, 0xd5, 0x9a,
            0x4e, 0x79, 0x17, 0x23, 0x8,  0x96, 0xe,  0x1d, 0x15, 0xf9, 0xa5,
            0xa0, 0x6f, 0x58, 0x17, 0xc8, 0xa9, 0x46, 0xda, 0x22, 0xff, 0xfd,
            0x87, 0x12, 0x42, 0xfb, 0xa9, 0xb8, 0x67, 0x6c, 0x91, 0x67, 0x64,
            0xf9, 0xd1, 0x1e, 0xe4, 0x50, 0x64, 0x6f, 0xf2, 0xb,  0xde, 0x40,
            0xe7, 0x47, 0xf1, 0x3,  0xcc, 0x2a, 0xad, 0x7f, 0x34, 0x21, 0xa0,
            0x64, 0x26, 0x98, 0x6c, 0xed, 0x69, 0xf4, 0xb5, 0x23, 0x8,  0x6e,
            0x7d, 0x92, 0xf6, 0xeb, 0x93, 0xf0, 0x7a, 0x89, 0x5e, 0xf9, 0xf8,
            0x7a, 0xaf, 0xe8, 0xa9, 0x48, 0xc2, 0xac, 0x11, 0x6b, 0x2b, 0x33,
            0xa7, 0x40, 0xd,  0xdc, 0x7d, 0xa7, 0x5b, 0xcf, 0xc8, 0x31, 0xd1,
            0x77, 0x52, 0x8d, 0x82, 0xac, 0x41, 0xb8, 0x73, 0xa5, 0x4f, 0x26,
            0x7c, 0xf,  0x39, 0xda, 0x5b, 0x37, 0x4a, 0xde, 0xa4, 0x49, 0xb,
            0x7c, 0x17, 0xa3, 0x43, 0xae, 0x77, 0x6,  0x64, 0x73, 0xc0, 0x43,
            0xa3, 0x18, 0x5a, 0xf,  0x9f, 0x2,  0x4c, 0x7e, 0x8b, 0x1,  0x9f,
            0x2d, 0xae, 0x72, 0x54, 0x13, 0xff, 0x96, 0xae, 0xb,  0x34, 0x58,
            0xcf, 0xe3, 0x0,  0x78, 0xbe, 0xe3, 0xf5, 0x61, 0xe4, 0x87, 0x7c,
            0xfc, 0x80, 0xaf, 0xc4, 0x8d, 0x46, 0x3a, 0x5d, 0xd0, 0x36, 0xbc,
            0xe5, 0x60, 0x77, 0x68, 0x8,  0x4f, 0xbb, 0xab, 0xe2, 0x78, 0x7,
            0xe8, 0x73, 0xbf},
    .exekey = std::array<uint8_t, 16>{0x08, 0x63, 0x80, 0x0b, 0x79, 0xe9, 0x51,
                                      0xf1, 0xe7, 0x2a, 0xe7, 0x60, 0xaf, 0x38,
                                      0xdb, 0x4e}};
}
