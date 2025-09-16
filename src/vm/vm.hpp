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

#include "vm/gc.hpp"
#include "vm/object.hpp"
#include "vm/value.hpp"

#include <chrono>
#include <deque>
#include <functional>
#include <iostream>
#include <memory>
#include <queue>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace serilang {

struct Fiber;
struct Chunk;

namespace helper {
inline Value pop(std::vector<Value>& stack) {
  Value val = std::move(stack.back());
  stack.pop_back();
  return val;
}
inline void push(std::vector<Value>& stack, Value v) {
  stack.emplace_back(std::move(v));
}
}  // namespace helper

class VM {
 public:
  explicit VM(std::shared_ptr<GarbageCollector> gc);
  explicit VM(std::shared_ptr<GarbageCollector> gc,
              Dict* globals,
              Dict* builtins);

  Fiber* AddFiber(Code* entry);

  // Run the event loop until no runnable work and no timers remain.
  // Returns the last dead fiber's result if any.
  Value Run();

  /// Pop an exception from f.stack and unwind to the nearest handler.
  void Error(Fiber& f);
  /// Push exc onto f.stack, then unwind to the nearest handler.
  void Error(Fiber& f, std::string exc);

  // REPL support: execute a single chunk, preserving VM state (globals, etc.)
  Value Evaluate(Code* chunk);

  // Trigger garbage collection: mark-root and sweep unreachable objects
  void CollectGarbage();
  // Let the garbage collector track a Value allocated elsewhere
  Value AddTrack(TempValue&& t);

  //----------------------------------------------------------------
  // Event loop scheduling API
  void Enqueue(Fiber* f);       // schedule to run queue (FIFO)
  void EnqueueMicro(Fiber* f);  // schedule to microtask queue (LIFO/FIFO)
  void ScheduleAt(Fiber* f, std::chrono::steady_clock::time_point when);
  void ScheduleAfter(Fiber* f, std::chrono::milliseconds dur) {
    ScheduleAt(f, std::chrono::steady_clock::now() + dur);
  }

 public:
  //----------------------------------------------------------------
  // Public VM state
  std::shared_ptr<GarbageCollector> gc_;
  size_t gc_threshold_ = 1024 * 1024;

  Fiber* main_fiber_ = nullptr;
  std::vector<Fiber*> fibres_;
  Value last_;  // last fiber's return value

  Dict *globals_, *builtins_;
  std::unordered_map<std::string, Module*> module_cache_;

 private:
  //----------------------------------------------------------------
  // Execution helpers
  void Return(Fiber& f);
  Dict* GetNamespace(Fiber& f);

  //----------------------------------------------------------------
  // Core interpreter loop for one fiber
  void ExecuteFiber(Fiber* fib);

  //----------------------------------------------------------------
  // Event loop internals
  struct TimerEntry {
    std::chrono::steady_clock::time_point when;
    Fiber* fib;
    bool operator>(const TimerEntry& other) const { return when > other.when; }
  };

  std::deque<Fiber*> runq_;
  std::deque<Fiber*> microq_;
  std::priority_queue<TimerEntry,
                      std::vector<TimerEntry>,
                      std::greater<TimerEntry>>
      timers_;

  struct IPoller {
    virtual ~IPoller() = default;
    virtual void Wait(std::chrono::milliseconds timeout) {
      if (timeout > std::chrono::milliseconds(0))
        std::this_thread::sleep_for(timeout);
      else
        std::this_thread::yield();
    }
  };
  std::unique_ptr<IPoller> poller_;
};

}  // namespace serilang
