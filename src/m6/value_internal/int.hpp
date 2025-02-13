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

#include "m6/value.hpp"

namespace m6 {

class Int : public IValue {
 public:
  Int(int val);

  virtual std::string Str() const override;
  virtual std::string Desc() const override;

  virtual std::type_index Type() const noexcept override;

  virtual Value Duplicate() override;

  virtual std::any Get() const override;
  virtual void* Getptr() override;

  virtual Value Operator(Op op, Value rhs) override;
  virtual Value Operator(Op op) override;

 private:
  int val_;
};

}  // namespace m6
