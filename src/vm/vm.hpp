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

#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace serilang {

struct Fiber;
struct Chunk;

class VM {
 public:
  explicit VM(std::shared_ptr<GarbageCollector> gc);
  explicit VM(std::shared_ptr<GarbageCollector> gc,
              Dict* globals,
              Dict* builtins);

  Fiber* AddFiber(Code* entry);

  // Run until all fibers die or error; returns last fiber's result
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
  static void push(std::vector<Value>& stack, Value v);
  static Value pop(std::vector<Value>& stack);
  void Return(Fiber& f);
  Dict* GetNamespace(Fiber& f);

  //----------------------------------------------------------------
  // Core interpreter loop for one fiber
  void ExecuteFiber(Fiber* fib);
};

}  // namespace serilang
