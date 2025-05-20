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

#include "machine/op.hpp"
#include "utilities/string_utilities.hpp"
#include "vm/call_frame.hpp"
#include "vm/chunk.hpp"
#include "vm/gc.hpp"
#include "vm/instruction.hpp"
#include "vm/object.hpp"
#include "vm/upvalue.hpp"
#include "vm/value.hpp"

#include <algorithm>
#include <chrono>
#include <stdexcept>

namespace serilang {

VM::VM(std::shared_ptr<Chunk> entry, std::ostream& os)
    : os_(os), main_chunk_(entry) {
  if (entry) {
    // bootstrap: main() as a closure pushed to main fiber
    auto main_cl = gc_.Allocate<Closure>(entry);
    main_cl->entry = 0;
    main_cl->nlocals = 0;
    main_cl->nparams = 0;
    main_fiber_ = gc_.Allocate<Fiber>();
    push(main_fiber_->stack, Value(main_cl));  // fn
    main_cl->Call(*this, *main_fiber_, 0, 0);  // set up base stack frame
    fibres_.push_front(main_fiber_);
  }

  // register native functions
  globals_["time"] = Value(gc_.Allocate<NativeFunction>(
      "time", [](Fiber& f, std::vector<Value> args,
                 std::unordered_map<std::string, Value> /*kwargs*/) {
        if (!args.empty())
          throw std::runtime_error("time() takes no arguments");
        using namespace std::chrono;
        auto now = system_clock::now().time_since_epoch();
        auto secs = duration_cast<seconds>(now).count();
        return Value(static_cast<int>(secs));
      }));

  globals_["print"] = Value(gc_.Allocate<NativeFunction>(
      "print", [&os](Fiber& f, std::vector<Value> args,
                     std::unordered_map<std::string, Value> /*kwargs*/) {
        bool nl = false;
        os << Join(
            ",",
            std::views::all(args) | std::views::filter([&nl](auto const& v) {
              return v == std::monostate() ? false : (nl = true);
            }) | std::views::transform([](auto const& v) { return v.Str(); }));
        if (nl)
          os << '\n';
        return Value();
      }));
}

void VM::CollectGarbage() {
  gc_.MarkRoot(*this);
  gc_.Sweep();
}

Value VM::Run() {
  while (!fibres_.empty()) {
    auto f = fibres_.front();
    if (f->state == FiberState::Dead) {
      last_ = f->last;
      fibres_.pop_front();
    } else {
      ExecuteFiber(f);  // may yield
    }

    if (gc_.AllocatedBytes() >= gc_threshold_) {
      CollectGarbage();
      gc_threshold_ *= 2;
    }
  }
  return last_;
}

Value VM::Evaluate(std::shared_ptr<Chunk> chunk) {
  // wrap chunk in a zero-arg closure
  auto cl = gc_.Allocate<Closure>(chunk);
  cl->entry = 0;
  cl->nparams = 0;
  cl->nlocals = 0;

  // create a new fiber for this snippet
  auto* replFiber = gc_.Allocate<Fiber>();
  push(replFiber->stack, Value(cl));
  cl->Call(*this, *replFiber, 0, 0);
  fibres_.push_front(replFiber);

  return Run();
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
    f.last = std::move(ret);
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
    auto& chunk = *frame.closure->chunk;
    auto& ip = frame.ip;

    if (ip >= chunk.code.size()) {
      Return(*fib);
      return;
    }

    switch (static_cast<OpCode>(chunk[ip++])) {
      //------------------------------------------------------------------
      // 0. No-op
      //------------------------------------------------------------------
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
      // 3. Locals / globals / upvalues
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
        push(fib->stack, globals_[name]);
      } break;

      case OpCode::StoreGlobal: {
        const auto ins = chunk.Read<serilang::StoreGlobal>(ip);
        ip += sizeof(ins);
        auto name =
            chunk.const_pool[ins.name_index].template Get<std::string>();
        globals_[name] = pop(fib->stack);
      } break;

      case OpCode::LoadUpvalue: {
        const auto ins = chunk.Read<serilang::LoadUpvalue>(ip);
        ip += sizeof(ins);
        // (not implemented here)
      } break;

      case OpCode::StoreUpvalue: {
        const auto ins = chunk.Read<serilang::StoreUpvalue>(ip);
        ip += sizeof(ins);
        // (not implemented here)
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
        return;  // unwind to caller

      //------------------------------------------------------------------
      // 5. Function calls
      //------------------------------------------------------------------
      case OpCode::MakeClosure: {
        const auto ins = chunk.Read<serilang::MakeClosure>(ip);
        ip += sizeof(ins);
        auto cl = gc_.Allocate<Closure>(frame.closure->chunk);
        cl->entry = ins.entry;
        cl->nparams = ins.nparams;
        cl->nlocals = ins.nlocals;
        push(fib->stack, Value(cl));
      } break;

      case OpCode::Call: {
        const auto ins = chunk.Read<serilang::Call>(ip);
        ip += sizeof(ins);
        Value& callee = fib->stack.end()[-ins.arity - 1];
        callee.Call(*this, *fib, ins.arity, 0);
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
        fib->stack.emplace_back(gc_.Allocate<List>(std::move(elms)));
      } break;

      case OpCode::MakeDict: {
        const auto ins = chunk.Read<serilang::MakeDict>(ip);
        ip += sizeof(ins);
        std::unordered_map<std::string, Value> elms;
        for (size_t i = fib->stack.size() - 2 * ins.nelms;
             i < fib->stack.size(); i += 2) {
          std::string key = fib->stack[i].Get<std::string>();
          Value val = std::move(fib->stack[i + 1]);
          elms.try_emplace(std::move(key), std::move(val));
        }
        fib->stack.resize(fib->stack.size() - 2 * ins.nelms);
        fib->stack.emplace_back(gc_.Allocate<Dict>(std::move(elms)));
      } break;

      case OpCode::MakeClass: {
        const auto ins = chunk.Read<serilang::MakeClass>(ip);
        ip += sizeof(ins);
        auto klass = gc_.Allocate<Class>();
        klass->name =
            chunk.const_pool[ins.name_index].template Get<std::string>();
        for (int i = 0; i < ins.nmethods; i++) {
          auto method = pop(fib->stack);
          auto name = pop(fib->stack).template Get<std::string>();
          klass->methods[std::move(name)] = std::move(method);
        }
        push(fib->stack, Value(klass));
      } break;

      case OpCode::GetField: {
        const auto ins = chunk.Read<serilang::GetField>(ip);
        ip += sizeof(ins);
        auto inst = pop(fib->stack).Get<Instance*>();
        auto name =
            chunk.const_pool[ins.name_index].template Get<std::string>();
        push(fib->stack, inst->fields[name]);
      } break;

      case OpCode::SetField: {
        const auto ins = chunk.Read<serilang::SetField>(ip);
        ip += sizeof(ins);
        auto val = pop(fib->stack);
        auto inst = pop(fib->stack).Get<Instance*>();
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
        auto f = gc_.Allocate<Fiber>();
        auto cl = gc_.Allocate<Closure>(frame.closure->chunk);
        cl->entry = ins.entry;
        cl->nparams = ins.nparams;
        cl->nlocals = ins.nlocals;
        f->stack.reserve(64);
        push(fib->stack, Value(f));
      } break;

      case OpCode::Resume: {
        const auto ins = chunk.Read<serilang::Resume>(ip);
        ip += sizeof(ins);
        auto arity = ins.arity;
        auto fVal = fib->stack.end()[-arity - 1];
        auto f2 = fVal.template Get<Fiber*>();
        // move args
        for (int i = arity - 1; i >= 0; --i)
          push(f2->stack, pop(fib->stack));
        fib->stack.pop_back();  // pop fiber
        fibres_.push_front(f2);
      }
        return;  // switch → resume

      case OpCode::Yield: {
        const auto ins = chunk.Read<serilang::Yield>(ip);
        ip += sizeof(ins);
        Value y = pop(fib->stack);
        fib->state = FiberState::Suspended;
        fib->last = y;
      }
        return;  // switch → yield

      //------------------------------------------------------------------
      // 8. Exceptions (not implemented yet)
      //------------------------------------------------------------------
      default:
        RuntimeError("Unimplemented instruction at " + std::to_string(ip - 1));
        break;
    }

    if (fib->state != FiberState::Running)
      break;
  }
}

}  // namespace serilang
