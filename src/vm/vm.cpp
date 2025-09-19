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
#include "vm/promise.hpp"
#include "vm/upvalue.hpp"
#include "vm/value.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iterator>
#include <memory>
#include <stdexcept>

namespace serilang {

using helper::pop;
using helper::push;

// -----------------------------------------------------------------------
// class VM
VM::VM(std::shared_ptr<GarbageCollector> gc)
    : gc_(gc),
      globals_(gc_->Allocate<Dict>()),
      builtins_(gc->Allocate<Dict>()),
      poller_(std::make_unique<IPoller>()) {
  // Promise class is lazily initialized in CreatePromise().
}

VM::VM(std::shared_ptr<GarbageCollector> gc, Dict* globals, Dict* builtins)
    : gc_(gc),
      globals_(globals),
      builtins_(builtins),
      poller_(std::make_unique<IPoller>()) {}

Value VM::Evaluate(Code* chunk) {
  Fiber* f = AddFiber(chunk);
  f->state = FiberState::Running;
  return Run();
}

Fiber* VM::AddFiber(Code* chunk) {
  Function* fn = gc_->Allocate<Function>(chunk);
  Fiber* fiber = gc_->Allocate<Fiber>();
  // Create and attach completion promise
  fiber->ResetPromise();
  push(fiber->stack, Value(fn));
  fn->Call(*this, *fiber, 0, 0);
  fibres_.push_back(fiber);
  Enqueue(fiber);
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
  // Seed: enqueue any New fibers so they can be processed.
  for (auto* f : fibres_) {
    if (f->state == FiberState::New)
      Enqueue(f);
  }

  while (true) {
    // Drain timers whose deadline has passed
    auto now = std::chrono::steady_clock::now();
    while (!timers_.empty() && timers_.top().when <= now) {
      auto t = timers_.top();
      timers_.pop();
      if (t.fib && t.fib->state != FiberState::Dead)
        Enqueue(t.fib);
    }

    Fiber* next = nullptr;
    if (!microq_.empty()) {
      next = microq_.front();
      microq_.pop_front();
    } else if (!runq_.empty()) {
      next = runq_.front();
      runq_.pop_front();
    }

    if (next) {
      if (next->state == FiberState::Running ||
          next->state == FiberState::New) {
        next->state = FiberState::Running;
        try {
          ExecuteFiber(next);
        } catch (const RuntimeError& e) {
          // Uncaught exception terminates fiber; reject its promise.
          if (next->completion_promise) {
            next->completion_promise->Reject(e.message());
            next->ResetPromise();
          }
          next->state = FiberState::Dead;
        }

        // If still running and not finished, requeue for fairness.
        if (next->state == FiberState::Running && !next->frames.empty())
          Enqueue(next);
      }
    } else {
      // No immediate work; if no timers pending, we are done (remaining
      // fibers are suspended and cannot make progress here).
      if (timers_.empty())
        break;

      // Sleep until next timer (if any) or yield control via poller.
      auto delta = timers_.top().when - now;
      std::chrono::milliseconds timeout =
          std::chrono::duration_cast<std::chrono::milliseconds>(delta);
      poller_->Wait(timeout);
    }

    // run garbage collector periodically
    if (gc_threshold_ > 0 && gc_->AllocatedBytes() >= gc_threshold_) {
      CollectGarbage();
      gc_threshold_ *= 2;
    }

    // Remove dead fibers and record last result
    std::erase_if(fibres_, [&](Fiber* f) {
      if (f->state == FiberState::Dead) {
        if (f->pending_result.has_value())
          last_ = std::move(*f->pending_result);
        return true;
      }
      return false;
    });
  }

  // Final sweep of dead fibers to update last_
  std::erase_if(fibres_, [&](Fiber* f) {
    if (f->state == FiberState::Dead) {
      if (f->pending_result.has_value())
        last_ = std::move(*f->pending_result);
      return true;
    }
    return false;
  });

  return last_;
}

//------------------------------------------------------------------------------
// Exec helpers
//------------------------------------------------------------------------------
Dict* VM::GetNamespace(Fiber& f) {
  if (f.frames.empty())
    return nullptr;
  Function* fn = f.frames.back().fn;
  if (fn == nullptr)
    return nullptr;
  return fn->globals;
}

void VM::Error(Fiber& f) {
  Value exc = pop(f.stack);
  while (true) {
    if (f.frames.empty())
      throw RuntimeError(exc.Str());

    CallFrame& fr = f.frames.back();
    if (!fr.handlers.empty()) {
      ExceptionHandler h = std::move(fr.handlers.back());
      fr.handlers.pop_back();
      f.stack.resize(h.stack_top);
      push(f.stack, std::move(exc));
      fr.ip = h.handler_ip;
      break;
    } else {
      f.stack.resize(fr.bp);
      f.frames.pop_back();
    }
  }
}

void VM::Error(Fiber& f, std::string exc) {
  push(f.stack, Value(std::move(exc)));
  Error(f);
}

void VM::Return(Fiber& f) {
  auto& frame = f.frames.back();
  Value ret = f.stack.empty() ? Value() : std::move(f.stack.back());

  // shrink stack to base pointer + 1 (for return value)
  f.stack.resize(frame.bp + 1);
  f.frames.pop_back();

  if (f.frames.empty()) {
    // fiber finished
    f.state = FiberState::Dead;

    // Resolve completion promise
    f.completion_promise->Resolve(ret);
    f.ResetPromise();
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
        TempValue result = nil;
        try {
          result = v.Operator(*this, *fib, ins.op);
        } catch (RuntimeError& e) {
          push(fib->stack, nil);
          Error(*fib, e.message());
          return;
        }

        push(fib->stack, AddTrack(std::move(result)));
      } break;

      case OpCode::BinaryOp: {
        const auto ins = chunk->Read<serilang::BinaryOp>(ip);
        ip += sizeof(ins);
        auto rhs = pop(fib->stack);
        auto lhs = pop(fib->stack);
        TempValue result = nil;
        try {
          result = lhs.Operator(*this, *fib, ins.op, std::move(rhs));
        } catch (RuntimeError& e) {
          push(fib->stack, nil);
          Error(*fib, e.message());
          return;
        }

        push(fib->stack, AddTrack(std::move(result)));
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
        if (it == d->map.cend()) {
          Error(*fib, "NameError: '" + name + "' is not defined");
          return;
        }
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
        try {
          callee.Call(*this, *fib, ins.argcnt, ins.kwargcnt);
        } catch (RuntimeError& e) {
          Error(*fib, e.message());
          return;
        }
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
        for (int i = 0; i < ins.nstaticfn; i++) {
          Function* fn = pop(fib->stack).template Get<Function*>();
          std::string name = pop(fib->stack).template Get<std::string>();
          klass->fields.try_emplace(std::move(name), fn);
        }
        for (int i = 0; i < ins.nmemfn; i++) {
          Function* fn = pop(fib->stack).template Get<Function*>();
          std::string name = pop(fib->stack).template Get<std::string>();
          klass->memfns.try_emplace(std::move(name), fn);
        }

        push(fib->stack, Value(klass));
      } break;

      case OpCode::GetField: {
        const auto ins = chunk->Read<serilang::GetField>(ip);
        ip += sizeof(ins);
        Value receiver = pop(fib->stack);
        auto name =
            chunk->const_pool[ins.name_index].template Get<std::string>();
        TempValue result = nil;
        try {
          result = receiver.Member(name);
        } catch (RuntimeError& e) {
          push(fib->stack, nil);
          Error(*fib, e.message());
          return;
        }

        push(fib->stack, AddTrack(std::move(result)));
      } break;

      case OpCode::SetField: {
        const auto ins = chunk->Read<serilang::SetField>(ip);
        ip += sizeof(ins);
        Value val = pop(fib->stack);
        Value receiver = pop(fib->stack);
        std::string* name =
            chunk->const_pool[ins.name_index].template Get_if<std::string>();
        try {
          receiver.SetMember(*name, std::move(val));
        } catch (RuntimeError& e) {
          Error(*fib, e.message());
          return;
        }
      } break;

      case OpCode::GetItem: {
        const auto ins = chunk->Read<serilang::GetItem>(ip);
        ip += sizeof(ins);

        Value& receiver = fib->stack.end()[-2];
        receiver.GetItem(*this, *fib);
      } break;
      case OpCode::SetItem: {
        const auto ins = chunk->Read<serilang::SetItem>(ip);
        ip += sizeof(ins);

        Value& receiver = fib->stack.end()[-3];
        receiver.SetItem(*this, *fib);
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
        f->ResetPromise();
        std::move(fib->stack.begin() + base, fib->stack.end(),
                  std::back_inserter(f->stack));
        fib->stack.resize(base);

        fn.Call(*this, *f, ins.argcnt, ins.kwargcnt);

        push(fib->stack, Value(f));
        fibres_.push_back(f);
        Enqueue(f);
      } break;

      case OpCode::Await: {
        const auto ins = chunk->Read<serilang::Await>(ip);
        ip += sizeof(ins);
        Value awaited = pop(fib->stack);
        std::shared_ptr<Promise> promise = nullptr;

        // If awaiting a Fiber, redirect to its completion promise so that
        // `await fiber` awaits final completion (not intermediate yields).
        if (auto tf = awaited.Get_if<Fiber>()) {
          if (tf->state == FiberState::Dead) {
            push(fib->stack, tf->pending_result.value_or(nil));
            return;
          }
          promise = tf->completion_promise;
        }

        if (promise == nullptr) {
          Error(*fib, "object is not awaitable: " + awaited.Desc());
          return;
        }

        fib->state = FiberState::Suspended;
        auto waker = [this, fib](Promise* promise) {
          if (promise->result->has_value() &&
              promise->status != Promise::Status::Pending) {
            push(fib->stack, promise->result->value());
          } else {
            this->Error(*fib, promise->result->error());
            // should we return here?
          }
          this->EnqueueMicro(fib);
        };

        if (promise->result.has_value()) {  // ready
          waker(promise.get());
        } else {  // pending
          promise->wakers.emplace_back(waker);
          EnqueueMicro(promise->fiber);
        }
        return;  // switch -> await
      } break;

      case OpCode::Yield: {
        const auto ins = chunk->Read<serilang::Yield>(ip);
        ip += sizeof(ins);
        fib->state = FiberState::Suspended;

        fib->completion_promise->Resolve(pop(fib->stack));
        fib->ResetPromise();
      }
        return;  // switch -> yield

      //------------------------------------------------------------------
      // 8. Exceptions
      //------------------------------------------------------------------
      case OpCode::Throw: {
        const auto ins = chunk->Read<serilang::Throw>(ip);
        ip += sizeof(ins);
        Error(*fib);
        return;
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
        throw std::runtime_error("Unimplemented instruction at " +
                                 std::to_string(ip - 1));
        break;
    }

    if (fib->state != FiberState::Running)
      break;
  }

  if (fib->frames.empty())
    fib->state = FiberState::Dead;
}

//------------------------------------------------------------------------------
// Event loop helpers
//------------------------------------------------------------------------------
void VM::Enqueue(Fiber* f) {
  if (!f || f->state == FiberState::Dead)
    return;
  f->state = FiberState::Running;
  runq_.push_back(f);
}

void VM::EnqueueMicro(Fiber* f) {
  if (!f || f->state == FiberState::Dead)
    return;
  f->state = FiberState::Running;
  microq_.push_back(f);
}

void VM::ScheduleAt(Fiber* f, std::chrono::steady_clock::time_point when) {
  if (!f || f->state == FiberState::Dead)
    return;
  timers_.push(TimerEntry{when, f});
}

}  // namespace serilang
