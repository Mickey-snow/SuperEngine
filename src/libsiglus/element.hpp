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

#ifndef SRC_LIBSIGLUS_ELEMENT_HPP_
#define SRC_LIBSIGLUS_ELEMENT_HPP_

#include "libsiglus/types.hpp"

#include <memory>
#include <string>

namespace libsiglus {

class IElement {
 public:
  virtual ~IElement() = default;

  virtual std::string ToDebugString() const = 0;
};
using Element = std::shared_ptr<IElement>;

class Line : public IElement {
 public:
  Line(int linenum) : linenum_(linenum) {}

  std::string ToDebugString() const override {
    return "#line " + std::to_string(linenum_);
  }

 private:
  int linenum_;
};

class Push : public IElement {
 public:
  Push(Type type, int value) : type_(type), value_(value) {}

  std::string ToDebugString() const override {
    return "push(" + ToString(type_) + ':' + std::to_string(value_) + ')';
  }

 private:
  Type type_;
  int value_;
};

class Pop : public IElement {
 public:
  Pop(Type type) : type_(type) {}

  std::string ToDebugString() const override {
    return "pop<" + ToString(type_) + ">()";
  }

 private:
  Type type_;
};

class Marker : public IElement {
 public:
  Marker() = default;

  std::string ToDebugString() const override { return "push(<elm>)"; }
};

class Command : public IElement {
 public:
  Command(int v1, int v2, int v3, int v4)
      : v1_(v1), v2_(v2), v3_(v3), v4_(v4) {}

  std::string ToDebugString() const override {
    return "cmd(" + std::to_string(v1_) + ',' + std::to_string(v2_) + ',' +
           std::to_string(v3_) + ',' + std::to_string(v4_) + ')';
  }

 private:
  int v1_, v2_, v3_, v4_;
};

}  // namespace libsiglus

#endif
