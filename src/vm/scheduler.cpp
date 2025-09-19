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

#include "vm/scheduler.hpp"

#include "vm/object.hpp"

#include <thread>

namespace serilang {
namespace chr = std::chrono;

// -----------------------------------------------------------------------
// class IPoller
void IPoller::Wait(std::chrono::milliseconds timeout) {
  if (timeout > std::chrono::milliseconds(0))
    std::this_thread::sleep_for(timeout);
  else
    std::this_thread::yield();
}

// -----------------------------------------------------------------------
// class Scheduler
Scheduler::Scheduler(std::unique_ptr<IPoller> poller)
    : poller_(std::move(poller)) {}

bool Scheduler::IsIdle() const {
  return timers_.empty() && microq_.empty() && runq_.empty();
}

void Scheduler::DrainExpiredTimers() {
  auto now = chr::steady_clock::now();
  while (!timers_.empty() && timers_.top().when <= now) {
    auto t = timers_.top();
    timers_.pop();
    if (t.fib && t.fib->state != FiberState::Dead)
      PushTask(t.fib);
  }
}

Fiber* Scheduler::NextTask() {
  Fiber* next = nullptr;
  if (!microq_.empty()) {
    next = microq_.back();
    microq_.pop_back();
  } else if (!runq_.empty()) {
    next = runq_.front();
    runq_.pop_front();
  }

  return next;
}

void Scheduler::WaitForNext() {
  if (timers_.empty())
    return;

  auto deltaTime = timers_.top().when - chr::steady_clock::now();
  if (deltaTime < chr::steady_clock::duration::zero())
    deltaTime = chr::steady_clock::duration::zero();
  poller_->Wait(chr::duration_cast<chr::milliseconds>(deltaTime));
}

void Scheduler::PushTask(Fiber* f) {
  if (!f || f->state == FiberState::Dead)
    return;
  f->state = FiberState::Running;
  runq_.push_back(f);
}
void Scheduler::PushMicroTask(Fiber* f) {
  if (!f || f->state == FiberState::Dead)
    return;
  f->state = FiberState::Running;
  microq_.push_back(f);
}

void Scheduler::PushAt(Fiber* f, chr::steady_clock::time_point when) {
  if (!f || f->state == FiberState::Dead)
    return;
  timers_.push(TimerEntry{when, f});
}

void Scheduler::PushAfter(Fiber* f, chr::milliseconds duration) {
  PushAt(f, chr::steady_clock::now() + duration);
}

}  // namespace serilang
