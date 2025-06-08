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

## vm.hpp / vm.cpp
Implementation of the virtual machine. `VM` manages fibers, executes bytecode,
handles built‑ins and provides garbage collection utilities.  `ExecuteFiber` in
`vm.cpp` forms the main interpreter loop.

