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

#pragma once

#include "libsiglus/argument_list.hpp"
#include "libsiglus/lexfwd.hpp"
#include "libsiglus/types.hpp"

#include <string>
#include <vector>

namespace libsiglus {

enum class ByteCode : uint8_t {
  None = 0x00,

  Newline = 0x01,
  Push = 0x02,
  Pop = 0x03,
  Copy = 0x04,
  Property = 0x05,
  CopyElm = 0x06,
  Declare = 0x07,
  Marker = 0x08,
  Arg = 0x09,

  Goto = 0x10,
  Goto_true = 0x11,
  Goto_false = 0x12,
  Gosub_int = 0x13,
  Gosub_str = 0x14,
  Return = 0x15,
  End = 0x16,

  Assign = 0x20,
  Op1 = 0x21,
  Op2 = 0x22,

  Cmd = 0x30,
  Text = 0x31,
  Namae = 0x32,
  SelBegin = 0x33,
  SelEnd = 0x34
};

namespace lex {

struct None {
  std::string ToDebugString() const { return "none"; }
  size_t ByteLength() const { return 1; }
};

struct Line {
  std::string ToDebugString() const {
    return "#line " + std::to_string(linenum_);
  }
  size_t ByteLength() const { return 5; }

  int linenum_;
};

struct Push {
  std::string ToDebugString() const {
    return "push(" + ToString(type_) + ':' + std::to_string(value_) + ')';
  }
  size_t ByteLength() const { return 9; }

  Type type_;
  int value_;
};

struct Pop {
  std::string ToDebugString() const { return "pop<" + ToString(type_) + ">()"; }
  size_t ByteLength() const { return 5; }

  Type type_;
};

struct Marker {
  std::string ToDebugString() const { return "<elm>"; }
  size_t ByteLength() const { return 1; }
};

struct Command {
  Command(Signature s) : sig(s) {}

  std::string ToDebugString() const;
  size_t ByteLength() const;

  Signature sig;
};

struct Property {
  std::string ToDebugString() const { return "<prop>"; }
  size_t ByteLength() const { return 1; }
};

struct Operate1 {
  std::string ToDebugString() const {
    return ToString(op_) + ' ' + ToString(type_);
  }
  size_t ByteLength() const { return 6; }

  Type type_;
  OperatorCode op_;
};

struct Operate2 {
  std::string ToDebugString() const {
    return ToString(ltype_) + ' ' + ToString(op_) + ' ' + ToString(rtype_);
  }
  size_t ByteLength() const { return 10; }

  Type ltype_, rtype_;
  OperatorCode op_;
};

struct Goto {
  enum class Condition { True, False, Unconditional };

  std::string ToDebugString() const {
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
  size_t ByteLength() const { return 10; }

  Condition cond_;
  int label_;
};

struct Assign {
  std::string ToDebugString() const {
    return "let[" + std::to_string(v1_) + "] " + ToString(ltype_) +
           " := " + ToString(rtype_);
  }

  size_t ByteLength() const { return 13; }

  Type ltype_, rtype_;
  int v1_;  // maybe always =1
};

struct Copy {
  std::string ToDebugString() const {
    return "push(<" + ToString(type_) + ">)";
  }
  size_t ByteLength() const { return 5; }

  Type type_;
};

struct CopyElm {
  std::string ToDebugString() const { return "push(<elm>)"; }
  size_t ByteLength() const { return 1; }
};

struct Gosub {
  std::string ToDebugString() const;
  size_t ByteLength() const;

  Type return_type_;
  int label_;
  ArgumentList argt_;
};

struct Namae {
  std::string ToDebugString() const { return "namae(<str>)"; }
  size_t ByteLength() const { return 1; }
};

struct EndOfScene {
  std::string ToDebugString() const { return "#EOF"; }
  size_t ByteLength() const { return 1; }
};

struct Textout {
  std::string ToDebugString() const {
    return "text@" + std::to_string(kidoku_) + "(<str>)";
  }
  size_t ByteLength() const { return 5; }

  int kidoku_;
};

struct Return {
  std::string ToDebugString() const;
  size_t ByteLength() const;

  ArgumentList ret_types_;
};

struct Arg {
  std::string ToDebugString() const { return "arg"; }
  size_t ByteLength() const { return 1; }
};

struct Declare {
  Type type;
  size_t size;
  std::string ToDebugString() const;
  size_t ByteLength() const { return 1 + 4 + 4; }
};

}  // namespace lex

struct ByteLengthOf {
  template <typename T>
  auto operator()(const T& it) {
    return it.ByteLength();
  }
};

// helper
inline std::string ToDebugString(const Lexeme& l) {
  return std::visit([](const auto& l) { return l.ToDebugString(); }, l);
}

}  // namespace libsiglus
