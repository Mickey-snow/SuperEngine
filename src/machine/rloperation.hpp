// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2006, 2007 Elliot Glaysher
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

#include "libreallive/bytecode_fwd.hpp"
#include "machine/rloperation/reference_types.hpp"
#include "rloperation/basic_types.hpp"

#include <string>
#include <type_traits>
#include <utility>
#include <vector>

class MappedRLModule;
class RLModule;

// @defgroup RLOperationGroup RLOperation and its type system
//
// Defines the base classes from which all of the opcode
// implementations derive from. This hierarchy of classes works by
// having one of your operation classes, which handles a specific
// prototype of a specific opcode, derive from one of the subclasses
// of RLOperation, specifically RLOpcode<> and RLOpcodeStore<>. The
// template parameters of these subclasses refer to the types of the
// parameters, some of which can be composed to represent more complex
// parameters.
//
// Valid type parameters are IntConstant_T, IntReference_T,
// StrConstant_T, StrReference_T, Argc_T< U > (takes another type as a
// parameter). The type parameters change the arguments to the
// implementation function.
//
// Let's say we want to implement an operation with the following
// prototype: <tt>fun (store)doSomething(str, intC+)</tt>. The
// function returns an integer value to the store register, so we want
// to derive the implementation struct from RLOpcodeStore<>, which will
// automatically place the return value in the store register. Our
// first parameter is a reference to a piece of string memory, so our
// first template argument is StrReference_T. We then take a variable
// number of ints, so we compose IntConstant_T into the template
// Argc_T for our second parameter.
//
// Thus, our sample operation would be implemented with this:
//
// @code
// struct Operation
//     : public RLOpcodeStore<StrReference_T, Argc_T< IntConstant_T> > {
//   int operator()(RLMachine& machine, StringReferenceIterator x,
//                  vector<int> y) {
//     // Do whatever with the input parameters...
//     return 5;
//   }
// };
// @endcode
//
// For information on how to group RLOperations into modules to
// attach to an RLMachine instance, please see @ref ModulesOpcodes
// "Modules and Opcode Definitions".

// Each RLOperation can optionally carry some numeric properties.
enum OperationProperties { PROP_NAME, PROP_FGBG, PROP_OBJSET };

// An RLOperation object implements an individual bytecode
// command. All command bytecodes have a corresponding instance of a
// subclass of RLOperation that defines it.
//
// RLOperations are grouped into RLModule s, which are then added to
// the RLMachine.
class RLOperation {
 public:
  // Default constructor
  RLOperation();
  virtual ~RLOperation();

  void SetName(const std::string& name) { name_ = name; }
  const std::string& Name() const { return name_; }

  RLOperation* SetProperty(int property, int value);
  bool GetProperty(int property, int& value) const;

  // The Dispatch function is implemented on a per type basis and is called by
  // the Module, after checking to make sure that the
  virtual void Dispatch(
      RLMachine& machine,
      const libreallive::ExpressionPiecesVector& parameters) = 0;

  // The public interface used by the RLModule; how a method is Dispatched.
  virtual void DispatchFunction(RLMachine& machine,
                                const libreallive::CommandElement& f);

 private:
  friend class RLModule;
  friend class MappedRLModule;

  typedef std::pair<int, int> Property;
  typedef std::vector<Property> PropertyList;

  // Searches for a property on this object.
  PropertyList::iterator FindProperty(int property) const;

  // Our properties (for the number of properties O(n) is faster than O(log
  // n)...)
  std::unique_ptr<PropertyList> property_list_;

  // The module that owns us (we ask it for properties).
  RLModule* module_ = nullptr;

  // The human readable name for this operation
  std::string name_;
};

// Implements a special case operation. This should be used with
// things that don't follow the usually function syntax in the
// bytecode, such as weird gotos, etc.
//
// RLOp_SpecialCase gives you complete control of the Dispatch,
// performing no type checking, no parameter conversion, and no
// implicit instruction pointer advancement.
//
// @warning This is almost certainly not what you want. This is only
// used to define handlers for CommandElements that aren't
// FunctionElements. Meaning the Gotos and Select. Also, you get to do
// weird tricks with the
class RLOp_SpecialCase : public RLOperation {
 public:
  // Empty function defined simply to obey the interface
  virtual void Dispatch(
      RLMachine& machine,
      const libreallive::ExpressionPiecesVector& parameters) override;

  virtual void DispatchFunction(RLMachine& machine,
                                const libreallive::CommandElement& f) override;

  // Method that is overridden by all subclasses to implement the
  // function of this opcode
  virtual void operator()(RLMachine&, const libreallive::CommandElement&) = 0;
};

// This is the fourth time we have overhauled the implementation of RLOperation
// and we've become exceedingly efficient at it.
template <typename... Args>
class RLNormalOpcode : public RLOperation {
 public:
  RLNormalOpcode();
  virtual ~RLNormalOpcode();
};

template <typename... Args>
RLNormalOpcode<Args...>::RLNormalOpcode() {}

template <typename... Args>
RLNormalOpcode<Args...>::~RLNormalOpcode() {}

template <typename... Args>
class RLOpcode : public RLNormalOpcode<Args...> {
 public:
  RLOpcode();
  virtual ~RLOpcode() override;

  virtual void Dispatch(
      RLMachine& machine,
      const libreallive::ExpressionPiecesVector& parameters) final;

  virtual void operator()(RLMachine&, typename Args::type...) = 0;

 private:
  template <std::size_t... I>
  void DispatchImpl(RLMachine& machine,
                    const std::tuple<typename Args::type...>& args,
                    std::index_sequence<I...> /*unused*/) {
    operator()(machine, std::get<I>(args)...);
  }
};

template <typename... Args>
RLOpcode<Args...>::RLOpcode() {}

template <typename... Args>
RLOpcode<Args...>::~RLOpcode() {}

template <typename... Args>
void RLOpcode<Args...>::Dispatch(
    RLMachine& machine,
    const libreallive::ExpressionPiecesVector& params) {
  unsigned int position = 0;
  const auto args = std::tuple<typename Args::type...>{
      Args::getData(machine, params, position)...};
  DispatchImpl(machine, args, std::make_index_sequence<sizeof...(Args)>());
}

template <>
void RLOpcode<>::Dispatch(
    RLMachine& machine,
    const libreallive::ExpressionPiecesVector& parameters);
