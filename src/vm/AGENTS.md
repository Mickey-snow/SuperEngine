# VM Module Overview

This directory contains the core bytecode interpreter and object system.  Each
file is briefly documented below to help contributors quickly locate relevant
functionality.

## call_frame.hpp
Defines the `CallFrame` struct used by the VM to store the current function,
instruction pointer and base pointer for each function call.

## disassembler.hpp / disassembler.cpp
Provides the `Disassembler` class which pretty‑prints a `Code` chunk's bytecode.

## gc.hpp / gc.cpp
Implementation of a simple mark‑and‑sweep garbage collector used for all heap
objects. `GarbageCollector` tracks `IObject` instances and sweeps unreachable
ones. `GCVisitor` is a helper used when marking roots.

## instruction.hpp
Defines all bytecode instruction structures and the `OpCode` enum.
`Instruction` is a `std::variant` that can hold any instruction type.  Utility
template `GetOpcode<T>()` maps a concrete instruction type to its opcode value.

## iobject.hpp
Base class for all heap allocated objects that participate in GC.

## objtype.hpp
Enumeration of object kinds used at runtime.

## object.hpp / object.cpp
Defines concrete object types. Implements their behaviour such as calling
functions, method lookup and GC marking.

## value.hpp / value.cpp
Defines the polymorphic `Value` type. `Value` holds primitive types or pointers
to `IObject` instances. Most VM operations interact through this class.

- Operator dispatch is centralized. Primitive semantics are table‑driven via
  `primops.cpp`. `Value::Operator(VM&, Fiber&, Op, Value)` first tries the
  primitive tables, then optional `IObject` fast hooks, then (future) script
  magic methods.
  - Legacy overloads `Value::Operator(Op, Value)` and `Value::Operator(Op)`
    remain for tests; they use only the primitive fast path.

## primops.hpp / primops.cpp
Table‑driven primitive operator implementations. Provides `EvaluateUnary` and
`EvaluateBinary` which return `false` if a given type combination is not
handled. Covers `int`, `double`, `bool`, and `str` (including `str * int`).

## operator_protocol.hpp
Maps `Op` values to Python‑like magic method names (e.g. `__add__`, `__radd__`,
`__iadd__`, `__neg__`, `__invert__`) for future script‑level overloading.

## magic.hpp / magic.cpp
Helper to resolve a magic member on an object. Execution of the resolved
callable is left to the VM; this will be wired up when script magic methods are
enabled.

## iobject.hpp
Base class for all heap allocated objects that participate in GC.

- Optional virtual hooks `UnaryOp` and `BinaryOp` allow native objects to
  fast‑path operators without string lookups. Default implementations return
  `std::nullopt`.
- Optional `Bool()` hook allows custom truthiness for native objects; default
  is `std::nullopt` (non‑null objects are truthy).

## vm.hpp / vm.cpp
Implementation of the virtual machine. `VM` manages fibers, executes bytecode,
handles built‑ins and provides garbage collection utilities.  `ExecuteFiber` in
`vm.cpp` forms the main interpreter loop.
