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

#include "vm/future.hpp"

#include "srbind/srbind.hpp"
#include "vm/object.hpp"

#include <chrono>
#include <format>

namespace serilang {
Future::Future() : promise(std::make_shared<Promise>()) {}

std::string Future::Str() const { return Desc(); }
std::string Future::Desc() const { return "<future>"; }

void Future::MarkRoots(GCVisitor& visitor) {
  for (IObject* it : promise->roots)
    visitor.MarkSub(it);
}

// -----------------------------------------------------------------------
static void SubscribeAwaitable(
    VM& vm,
    std::shared_ptr<Promise> owner,
    Value& awaited,
    std::function<void(expected<Value, std::string>)> callback) {
  auto deliver = [cb = std::move(callback)](Promise* pr) {
    if (cb && pr->result.has_value())
      cb(*pr->result);
  };

  owner->AddRoot(awaited);
  std::shared_ptr<Promise> promise = nullptr;

  if (auto tf = awaited.Get_if<Fiber>()) {
    promise = tf->completion_promise;
    if (!promise->result.has_value())
      vm.scheduler_.PushMicroTask(tf);
  } else if (auto ft = awaited.Get_if<Future>()) {
    promise = ft->promise;
  }

  if (!promise) {
    callback(unexpected("Object is not awaitable: " + awaited.Desc()));
    return;
  }

  if (promise->result.has_value())
    deliver(promise.get());
  else
    promise->wakers.emplace_back(std::move(deliver));
}
void InstallAsyncBuiltins(VM& vm) {
  namespace chr = std::chrono;

  srbind::module_ m(vm, "async");

  m.def(
      "Sleep",
      [&vm](int ms, Value result) {
        if (ms < 0)
          ms = 0;
        Future* future = vm.gc_->Allocate<Future>();
        future->AddRoot(result);
        vm.scheduler_.PushCallbackAfter(
            [state = std::weak_ptr(future->promise), r = std::move(result)]() {
              if (auto p = state.lock())
                p->Resolve(std::move(r));
            },
            chr::milliseconds(ms));
        return Value(future);
      },
      srbind::arg("msecs"), srbind::arg("result") = nil);

  m.def(
      "Timeout",
      [&vm](Value awaited, int ms) {
        if (ms < 0)
          ms = 0;
        Future* future = vm.gc_->Allocate<Future>();
        vm.scheduler_.PushCallbackAfter(
            [ms, state = std::weak_ptr(future->promise)]() {
              if (auto p = state.lock())
                p->Reject(std::format("Timeout after {} ms", ms));
            },
            chr::milliseconds(ms));

        SubscribeAwaitable(vm, future->promise, awaited,
                           [state = std::weak_ptr(future->promise)](
                               expected<Value, std::string> outcome) {
                             if (auto p = state.lock()) {
                               if (outcome.has_value())
                                 p->Resolve(outcome.value());
                               else
                                 p->Reject(outcome.error());
                             }
                           });
        return Value(future);
      },
      srbind::arg("awaitable"), srbind::arg("timeout_ms"));

  m.def("Gather", [&vm](std::vector<Value> awaitables) {
    Future* future = vm.gc_->Allocate<Future>();
    if (awaitables.empty()) {
      auto* empty_list = vm.gc_->Allocate<List>();
      future->promise->Resolve(Value(empty_list));
      return Value(future);
    }

    struct GatherData {
      List* results;
      size_t remaining;
    };
    auto data = std::make_shared<GatherData>();
    data->results =
        vm.gc_->Allocate<List>(std::vector<Value>(awaitables.size(), nil));
    data->remaining = awaitables.size();
    future->promise->roots.push_back(data->results);

    for (size_t i = 0; i < awaitables.size(); ++i) {
      Value& awaitable = awaitables[i];

      SubscribeAwaitable(vm, future->promise, awaitable,
                         [data, i, state = std::weak_ptr(future->promise)](
                             expected<Value, std::string> outcome) {
                           if (auto p = state.lock()) {
                             if (!outcome.has_value())
                               p->Reject(outcome.error());
                             else
                               data->results->items[i] = outcome.value();

                             if (--data->remaining == 0)
                               p->Resolve(Value(data->results));
                           }
                         });
    }

    return Value(future);
  });

  m.def("Race", [&vm](std::vector<Value> awaitables) {
    Future* future = vm.gc_->Allocate<Future>();
    if (awaitables.empty()) {
      future->promise->Resolve(nil);
      return Value(future);
    }

    for (Value& awaitable : awaitables) {
      SubscribeAwaitable(vm, future->promise, awaitable,
                         [state = std::weak_ptr(future->promise)](
                             expected<Value, std::string> outcome) {
                           if (auto p = state.lock()) {
                             if (!outcome.has_value())
                               p->Reject(outcome.error());
                             else
                               p->Resolve(outcome.value());
                           }
                         });
    }

    return Value(future);
  });
}

}  // namespace serilang
