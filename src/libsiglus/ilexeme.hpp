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

#ifndef SRC_LIBSIGLUS_ILEXEME_HPP_
#define SRC_LIBSIGLUS_ILEXEME_HPP_

#include "libsiglus/types.hpp"

#include <memory>
#include <string>

namespace libsiglus {

class ILexeme {
 public:
  virtual ~ILexeme() = default;

  virtual int GetType() const = 0;

  virtual std::string ToDebugString() const = 0;
  virtual size_t ByteLength() const = 0;
};
using Lexeme = std::shared_ptr<ILexeme>;

}  // namespace libsiglus

#endif
