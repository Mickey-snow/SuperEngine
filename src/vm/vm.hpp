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
#include "vm/value.hpp"

#include <deque>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace serilang {

struct Fiber;
struct Chunk;

class VM {
 public:
  // Construct a VM, optionally bootstrapping with an entry chunk.
  static VM Create(std::shared_ptr<Chunk> bootstrap = nullptr,
                   std::ostream& stdout = std::cout,
                   std::istream& stdin = std::cin,
                   std::ostream& stderr = std::cerr);

  explicit VM(std::shared_ptr<Chunk> entry);

  // Trigger garbage collection: mark-root and sweep unreachable objects
  void CollectGarbage();
  // Let the garbage collector track a Value allocated elsewhere
  Value AddTrack(TempValue&& t);

  // Run until all fibers die or error; returns last fiber's result
  Value Run();

  // REPL support: execute a single chunk, preserving VM state (globals, etc.)
  Value Evaluate(std::shared_ptr<Chunk> chunk);

  // For adding built-in as global
  void AddGlobal(std::string key, TempValue&& v);

 public:
  //----------------------------------------------------------------
  // Public VM state
  GarbageCollector gc_;
  size_t gc_threshold_ = 1024 * 1024;

  Fiber* main_fiber_ = nullptr;
  std::deque<Fiber*> fibres_;
  Value last_;  // last fiber's return value

  std::unordered_map<std::string_view,
                     Value,
                     std::hash<std::string_view>,
                     std::equal_to<>>
      globals_;

 private:
  //----------------------------------------------------------------
  // Execution helpers
  static void push(std::vector<Value>& stack, Value v);
  static Value pop(std::vector<Value>& stack);

  void RuntimeError(const std::string& msg);
  void Return(Fiber& f);

  //----------------------------------------------------------------
  // Core interpreter loop for one fiber
  void ExecuteFiber(Fiber* fib);
};

}  // namespace serilang
