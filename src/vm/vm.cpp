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

#include "vm/vm.hpp"

#include "m6/compiler_pipeline.hpp"

#include "machine/op.hpp"
#include "utilities/string_utilities.hpp"
#include "vm/call_frame.hpp"
#include "vm/gc.hpp"
#include "vm/instruction.hpp"
#include "vm/object.hpp"
#include "vm/upvalue.hpp"
#include "vm/value.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>

namespace serilang {

// -----------------------------------------------------------------------
// class VM
VM::VM(std::shared_ptr<GarbageCollector> gc)
    : gc_(gc),
      globals_(gc_->Allocate<Dict>()),
      builtins_(gc->Allocate<Dict>()) {}

VM::VM(std::shared_ptr<GarbageCollector> gc, Dict* globals, Dict* builtins)
    : gc_(gc), globals_(globals), builtins_(builtins) {}

Value VM::Evaluate(Code* chunk) {
  Fiber* f = AddFiber(chunk);
  f->state = FiberState::Running;
  return Run();
}

Fiber* VM::AddFiber(Code* chunk) {
  Function* fn = gc_->Allocate<Function>(chunk);
  Fiber* fiber = gc_->Allocate<Fiber>();
  push(fiber->stack, Value(fn));
  fn->Call(*this, *fiber, 0, 0);
  fibres_.push_back(fiber);

  return fiber;
}

void VM::CollectGarbage() {
  gc_->UnmarkAll();
  GCVisitor collector(*gc_);
  collector.MarkSub(last_);
  collector.MarkSub(main_fiber_);
  collector.MarkSub(globals_);
  collector.MarkSub(builtins_);
  for (auto& it : fibres_)
    collector.MarkSub(it);
  for (auto& [name, mod] : module_cache_)
    collector.MarkSub(mod);
  gc_->Sweep();
}

Value VM::AddTrack(TempValue&& t) { return gc_->TrackValue(std::move(t)); }

Value VM::Run() {
  bool active = true;
  while (active) {
    active = false;

    std::erase_if(fibres_, [&](Fiber* f) {
      switch (f->state) {
        case FiberState::Dead:
          if (f->pending_result.has_value())
            last_ = f->pending_result.value();
          return true;
        case FiberState::New:
          f->state = FiberState::Running;
        case FiberState::Running:
          ExecuteFiber(f);
          active = true;
          return false;
        case FiberState::Suspended:
          return false;
      }
      return false;
    });

    // run garbage collector
    if (gc_threshold_ > 0 && gc_->AllocatedBytes() >= gc_threshold_) {
      CollectGarbage();
      gc_threshold_ *= 2;
    }
  }

  return last_;
}

//------------------------------------------------------------------------------
// Exec helpers
//------------------------------------------------------------------------------

void VM::push(std::vector<Value>& stack, Value v) {
  stack.emplace_back(std::move(v));
}

Value VM::pop(std::vector<Value>& stack) {
  auto v = std::move(stack.back());
  stack.pop_back();
  return v;
}

Dict* VM::GetNamespace(Fiber& f) {
  if (f.frames.empty())
    return nullptr;
  Function* fn = f.frames.back().fn;
  if (fn == nullptr)
    return nullptr;
  return fn->globals;
}

void VM::RuntimeError(const std::string& msg) { throw std::runtime_error(msg); }

void VM::Return(Fiber& f) {
  auto& frame = f.frames.back();
  Value ret = f.stack.empty() ? Value() : f.stack.back();

  // shrink stack to base pointer + 1 (for return value)
  f.stack.resize(frame.bp + 1);

  f.frames.pop_back();
  if (f.frames.empty()) {
    // fiber finished
    f.state = FiberState::Dead;
    if (f.waiter) {
      push(f.waiter->stack, std::move(ret));
      f.waiter->state = FiberState::Running;
    } else
      f.pending_result = std::move(ret);
  } else {
    // push return value back on stack
    f.stack.back() = std::move(ret);
  }
}

//------------------------------------------------------------------------------
// Core interpreter loop
//------------------------------------------------------------------------------

void VM::ExecuteFiber(Fiber* fib) {
  fib->state = FiberState::Running;
  while (!fib->frames.empty()) {
    auto& frame = fib->frames.back();
    auto* chunk = frame.fn->chunk;
    auto& ip = frame.ip;

    if (ip >= chunk->code.size()) {
      Return(*fib);
      return;
    }

    switch (static_cast<OpCode>((*chunk)[ip++])) {
      //------------------------------------------------------------------
      // 0. No-op
      //------------------------------------------------------------------
      case OpCode::Nop:
        break;

      //------------------------------------------------------------------
      // 1. Stack Manipulation
      //------------------------------------------------------------------
      case OpCode::Push: {
        const auto ins = chunk->Read<serilang::Push>(ip);
        ip += sizeof(ins);
        push(fib->stack, chunk->const_pool[ins.const_index]);
      } break;

      case OpCode::Dup: {
        const auto ins = chunk->Read<serilang::Dup>(ip);
        ip += sizeof(ins);
        push(fib->stack, fib->stack.end()[-ins.top_ofs - 1]);
      } break;

      case OpCode::Swap: {
        const auto ins = chunk->Read<serilang::Swap>(ip);
        ip += sizeof(ins);
        std::swap(fib->stack.end()[-1], fib->stack.end()[-2]);
      } break;

      case OpCode::Pop: {
        const auto ins = chunk->Read<serilang::Pop>(ip);
        ip += sizeof(ins);
        for (uint8_t i = 0; i < ins.count; ++i)
          fib->stack.pop_back();
      } break;

      //------------------------------------------------------------------
      // 2. Unary / Binary
      //------------------------------------------------------------------
      case OpCode::UnaryOp: {
        const auto ins = chunk->Read<serilang::UnaryOp>(ip);
        ip += sizeof(ins);
        auto v = pop(fib->stack);
        Value r = AddTrack(v.Operator(ins.op));
        push(fib->stack, r);
      } break;

      case OpCode::BinaryOp: {
        const auto ins = chunk->Read<serilang::BinaryOp>(ip);
        ip += sizeof(ins);
        auto rhs = pop(fib->stack);
        auto lhs = pop(fib->stack);
        Value out = AddTrack(lhs.Operator(ins.op, std::move(rhs)));
        push(fib->stack, out);
      } break;

      //------------------------------------------------------------------
      // 3. Locals / globals / upvalues
      //------------------------------------------------------------------
      case OpCode::LoadLocal: {
        const auto ins = chunk->Read<serilang::LoadLocal>(ip);
        ip += sizeof(ins);
        push(fib->stack, *fib->local_slot(fib->frames.size() - 1, ins.slot));
      } break;

      case OpCode::StoreLocal: {
        const auto ins = chunk->Read<serilang::StoreLocal>(ip);
        ip += sizeof(ins);
        *fib->local_slot(fib->frames.size() - 1, ins.slot) = pop(fib->stack);
      } break;

      case OpCode::LoadGlobal: {
        const auto ins = chunk->Read<serilang::LoadGlobal>(ip);
        ip += sizeof(ins);
        const auto name =
            chunk->const_pool[ins.name_index].template Get<std::string>();

        Dict* d = GetNamespace(*fib);
        if (d == nullptr || !d->map.contains(name))
          d = globals_;
        if (d == nullptr || !d->map.contains(name))
          d = builtins_;
        auto it = d->map.find(name);
        if (it == d->map.cend())
          RuntimeError("NameError: '" + name + "' is not defined");
        push(fib->stack, it->second);
      } break;

      case OpCode::StoreGlobal: {
        const auto ins = chunk->Read<serilang::StoreGlobal>(ip);
        ip += sizeof(ins);
        const auto name =
            chunk->const_pool[ins.name_index].template Get<std::string>();
        Value val = pop(fib->stack);

        Dict* dst = GetNamespace(*fib);
        if (dst == nullptr)
          dst = globals_;
        dst->map[name] = std::move(val);
      } break;

      case OpCode::LoadUpvalue: {
        const auto ins = chunk->Read<serilang::LoadUpvalue>(ip);
        ip += sizeof(ins);
        // (not implemented here)
      } break;

      case OpCode::StoreUpvalue: {
        const auto ins = chunk->Read<serilang::StoreUpvalue>(ip);
        ip += sizeof(ins);
        // (not implemented here)
      } break;

      case OpCode::CloseUpvalues: {
        const auto ins = chunk->Read<serilang::CloseUpvalues>(ip);
        ip += sizeof(ins);
        auto* base = fib->local_slot(fib->frames.size() - 1, ins.from_slot);
        fib->close_upvalues_from(base);
      } break;

      //------------------------------------------------------------------
      // 4. Control flow
      //------------------------------------------------------------------
      case OpCode::Jump: {
        const auto ins = chunk->Read<serilang::Jump>(ip);
        ip += sizeof(ins);
        frame.ip += ins.offset;
      } break;

      case OpCode::JumpIfTrue: {
        const auto ins = chunk->Read<serilang::JumpIfTrue>(ip);
        ip += sizeof(ins);
        auto cond = pop(fib->stack);
        if (cond.IsTruthy())
          frame.ip += ins.offset;
      } break;

      case OpCode::JumpIfFalse: {
        const auto ins = chunk->Read<serilang::JumpIfFalse>(ip);
        ip += sizeof(ins);
        auto cond = pop(fib->stack);
        if (!cond.IsTruthy())
          frame.ip += ins.offset;
      } break;

      case OpCode::Return: {
        const auto ins = chunk->Read<serilang::Return>(ip);
        ip += sizeof(ins);
        Return(*fib);
      }
        return;  // unwind to caller

      //------------------------------------------------------------------
      // 5. Function calls
      //------------------------------------------------------------------
      case OpCode::MakeFunction: {
        const auto ins = chunk->Read<serilang::MakeFunction>(ip);
        ip += sizeof(ins);
        std::unordered_map<std::string, size_t> param_index;
        param_index.reserve(ins.nparam);
        size_t name_idx = fib->stack.size() - ins.nparam;
        for (uint32_t i = 0; i < ins.nparam; ++i) {
          param_index.emplace(
              std::move(*fib->stack[name_idx + i].Get_if<std::string>()), i);
        }
        fib->stack.resize(name_idx);

        Code* chunk =
            fib->stack.end()[-1 - static_cast<size_t>(ins.ndefault) * 2]
                .Get<Code*>();
        std::unordered_map<size_t, Value> defaults;
        defaults.reserve(ins.ndefault);
        for (size_t i = fib->stack.size() - ins.ndefault * 2;
             i < fib->stack.size(); i += 2) {
          std::string* k = fib->stack[i].Get_if<std::string>();
          Value v = std::move(fib->stack[i + 1]);
          defaults.emplace(param_index.at(*k), std::move(v));
        }
        fib->stack.resize(fib->stack.size() - ins.ndefault * 2);

        Function* fn = gc_->Allocate<Function>(chunk);
        fn->globals = globals_;
        fn->entry = ins.entry;
        fn->defaults = std::move(defaults);
        fn->nparam = ins.nparam;
        fn->param_index = std::move(param_index);
        fn->has_vararg = ins.has_vararg;
        fn->has_kwarg = ins.has_kwarg;

        fib->stack.back() = Value(fn);
      } break;

      case OpCode::Call: {
        const auto ins = chunk->Read<serilang::Call>(ip);
        ip += sizeof(ins);
        Value& callee =
            fib->stack.end()[-static_cast<size_t>(ins.argcnt) -
                             2 * static_cast<size_t>(ins.kwargcnt) - 1];
        callee.Call(*this, *fib, ins.argcnt, ins.kwargcnt);
      } break;

      //------------------------------------------------------------------
      // 6. Object ops
      //------------------------------------------------------------------
      case OpCode::MakeList: {
        const auto ins = chunk->Read<serilang::MakeList>(ip);
        ip += sizeof(ins);
        std::vector<Value> elms;
        elms.reserve(ins.nelms);
        for (size_t i = fib->stack.size() - ins.nelms; i < fib->stack.size();
             ++i)
          elms.emplace_back(std::move(fib->stack[i]));
        fib->stack.resize(fib->stack.size() - ins.nelms);
        fib->stack.emplace_back(gc_->Allocate<List>(std::move(elms)));
      } break;

      case OpCode::MakeDict: {
        const auto ins = chunk->Read<serilang::MakeDict>(ip);
        ip += sizeof(ins);
        std::unordered_map<std::string, Value> elms;
        for (size_t i = fib->stack.size() - 2 * ins.nelms;
             i < fib->stack.size(); i += 2) {
          std::string key = fib->stack[i].Get<std::string>();
          Value val = std::move(fib->stack[i + 1]);
          elms.try_emplace(std::move(key), std::move(val));
        }
        fib->stack.resize(fib->stack.size() - 2 * ins.nelms);
        fib->stack.emplace_back(gc_->Allocate<Dict>(std::move(elms)));
      } break;

      case OpCode::MakeClass: {
        const auto ins = chunk->Read<serilang::MakeClass>(ip);
        ip += sizeof(ins);
        auto klass = gc_->Allocate<Class>();
        klass->name =
            chunk->const_pool[ins.name_index].template Get<std::string>();
        for (int i = 0; i < ins.nmethods; i++) {
          auto method = pop(fib->stack);
          auto name = pop(fib->stack).template Get<std::string>();
          klass->methods[std::move(name)] = std::move(method);
        }
        push(fib->stack, Value(klass));
      } break;

      case OpCode::GetField: {
        const auto ins = chunk->Read<serilang::GetField>(ip);
        ip += sizeof(ins);
        Value receiver = pop(fib->stack);
        auto name =
            chunk->const_pool[ins.name_index].template Get<std::string>();
        TempValue result = receiver.Member(name);
        push(fib->stack, gc_->TrackValue(std::move(result)));
      } break;

      case OpCode::SetField: {
        const auto ins = chunk->Read<serilang::SetField>(ip);
        ip += sizeof(ins);
        auto val = pop(fib->stack);
        auto receiver = pop(fib->stack);
        auto name =
            chunk->const_pool[ins.name_index].template Get<std::string>();
        receiver.SetMember(name, std::move(val));
      } break;

      case OpCode::GetItem: {
        const auto ins = chunk->Read<serilang::GetItem>(ip);
        ip += sizeof(ins);

        Value index = pop(fib->stack);
        Value receiver = pop(fib->stack);
        TempValue result = receiver.Item(index);
        push(fib->stack, gc_->TrackValue(std::move(result)));
      } break;
      case OpCode::SetItem: {
        const auto ins = chunk->Read<serilang::SetItem>(ip);
        ip += sizeof(ins);

        Value val = pop(fib->stack);
        Value index = pop(fib->stack);
        Value receiver = pop(fib->stack);
        receiver.SetItem(index, std::move(val));
      } break;

      //------------------------------------------------------------------
      // 7. Coroutines
      //------------------------------------------------------------------
      case OpCode::MakeFiber: {
        const auto ins = chunk->Read<serilang::MakeFiber>(ip);
        ip += sizeof(ins);
        size_t base = fib->stack.size() - ins.argcnt - 2 * ins.kwargcnt - 1;
        Value fn = fib->stack[base];

        Fiber* f = gc_->Allocate<Fiber>(/*reserve=*/16 + ins.argcnt +
                                        2 * ins.kwargcnt);
        std::move(fib->stack.begin() + base, fib->stack.end(),
                  std::back_inserter(f->stack));
        fib->stack.resize(base);

        fn.Call(*this, *f, ins.argcnt, ins.kwargcnt);

        push(fib->stack, Value(f));
        fibres_.push_back(f);
      } break;

      case OpCode::Await: {
        const auto ins = chunk->Read<serilang::Await>(ip);
        ip += sizeof(ins);
        // TODO: support passing argument
        Fiber* f = pop(fib->stack).template Get<Fiber*>();
        if (f->pending_result.has_value()) {
          push(fib->stack, f->pending_result.value());
          f->pending_result.reset();
          if (f->state != FiberState::Dead)
            f->state = FiberState::Running;
        } else {
          if (f->state == FiberState::Dead)
            RuntimeError("cannot await a dead fiber: " + f->Desc());
          f->state = FiberState::Running;
          f->waiter = fib;
          fib->state = FiberState::Suspended;
        }
      }
        return;  // switch -> await

      case OpCode::Yield: {
        const auto ins = chunk->Read<serilang::Yield>(ip);
        ip += sizeof(ins);
        fib->state = FiberState::Suspended;

        if (fib->waiter) {
          push(fib->waiter->stack, pop(fib->stack));
          fib->waiter->state = FiberState::Running;
          fib->waiter = nullptr;
        } else
          fib->pending_result = pop(fib->stack);
      }
        return;  // switch -> yield

      //------------------------------------------------------------------
      // 8. Exceptions
      //------------------------------------------------------------------
      case OpCode::Throw: {
        const auto ins = chunk->Read<serilang::Throw>(ip);
        ip += sizeof(ins);
        Value exc = pop(fib->stack);
        while (true) {
          if (fib->frames.empty()) {
            RuntimeError(exc.Str());
            return;
          }
          auto& fr = fib->frames.back();
          if (!fr.handlers.empty()) {
            auto h = fr.handlers.back();
            fr.handlers.pop_back();
            fib->stack.resize(h.stack_top);
            push(fib->stack, std::move(exc));
            fr.ip = h.handler_ip;
            break;
          } else {
            fib->stack.resize(fr.bp);
            fib->frames.pop_back();
          }
        }
      } break;

      case OpCode::TryBegin: {
        const auto ins = chunk->Read<serilang::TryBegin>(ip);
        ip += sizeof(ins);
        frame.handlers.push_back(
            {static_cast<uint32_t>(ip + ins.handler_rel_ofs),
             fib->stack.size()});
      } break;

      case OpCode::TryEnd: {
        const auto ins = chunk->Read<serilang::TryEnd>(ip);
        ip += sizeof(ins);
        if (!frame.handlers.empty())
          frame.handlers.pop_back();
      } break;

      //------------------------------------------------------------------
      // 9. Unimplemented
      //------------------------------------------------------------------
      default:
        RuntimeError("Unimplemented instruction at " + std::to_string(ip - 1));
        break;
    }

    if (fib->state != FiberState::Running)
      break;
  }

  if (fib->frames.empty())
    fib->state = FiberState::Dead;
}

}  // namespace serilang
