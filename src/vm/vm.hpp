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
#include "vm/instruction.hpp"
#include "vm/value.hpp"

#include <deque>
#include <memory>
#include <stdexcept>

namespace serilang {

class VM {
 public:
  explicit VM(std::shared_ptr<Chunk> entry) : main_chunk(entry) {
    // bootstrap: main() as a closure pushed to main fiber
    auto main_cl = std::make_shared<Closure>(entry);
    main_cl->entry = 0;
    main_cl->nlocals = 0;
    main_cl->nparams = 0;
    main_fiber = std::make_shared<Fiber>();
    push(main_fiber->stack, Value(main_cl));  // fn
    CallClosure(*main_fiber, main_cl, 0);     // run main()
    fibers.push_front(main_fiber);
  }

  // single‑step the interpreter until all fibers dead / error
  void Run() {
    while (!fibers.empty()) {
      auto f = fibers.front();
      if (f->state == FiberState::Dead) {
        fibers.pop_front();
        continue;
      }
      ExecuteFiber(f);  // may yield
    }
  }

  //----------------------------------------------------------------
  // host API helpers
  //----------------------------------------------------------------
  // push a constant into the global table for easy tests
  void AddGlobal(std::string name, Value v) {
    globals.emplace(std::move(name), v);
  }

 public:
  //----------------------------------------------------------------
  std::shared_ptr<Chunk> main_chunk;
  std::shared_ptr<Fiber> main_fiber;
  std::deque<std::shared_ptr<Fiber>> fibers{};

  std::unordered_map<std::string, Value> globals;

 private:
  //----------------------------------------------------------------
  //--------------------------- exec helpers ------------------------
  static void push(std::vector<Value>& stack, Value v) {
    stack.emplace_back(std::move(v));
  }
  static Value pop(std::vector<Value>& stack) {
    auto v = std::move(stack.back());
    stack.pop_back();
    return v;
  }

  void RuntimeError(const std::string& msg) { throw std::runtime_error(msg); }

  // call helpers ---------------------------------------------------
  void CallClosure(Fiber& f, std::shared_ptr<Closure> cl, uint8_t arity) {
    if (arity != cl->nparams)
      RuntimeError("arity mismatch");
    const auto base = f.stack.size() - arity - 1;  // fn slot
    f.frames.push_back({cl, cl->entry, base});
    f.stack.resize(base + cl->nlocals);  // allocate locals
  }
  // tail‑call collapses frame
  void TailCall(Fiber& f, std::shared_ptr<Closure> cl, uint8_t arity) {
    auto& frame = f.frames.back();
    const auto base = frame.bp;
    if (arity != cl->nparams)
      RuntimeError("arity mismatch");
    // shift arguments over previous locals
    size_t arg_start = f.stack.size() - arity;
    for (uint8_t i = 0; i < arity; ++i)
      f.stack[base + i] = std::move(f.stack[arg_start + i]);
    f.stack.resize(base + cl->nlocals);
    frame = {cl, cl->entry, base};  // replace frame
  }

  //----------------------------------------------------------------
  void ExecuteFiber(const std::shared_ptr<Fiber>& fib) {
    fib->state = FiberState::Running;
    while (!fib->frames.empty()) {
      auto& frame = fib->frames.back();
      auto& chunk = *frame.closure->chunk;
      auto& code = chunk.code;

      auto fetch = [&]() -> Instruction {
        if (frame.ip < code.size())
          return code[frame.ip++];
        else
          return serilang::Return();
      };

      // main dispatch loop
      std::visit(
          [&](auto ins) {
            using T = std::decay_t<decltype(ins)>;
            //------------------------------------------------------------------
            // 1. Stack ops
            //------------------------------------------------------------------
            if constexpr (std::is_same_v<T, serilang::Push>) {
              push(fib->stack, chunk.const_pool[ins.const_index]);
            } else if constexpr (std::is_same_v<T, serilang::Dup>) {
              push(fib->stack, fib->stack.back());
            } else if constexpr (std::is_same_v<T, serilang::Swap>) {
              std::swap(fib->stack.end()[-1], fib->stack.end()[-2]);
            } else if constexpr (std::is_same_v<T, serilang::Pop>) {
              for (uint8_t i = 0; i < ins.count; ++i)
                fib->stack.pop_back();
              //------------------------------------------------------------------
              // 2. Unary / Binary
              //------------------------------------------------------------------
            } else if constexpr (std::is_same_v<T, serilang::UnaryOp>) {
              auto v = pop(fib->stack);
              Value r = v.Operator(ins.op);
              push(fib->stack, r);
            } else if constexpr (std::is_same_v<T, serilang::BinaryOp>) {
              auto rhs = pop(fib->stack);
              auto lhs = pop(fib->stack);
              Value out = lhs.Operator(ins.op, std::move(rhs));
              push(fib->stack, out);
              //------------------------------------------------------------------
              // 3. Locals / globals / upvals
              //------------------------------------------------------------------
            } else if constexpr (std::is_same_v<T, serilang::LoadLocal>) {
              push(fib->stack,
                   *fib->local_slot(fib->frames.size() - 1, ins.slot));
            } else if constexpr (std::is_same_v<T, serilang::StoreLocal>) {
              *fib->local_slot(fib->frames.size() - 1, ins.slot) =
                  pop(fib->stack);
            } else if constexpr (std::is_same_v<T, serilang::LoadGlobal>) {
              auto name =
                  chunk.const_pool[ins.name_index].template Get<std::string>();
              push(fib->stack, globals[name]);
            } else if constexpr (std::is_same_v<T, serilang::StoreGlobal>) {
              auto name =
                  chunk.const_pool[ins.name_index].template Get<std::string>();
              globals[name] = pop(fib->stack);
            } else if constexpr (std::is_same_v<T, serilang::LoadUpvalue>) {
              push(fib->stack, frame.closure->up[ins.slot]->get());
            } else if constexpr (std::is_same_v<T, serilang::StoreUpvalue>) {
              frame.closure->up[ins.slot]->set(pop(fib->stack));
            } else if constexpr (std::is_same_v<T, serilang::CloseUpvalues>) {
              auto* base =
                  fib->local_slot(fib->frames.size() - 1, ins.from_slot);
              fib->close_upvalues_from(base);
              //------------------------------------------------------------------
              // 4. Control flow
              //------------------------------------------------------------------
            } else if constexpr (std::is_same_v<T, serilang::Jump>) {
              frame.ip += ins.offset;
            } else if constexpr (std::is_same_v<T, serilang::JumpIfTrue>) {
              if (fib->stack.back().IsTruthy())
                frame.ip += ins.offset;
            } else if constexpr (std::is_same_v<T, serilang::JumpIfFalse>) {
              if (!fib->stack.back().IsTruthy())
                frame.ip += ins.offset;
            } else if constexpr (std::is_same_v<T, serilang::Return>) {
              Value ret = fib->stack.empty() ? Value() : fib->stack.back();
              fib->stack.resize(frame.bp);
              fib->frames.pop_back();
              if (fib->frames.empty()) {  // fiber finished
                fib->state = FiberState::Dead;
                fib->last = ret;
                return;
              }
              push(fib->stack, ret);
              return;  // let scheduler switch
              //------------------------------------------------------------------
              // 5. Function calls
              //------------------------------------------------------------------
            } else if constexpr (std::is_same_v<T, serilang::MakeClosure>) {
              auto cl = std::make_shared<Closure>(frame.closure->chunk);
              cl->entry = ins.entry;
              cl->nparams = ins.nparams;
              cl->nlocals = ins.nlocals;
              cl->up.resize(ins.nupvals);
              // capture upvalues descriptors (already on stack as ints)
              for (int i = ins.nupvals - 1; i >= 0; --i) {
                auto uv_slot = static_cast<uint8_t>(pop(fib->stack).Get<int>());
                cl->up[i] = fib->capture_upvalue(
                    fib->local_slot(fib->frames.size() - 1, uv_slot));
              }
              push(fib->stack, Value(cl));
            } else if constexpr (std::is_same_v<T, serilang::Call>) {
              auto arity = ins.arity;
              auto fnVal = fib->stack.end()[-arity - 1];
              auto cl = fnVal.template Get_if<std::shared_ptr<Closure>>();
              if (!cl)
                RuntimeError("call non‑closure");
              CallClosure(*fib, cl, arity);
            } else if constexpr (std::is_same_v<T, serilang::TailCall>) {
              auto arity = ins.arity;
              auto fnVal = fib->stack.end()[-arity - 1];
              auto cl = fnVal.template Get_if<std::shared_ptr<Closure>>();
              if (!cl)
                RuntimeError("tailcall non‑closure");
              TailCall(*fib, cl, arity);
              //------------------------------------------------------------------
              // 6. Object ops  (very naive)
              //------------------------------------------------------------------
            } else if constexpr (std::is_same_v<T, serilang::MakeClass>) {
              auto klass = std::make_shared<Class>();
              klass->name =
                  chunk.const_pool[ins.name_index].template Get<std::string>();
              for (int i = 0; i < ins.nmethods; i++) {
                auto method = pop(fib->stack);
                auto name = pop(fib->stack).template Extract<std::string>();
                klass->methods[std::move(name)] = std::move(method);
              }
              push(fib->stack, Value(klass));
            } else if constexpr (std::is_same_v<T, serilang::New>) {
              auto klass = pop(fib->stack).Extract<std::shared_ptr<Class>>();
              push(fib->stack, Value(std::make_shared<Instance>(klass)));
            } else if constexpr (std::is_same_v<T, serilang::GetField>) {
              auto inst = pop(fib->stack).Extract<std::shared_ptr<Instance>>();
              auto name =
                  chunk.const_pool[ins.name_index].template Get<std::string>();
              push(fib->stack, inst->fields[name]);
            } else if constexpr (std::is_same_v<T, serilang::SetField>) {
              auto val = pop(fib->stack);
              auto inst = pop(fib->stack).Extract<std::shared_ptr<Instance>>();
              auto name =
                  chunk.const_pool[ins.name_index].template Get<std::string>();
              inst->fields[name] = val;
              push(fib->stack, val);
              //------------------------------------------------------------------
              // 7. Coroutines
              //------------------------------------------------------------------
            } else if constexpr (std::is_same_v<T, serilang::MakeFiber>) {
              auto f = std::make_shared<Fiber>();
              auto cl = std::make_shared<Closure>(frame.closure->chunk);
              cl->entry = ins.entry;
              cl->nparams = ins.nparams;
              cl->nlocals = ins.nlocals;
              cl->up.resize(ins.nupvals);  // TODO: capture
              f->stack.reserve(64);
              push(fib->stack, Value(f));
            } else if constexpr (std::is_same_v<T, serilang::Resume>) {
              auto arity = ins.arity;
              auto fVal = fib->stack.end()[-arity - 1];
              auto f2 = fVal.template Get<std::shared_ptr<Fiber>>();
              // move args
              for (int i = arity - 1; i >= 0; --i) {
                push(f2->stack, pop(fib->stack));
              }
              fib->stack.pop_back();  // pop fiber
              fibers.push_front(f2);
              return;  // switch
            } else if constexpr (std::is_same_v<T, serilang::Yield>) {
              Value y = pop(fib->stack);
              fib->state = FiberState::Suspended;
              fib->last = y;
              return;
              //------------------------------------------------------------------
              // 8. Exceptions
              //------------------------------------------------------------------
            } else if constexpr (std::is_same_v<T, serilang::Throw>) {
              RuntimeError("throw not implemented");
            } else if constexpr (std::is_same_v<T, serilang::TryBegin> ||
                                 std::is_same_v<T, serilang::TryEnd>) {
            } else {
              throw std::runtime_error("Unimplemented instruction");
            }
            //------------------------------------------------------------------
          },
          fetch());  // end visit

      if (fib->state != FiberState::Running)
        break;
    }
  }
};

}  // namespace serilang
