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
#include "utilities/string_utilities.hpp"
#include "vm/call_frame.hpp"
#include "vm/chunk.hpp"
#include "vm/instruction.hpp"
#include "vm/upvalue.hpp"
#include "vm/value.hpp"
#include "vm/value_internal/class.hpp"
#include "vm/value_internal/closure.hpp"
#include "vm/value_internal/fiber.hpp"
#include "vm/value_internal/list.hpp"
#include "vm/value_internal/native_function.hpp"

#include <chrono>
#include <deque>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace serilang {

class VM {
 private:
  std::ostream& os_;

 public:
  explicit VM(std::shared_ptr<Chunk> entry, std::ostream& os = std::cout)
      : os_(os), main_chunk(entry) {
    // bootstrap: main() as a closure pushed to main fiber
    auto main_cl = std::make_shared<Closure>(entry);
    main_cl->entry = 0;
    main_cl->nlocals = 0;
    main_cl->nparams = 0;
    main_fiber = std::make_shared<Fiber>();
    push(main_fiber->stack, Value(main_cl));  // fn
    Call(*main_fiber, main_cl, 0);            // run main()
    fibers.push_front(main_fiber);

    // register native functions
    globals["time"] = Value(std::make_shared<NativeFunction>(
        "time", [](VM& vm, std::vector<Value> args) {
          if (!args.empty())
            vm.RuntimeError("time() takes no arguments");
          using namespace std::chrono;
          auto now = system_clock::now().time_since_epoch();
          auto secs = duration_cast<seconds>(now).count();
          return Value(static_cast<int>(secs));
        }));
    globals["print"] = Value(std::make_shared<NativeFunction>(
        "print", [](VM& vm, std::vector<Value> args) {
          bool nl = false;
          vm.os_ << Join(
              ",",
              std::views::all(args) | std::views::filter([&nl](const Value& v) {
                return v == std::monostate() ? false : (nl = true);
              }) | std::views::transform([](const Value& v) {
                return v.Str();
              }));
          if (nl)
            vm.os_ << '\n';
          return Value();
        }));
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

  // REPL support: execute a single chunk, preserving VM state (globals, etc.)
  Value Evaluate(std::shared_ptr<Chunk> chunk) {
    // wrap chunk in a zero-arg closure
    auto cl = std::make_shared<Closure>(chunk);
    cl->entry = 0;
    cl->nparams = 0;
    cl->nlocals = 0;

    // create a new fiber for this snippet
    auto replFiber = std::make_shared<Fiber>();
    push(replFiber->stack, Value(cl));
    Call(*replFiber, cl, /*arity=*/0);
    fibers.push_front(replFiber);

    // run until this fiber finishes
    while (replFiber->state != FiberState::Dead) {
      ExecuteFiber(replFiber);
    }

    // clean up from the scheduler queue
    if (!fibers.empty() && fibers.front() == replFiber) {
      fibers.pop_front();
    }

    // return the last yielded/returned value
    return replFiber->last;
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
  void Call(Fiber& f, std::shared_ptr<Closure> cl, uint8_t arity) {
    if (arity != cl->nparams)
      RuntimeError("arity mismatch");
    const auto base = f.stack.size() - arity - 1;  // fn slot
    f.frames.push_back({cl, cl->entry, base});
    f.stack.resize(base + cl->nlocals);  // allocate locals
  }
  void Call(Fiber& f, std::shared_ptr<NativeFunction> fn, uint8_t arity) {
    std::vector<Value> args;
    args.reserve(arity);
    for (size_t i = f.stack.size() - arity; i < f.stack.size(); ++i)
      args.emplace_back(std::move(f.stack[i]));

    f.stack.resize(f.stack.size() - arity);
    f.stack.back() = fn->Call(*this, std::move(args));
  }
  void Call(Fiber& f, std::shared_ptr<Class> cl, uint8_t arity) {
    f.stack.resize(f.stack.size() - arity);
    auto inst = std::make_shared<Instance>(cl);
    inst->fields = cl->methods;
    f.stack.back() = Value(std::move(inst));
  }
  void DispatchCall(Fiber& f, Value& v, uint8_t arity) {
    switch (v.Type()) {
      case ObjType::Native:  // native function
        Call(f, v.template Get<std::shared_ptr<NativeFunction>>(), arity);
        break;
      case ObjType::Closure:  // function
        Call(f, v.template Get<std::shared_ptr<Closure>>(), arity);
        break;
      case ObjType::Class:  // instantiate class
        Call(f, v.template Get<std::shared_ptr<Class>>(), arity);
        break;

      default:
        RuntimeError(v.Desc() + " not callable");
    }
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
  void Return(Fiber& f) {
    auto& frame = f.frames.back();
    Value ret = f.stack.empty() ? Value() : f.stack.back();
    f.stack.resize(frame.bp + 1 /* for return value */);

    f.frames.pop_back();
    if (f.frames.empty()) {  // fiber finished
      f.state = FiberState::Dead;
      f.last = std::move(ret);
    } else {  // push return value back on stack
      f.stack.back() = std::move(ret);
    }
  }

  //----------------------------------------------------------------
  void ExecuteFiber(const std::shared_ptr<Fiber>& fib) {
    fib->state = FiberState::Running;
    while (!fib->frames.empty()) {
      auto& frame = fib->frames.back();
      auto& chunk = *frame.closure->chunk;
      auto& ip = frame.ip;

      if (ip >= chunk.code.size()) {
        Return(*fib);
        return;
      }

      // main dispatch switch
      switch (static_cast<OpCode>(chunk[ip++])) {
        case OpCode::Nop:
          break;

          //------------------------------------------------------------------
          // 1. Stack Manipulation
          //------------------------------------------------------------------
        case OpCode::Push: {
          const auto ins = chunk.Read<serilang::Push>(ip);
          ip += sizeof(ins);
          push(fib->stack, chunk.const_pool[ins.const_index]);
        } break;
        case OpCode::Dup: {
          const auto ins = chunk.Read<serilang::Dup>(ip);
          ip += sizeof(ins);
          push(fib->stack, fib->stack.end()[-ins.top_ofs - 1]);
        } break;
        case OpCode::Swap: {
          const auto ins = chunk.Read<serilang::Swap>(ip);
          ip += sizeof(ins);
          std::swap(fib->stack.end()[-1], fib->stack.end()[-2]);
        } break;
        case OpCode::Pop: {
          const auto ins = chunk.Read<serilang::Pop>(ip);
          ip += sizeof(ins);
          for (uint8_t i = 0; i < ins.count; ++i)
            fib->stack.pop_back();
        } break;

          //------------------------------------------------------------------
          // 2. Unary / Binary
          //------------------------------------------------------------------
        case OpCode::UnaryOp: {
          const auto ins = chunk.Read<serilang::UnaryOp>(ip);
          ip += sizeof(ins);
          auto v = pop(fib->stack);
          Value r = v.Operator(ins.op);
          push(fib->stack, r);
        } break;
        case OpCode::BinaryOp: {
          const auto ins = chunk.Read<serilang::BinaryOp>(ip);
          ip += sizeof(ins);
          auto rhs = pop(fib->stack);
          auto lhs = pop(fib->stack);
          Value out = lhs.Operator(ins.op, std::move(rhs));
          push(fib->stack, out);
        } break;

          //------------------------------------------------------------------
          // 3. Locals / globals / upvals
          //------------------------------------------------------------------
        case OpCode::LoadLocal: {
          const auto ins = chunk.Read<serilang::LoadLocal>(ip);
          ip += sizeof(ins);
          push(fib->stack, *fib->local_slot(fib->frames.size() - 1, ins.slot));
        } break;
        case OpCode::StoreLocal: {
          const auto ins = chunk.Read<serilang::StoreLocal>(ip);
          ip += sizeof(ins);
          *fib->local_slot(fib->frames.size() - 1, ins.slot) = pop(fib->stack);
        } break;
        case OpCode::LoadGlobal: {
          const auto ins = chunk.Read<serilang::LoadGlobal>(ip);
          ip += sizeof(ins);
          auto name =
              chunk.const_pool[ins.name_index].template Get<std::string>();
          push(fib->stack, globals[name]);
        } break;
        case OpCode::StoreGlobal: {
          const auto ins = chunk.Read<serilang::StoreGlobal>(ip);
          ip += sizeof(ins);
          auto name =
              chunk.const_pool[ins.name_index].template Get<std::string>();
          globals[name] = pop(fib->stack);
        } break;
        case OpCode::LoadUpvalue: {
          const auto ins = chunk.Read<serilang::LoadUpvalue>(ip);
          ip += sizeof(ins);
          push(fib->stack, frame.closure->up[ins.slot]->get());
        } break;
        case OpCode::StoreUpvalue: {
          const auto ins = chunk.Read<serilang::StoreUpvalue>(ip);
          ip += sizeof(ins);
          frame.closure->up[ins.slot]->set(pop(fib->stack));
        } break;
        case OpCode::CloseUpvalues: {
          const auto ins = chunk.Read<serilang::CloseUpvalues>(ip);
          ip += sizeof(ins);
          auto* base = fib->local_slot(fib->frames.size() - 1, ins.from_slot);
          fib->close_upvalues_from(base);
        } break;

          //------------------------------------------------------------------
          // 4. Control flow
          //------------------------------------------------------------------
        case OpCode::Jump: {
          const auto ins = chunk.Read<serilang::Jump>(ip);
          ip += sizeof(ins);
          frame.ip += ins.offset;

        } break;
        case OpCode::JumpIfTrue: {
          const auto ins = chunk.Read<serilang::JumpIfTrue>(ip);
          ip += sizeof(ins);
          auto cond = pop(fib->stack);
          if (cond.IsTruthy())
            frame.ip += ins.offset;
        } break;
        case OpCode::JumpIfFalse: {
          const auto ins = chunk.Read<serilang::JumpIfFalse>(ip);
          ip += sizeof(ins);
          auto cond = pop(fib->stack);
          if (!cond.IsTruthy())
            frame.ip += ins.offset;
        } break;
        case OpCode::Return: {
          const auto ins = chunk.Read<serilang::Return>(ip);
          ip += sizeof(ins);
          Return(*fib);
        }
          return;  //  switch

          //------------------------------------------------------------------
          // 5. Function calls
          //------------------------------------------------------------------
        case OpCode::MakeClosure: {
          const auto ins = chunk.Read<serilang::MakeClosure>(ip);
          ip += sizeof(ins);
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
        } break;
        case OpCode::Call: {
          const auto ins = chunk.Read<serilang::Call>(ip);
          ip += sizeof(ins);
          DispatchCall(*fib, fib->stack.end()[-ins.arity - 1], ins.arity);
        } break;
        case OpCode::TailCall: {
          const auto ins = chunk.Read<serilang::TailCall>(ip);
          ip += sizeof(ins);
          auto arity = ins.arity;
          Value& fnVal = fib->stack.end()[-arity - 1];
          auto cl = fnVal.template Get_if<std::shared_ptr<Closure>>();
          if (!cl)
            RuntimeError("tailcall non‑closure");
          TailCall(*fib, cl, arity);
        } break;

          //------------------------------------------------------------------
          // 6. Object ops
          //------------------------------------------------------------------
        case OpCode::MakeList: {
          const auto ins = chunk.Read<serilang::MakeList>(ip);
          ip += sizeof(ins);
          std::vector<Value> elms;
          elms.reserve(ins.nelms);
          for (size_t i = fib->stack.size() - ins.nelms; i < fib->stack.size();
               ++i)
            elms.emplace_back(std::move(fib->stack[i]));
          fib->stack.resize(fib->stack.size() - ins.nelms);
          fib->stack.emplace_back(make_list(std::move(elms)));
        } break;

        case OpCode::MakeClass: {
          const auto ins = chunk.Read<serilang::MakeClass>(ip);
          ip += sizeof(ins);
          auto klass = std::make_shared<Class>();
          klass->name =
              chunk.const_pool[ins.name_index].template Get<std::string>();
          for (int i = 0; i < ins.nmethods; i++) {
            auto method = pop(fib->stack);
            auto name = pop(fib->stack).template Extract<std::string>();
            klass->methods[std::move(name)] = std::move(method);
          }
          push(fib->stack, Value(klass));
        } break;

        case OpCode::GetField: {
          const auto ins = chunk.Read<serilang::GetField>(ip);
          ip += sizeof(ins);
          auto inst = pop(fib->stack).Extract<std::shared_ptr<Instance>>();
          auto name =
              chunk.const_pool[ins.name_index].template Get<std::string>();
          push(fib->stack, inst->fields[name]);
        } break;

        case OpCode::SetField: {
          const auto ins = chunk.Read<serilang::SetField>(ip);
          ip += sizeof(ins);
          auto val = pop(fib->stack);
          auto inst = pop(fib->stack).Extract<std::shared_ptr<Instance>>();
          auto name =
              chunk.const_pool[ins.name_index].template Get<std::string>();
          inst->fields[name] = val;
          push(fib->stack, val);
        } break;

          //------------------------------------------------------------------
          // 7. Coroutines
          //------------------------------------------------------------------
        case OpCode::MakeFiber: {
          const auto ins = chunk.Read<serilang::MakeFiber>(ip);
          ip += sizeof(ins);
          auto f = std::make_shared<Fiber>();
          auto cl = std::make_shared<Closure>(frame.closure->chunk);
          cl->entry = ins.entry;
          cl->nparams = ins.nparams;
          cl->nlocals = ins.nlocals;
          cl->up.resize(ins.nupvals);  // TODO: capture
          f->stack.reserve(64);
          push(fib->stack, Value(f));
        } break;
        case OpCode::Resume: {
          const auto ins = chunk.Read<serilang::Resume>(ip);
          ip += sizeof(ins);
          auto arity = ins.arity;
          auto fVal = fib->stack.end()[-arity - 1];
          auto f2 = fVal.template Get<std::shared_ptr<Fiber>>();
          // move args
          for (int i = arity - 1; i >= 0; --i) {
            push(f2->stack, pop(fib->stack));
          }
          fib->stack.pop_back();  // pop fiber
          fibers.push_front(f2);
        }
          return;  // switch
        case OpCode::Yield: {
          const auto ins = chunk.Read<serilang::Yield>(ip);
          ip += sizeof(ins);
          Value y = pop(fib->stack);
          fib->state = FiberState::Suspended;
          fib->last = y;
        }
          return;  // switch

          //------------------------------------------------------------------
          // 8. Exceptions (not implemented yet)
          //------------------------------------------------------------------
        default:
          RuntimeError("Unimplemented instruction at " +
                       std::to_string(ip - 1));
          break;
      }

      if (fib->state != FiberState::Running)
        break;
    }
  }
};

}  // namespace serilang
