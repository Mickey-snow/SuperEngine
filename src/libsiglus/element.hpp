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
#include <vector>

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
  Command(int arglist,
          std::vector<Type> stackarg,
          std::vector<int> extraarg,
          Type returntype)
      : alist_(arglist),
        stack_arg_(std::move(stackarg)),
        extra_arg_(std::move(extraarg)),
        rettype_(returntype) {}

  std::string ToDebugString() const override {
    std::string result = "cmd[" + std::to_string(alist_) + "](";
    bool first = true;
    for (const auto it : stack_arg_) {
      if (!first)
        result += ',';
      else
        first = false;
      result += ToString(it);
    }
    for (const auto it : extra_arg_) {
      if (!first)
        result += ',';
      else
        first = false;
      result += std::to_string(it);
    }

    result += ") -> " + ToString(rettype_);

    return result;
  }

  size_t ByteLength() const override { return 17; }

 private:
  int alist_;
  std::vector<Type> stack_arg_;
  std::vector<int> extra_arg_;
  Type rettype_;
};

class Property : public IElement {
 public:
  Property() = default;

  std::string ToDebugString() const override { return "<prop>"; }

  size_t ByteLength() const override { return 1; }
};

class Operate1 : public IElement {
 public:
  Operate1(Type t, OperatorCode op) : type_(t), op_(op) {}

  std::string ToDebugString() const override {
    return ToString(op_) + ' ' + ToString(type_);
  }

  size_t ByteLength() const override { return 6; }

 private:
  Type type_;
  OperatorCode op_;
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
  enum class Condition { True, False, Unconditional };

  Goto(Condition cond, int label) : cond_(cond), label_(label) {}

  std::string ToDebugString() const override {
    std::string str = '(' + std::to_string(label_) + ')';
    switch (cond_) {
      case Condition::True:
        return "goto_true" + str;
      case Condition::False:
        return "goto_false" + str;
      case Condition::Unconditional:
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

class Assign : public IElement {
 public:
  Assign(Type ltype, Type rtype, int v1)
      : ltype_(ltype), rtype_(rtype), v1_(v1) {}

  std::string ToDebugString() const override {
    return "let[" + std::to_string(v1_) + "] " + ToString(ltype_) +
           " := " + ToString(rtype_);
  }

  size_t ByteLength() const override { return 13; }

 private:
  Type ltype_, rtype_;
  int v1_;
};

class Copy : public IElement {
 public:
  Copy(Type type) : type_(type) {}

  std::string ToDebugString() const override {
    return "push(<" + ToString(type_) + ">)";
  }

  size_t ByteLength() const override { return 5; }

 private:
  Type type_;
};

class CopyElm : public IElement {
 public:
  CopyElm() = default;

  std::string ToDebugString() const override { return "push(<elm>)"; }

  size_t ByteLength() const override { return 1; }
};

}  // namespace libsiglus

#endif
