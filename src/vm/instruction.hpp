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

#include "machine/op.hpp"

#include <cstdint>
#include <variant>

namespace serilang {

// ––– 1. Stack manipulation ––––––––––––––––––––––––––––––––––––––––
struct Push {
  uint32_t const_index;
};  // () → (value)
struct Dup {};   // (x)   → (x,x)
struct Swap {};  // (a,b) → (b,a)
struct Pop {
  uint8_t count = 1;
};  // (… n values …) →

// ––– 2. Arithmetic / logic ––––––––––––––––––––––––––––––––––––––––
struct BinaryOp {
  Op op;
};  // (lhs,rhs) → (result)

struct UnaryOp {
  Op op;
};  // (x) → (result)

// ––– 3. Local / global / up‑value access ––––––––––––––––––––––––––
struct LoadLocal {
  uint8_t slot;
};  // () → (value)
struct StoreLocal {
  uint8_t slot;
};  // (value) →
struct LoadGlobal {
  uint32_t name_index;
};  // () → (value)
struct StoreGlobal {
  uint32_t name_index;
};  // (value) →
struct LoadUpvalue {
  uint8_t slot;
};  // () → (value)
struct StoreUpvalue {
  uint8_t slot;
};  // (value) →
struct CloseUpvalues {
  uint8_t from_slot;
};  // close ≥slot up‑vals

// ––– 4. Control flow ––––––––––––––––––––––––––––––––––––––––––––––
struct Jump {
  int32_t offset;
};  // →
struct JumpIfTrue {
  int32_t offset;
};  // (cond) →
struct JumpIfFalse {
  int32_t offset;
};  // (cond) →
struct Return {};  // (retval) → pops frame

// ––– 5. Function & call –––––––––––––––––––––––––––––––––––––––––––
struct MakeClosure {
  uint32_t entry, nparams, nlocals, nupvals;
};  // (…) → (fn)
struct Call {
  uint8_t arity;
};  // (fn,arg*) → (ret)
struct TailCall {
  uint8_t arity;
};  // (fn,arg*) → (ret)  (TCO)

// ––– 6. Object / container ops ––––––––––––––––––––––––––––––––––––
struct MakeClass {
  uint32_t name_index;
  uint16_t nmethods;
};  // (method_fn*) → (class)
struct GetField {
  uint32_t name_index;
};  // (inst) → (value)
struct SetField {
  uint32_t name_index;
};  // (inst,val) →
struct GetItem {};  // (container,idx) → (val)
struct SetItem {};  // (container,idx,val) →

// ––– 7. Coroutine / fiber support ––––––––––––––––––––––––––––––––
struct MakeFiber {
  uint32_t entry, nparams, nlocals, nupvals;
};  // (…) → (fiber)
struct Resume {
  uint8_t arity;
};  // (fiber,args…) → (result|exc)
struct Yield {};  // (value) → (yielded)
//  ‣ `Yield` suspends the *current* fiber and returns control to its resumer
//  ‣ `Resume` runs the fiber until next `Yield` or `Return`

// ––– 8. Exception handling -------------––––––––––––––––––---------
struct Throw {};  // (exc) → never
struct TryBegin {
  int32_t handler_rel_ofs;
};  // mark try scope
struct TryEnd {};  // pop handler

using Instruction = std::variant<Push,
                                 Dup,
                                 Swap,
                                 Pop,
                                 BinaryOp,
                                 UnaryOp,
                                 LoadLocal,
                                 StoreLocal,
                                 LoadGlobal,
                                 StoreGlobal,
                                 LoadUpvalue,
                                 StoreUpvalue,
                                 CloseUpvalues,
                                 Jump,
                                 JumpIfTrue,
                                 JumpIfFalse,
                                 Return,
                                 MakeClosure,
                                 Call,
                                 TailCall,
                                 MakeClass,
                                 GetField,
                                 SetField,
                                 GetItem,
                                 SetItem,
                                 MakeFiber,
                                 Resume,
                                 Yield,
                                 Throw,
                                 TryBegin,
                                 TryEnd>;

}  // namespace serilang
