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
  virtual size_t ByteLength() const = 0;
};
using Element = std::shared_ptr<IElement>;

class Line : public IElement {
 public:
  Line(int linenum) : linenum_(linenum) {}

  std::string ToDebugString() const override {
    return "#line " + std::to_string(linenum_);
  }

  size_t ByteLength() const override { return 5; }

 private:
  int linenum_;
};

class Push : public IElement {
 public:
  Push(Type type, int value) : type_(type), value_(value) {}

  std::string ToDebugString() const override {
    return "push(" + ToString(type_) + ':' + std::to_string(value_) + ')';
  }

  size_t ByteLength() const override { return 9; }

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

  size_t ByteLength() const override { return 5; }

 private:
  Type type_;
};

class Marker : public IElement {
 public:
  Marker() = default;

  std::string ToDebugString() const override { return "push(<elm>)"; }

  size_t ByteLength() const override { return 1; }
};

class Command : public IElement {
 public:
  Command(int v1, int v2, int v3, int v4)
      : v1_(v1), v2_(v2), v3_(v3), v4_(v4) {}

  std::string ToDebugString() const override {
    return "cmd(" + std::to_string(v1_) + ',' + std::to_string(v2_) + ',' +
           std::to_string(v3_) + ',' + std::to_string(v4_) + ')';
  }

  size_t ByteLength() const override { return 17; }

 private:
  int v1_, v2_, v3_, v4_;
};

class Property : public IElement {
 public:
  Property() = default;

  std::string ToDebugString() const override { return "<prop>"; }

  size_t ByteLength() const override { return 1; }
};

class Operate2 : public IElement {
 public:
  Operate2(Type lt, Type rt, OperatorCode op)
      : ltype_(lt), rtype_(rt), op_(op) {}

  std::string ToDebugString() const override {
    return ToString(ltype_) + ' ' + ToString(op_) + ' ' + ToString(rtype_);
  }

  size_t ByteLength() const override { return 10; }

 private:
  Type ltype_, rtype_;
  OperatorCode op_;
};

class Goto : public IElement {
 public:
  enum class Condition { True, False, Uncondition };

  Goto(Condition cond, int label) : cond_(cond), label_(label) {}

  std::string ToDebugString() const override {
    std::string str = '(' + std::to_string(label_) + ')';
    switch (cond_) {
      case Condition::True:
        return "goto_true" + str;
      case Condition::False:
        return "goto_false" + str;
      case Condition::Uncondition:
        return "goto" + str;
      default:
        return "goto?" + str;
    }
  }

  size_t ByteLength() const override { return 10; }

 private:
  Condition cond_;
  int label_;
};

}  // namespace libsiglus

#endif
