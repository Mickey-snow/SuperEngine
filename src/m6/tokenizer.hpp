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
// Along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.
//
// -----------------------------------------------------------------------

#pragma once

#include "m6/exception.hpp"
#include "m6/token.hpp"
#include "utilities/string_pool.hpp"

#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace m6 {
class SourceBuffer;

class Tokenizer {
 public:
  explicit Tokenizer(std::vector<Token>& storage,
                     util::StringPool& pool = util::GlobalStringPool());

  void Parse(std::shared_ptr<SourceBuffer> input);

  bool Ok() const;
  std::span<const Error> GetErrors() const;
  void ClearErrors();

 public:
  std::vector<Error> errors_;
  bool skip_ws_;
  bool add_eof_;

 private:
  std::vector<Token>& storage_;
  util::StringPool& pool_;
};

}  // namespace m6
