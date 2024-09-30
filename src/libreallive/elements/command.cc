// -----------------------------------------------------------------------
//
// This file is part of libreallive, a dependency of RLVM.
//
// -----------------------------------------------------------------------
//
// Copyright (c) 2006, 2007 Peter Jolly
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// -----------------------------------------------------------------------

#include <cassert>
#include <iomanip>
#include <vector>

#include "libreallive/elements/command.h"
#include "machine/module_manager.h"
#include "machine/rlmachine.h"

namespace libreallive {

// -----------------------------------------------------------------------
// Pointers
// -----------------------------------------------------------------------

Pointers::Pointers() {}

Pointers::~Pointers() {}

void Pointers::SetPointers(ConstructionData& cdata) {
  assert(target_ids.size() != 0);
  targets.reserve(target_ids.size());
  for (unsigned int i = 0; i < target_ids.size(); ++i) {
    ConstructionData::offsets_t::const_iterator it =
        cdata.offsets.find(target_ids[i]);
    assert(it != cdata.offsets.end());
    targets.push_back(it->second);
  }
  target_ids.clear();
}

// -----------------------------------------------------------------------
// CommandElement
// -----------------------------------------------------------------------

CommandElement::CommandElement(const char* src) {
  auto stream = reinterpret_cast<const unsigned char*>(src);
  std::copy(stream, stream + 8, command.begin());
}

CommandElement::CommandElement(CommandInfo&& cmd)
    : command(std::move(cmd.cmd)), parsed_parameters_(std::move(cmd.param)) {
  // Because line number metaelements can be placed inside parameters (!?!?!),
  // it's possible that our last parameter consists only of the data for a
  // source line MetaElement. We can't detect this during parsing (because just
  // dropping the parameter will put the stream cursor in the wrong place), so
  // hack this here.
}

CommandElement::~CommandElement() {}

void CommandElement::SetParsedParameters(
    ExpressionPiecesVector parsedParameters) const {
  parsed_parameters_ = std::move(parsedParameters);
}

const ExpressionPiecesVector& CommandElement::GetParsedParameters() const {
  return parsed_parameters_;
}

const size_t CommandElement::GetParamCount() const {
  return parsed_parameters_.size();
}

std::string CommandElement::GetParam(int index) const {
  if (0 <= index && index < parsed_parameters_.size())
    return parsed_parameters_[index]->GetDebugString();
  else
    return {};
}

const size_t CommandElement::GetPointersCount() const { return 0; }

pointer_t CommandElement::GetPointer(int i) const { return pointer_t(); }

size_t CommandElement::GetCaseCount() const { return 0; }

Expression CommandElement::GetCase(int i) const {
  throw Error("Call to CommandElement::GetCase is invalid");
}

std::string CommandElement::GetSourceRepresentation(
    IModuleManager* manager) const {
  std::string repr = manager ? manager->GetCommandName(*this) : "";
  if (repr == "") {
    std::ostringstream op;
    op << "op<" << modtype() << ":" << std::setw(3) << std::setfill('0')
       << module() << ":" << std::setw(5) << std::setfill('0') << opcode()
       << ", " << overload() << ">";
    repr += op.str();
  }

  repr += '(';
  bool first = true;
  for (const auto& param : parsed_parameters_) {
    if (first)
      first = false;
    else
      repr += ", ";
    repr += param->GetDebugString();
  }
  repr += ')';
  return repr;
}

void CommandElement::RunOnMachine(RLMachine& machine) const {
  machine.ExecuteCommand(*this);
}

// -----------------------------------------------------------------------
// SelectElement
// -----------------------------------------------------------------------
// todo: cleanup below
SelectElement::SelectElement(const char* src)
    : CommandElement(src), uselessjunk(0) {
  repr.assign(src, 8);

  src += 8;
  if (*src == '(') {
    const int elen = NextExpression(src);
    repr.append(src, elen);
    src += elen;
  }

  if (*src++ != '{')
    throw Error("SelectElement(): expected `{'");

  if (*src == '\n') {
    firstline = read_i16(src + 1);
    src += 3;
  } else {
    firstline = 0;
  }

  for (int i = 0; i < argc(); ++i) {
    // Skip preliminary metadata.
    while (*src == ',')
      ++src;
    // Read condition, if present.
    const char* cond = src;
    std::vector<Condition> cond_parsed;
    if (*src == '(') {
      ++src;
      while (*src != ')') {
        Condition c;
        if (*src == '(') {
          int len = NextExpression(src);
          c.condition = string(src, len);
          src += len;
        }
        bool seekarg = *src != '2' && *src != '3';
        c.effect = *src;
        ++src;
        if (seekarg && *src != ')' && (*src < '0' || *src > '9')) {
          int len = NextExpression(src);
          c.effect_argument = string(src, len);
          src += len;
        }
        cond_parsed.push_back(c);
      }
      if (*src++ != ')')
        throw Error("SelectElement(): expected `)'");
    }
    size_t clen = src - cond;
    // Read text.
    const char* text = src;
    src += NextString(src);
    size_t tlen = src - text;
    // Add parameter to list.
    if (*src != '\n')
      throw Error("SelectElement(): expected `\\n'");
    int lnum = read_i16(src + 1);
    src += 3;
    params.emplace_back(cond_parsed, cond, clen, text, tlen, lnum);
  }

  // HACK?: In Kotomi's path in CLANNAD, there's a select with empty options
  // outside the count specified by argc().
  //
  // There are comments inside of disassembler.ml that seem to indicate that
  // NULL arguments are allowed. I am not sure if this is a hack or if this is
  // the proper behaviour. Also, why the hell would the official RealLive
  // compiler generate this bytecode. WTF?
  while (*src == '\n') {
    // The only thing allowed other than a 16 bit integer.
    src += 3;
    uselessjunk++;
  }

  if (*src++ != '}')
    throw Error("SelectElement(): expected `}'");
}

SelectElement::~SelectElement() {}

Expression SelectElement::GetWindowExpression() const {
  if (repr[8] == '(') {
    const char* location = repr.c_str() + 9;
    return ExpressionParser::GetExpression(location);
  }
  return ExpressionFactory::IntConstant(-1);
}

const size_t SelectElement::GetParamCount() const { return params.size(); }

string SelectElement::GetParam(int i) const {
  string rv(params[i].cond_text);
  rv.append(params[i].text);
  return rv;
}

const size_t SelectElement::GetBytecodeLength() const {
  size_t rv = repr.size() + 5;
  for (Param const& param : params)
    rv += param.cond_text.size() + param.text.size() + 3;
  rv += (uselessjunk * 3);
  return rv;
}

// -----------------------------------------------------------------------
// FunctionElement
// -----------------------------------------------------------------------

FunctionElement::FunctionElement(CommandInfo&& cmd, size_t len)
    : CommandElement(std::move(cmd)), length_(len) {}

const size_t FunctionElement::GetBytecodeLength() const { return length_; }

std::string FunctionElement::GetSerializedCommand(RLMachine& machine) const {
  std::string rv;
  for (int i = 0; i < COMMAND_SIZE; ++i)
    rv.push_back(command[i]);
  if (parsed_parameters_.size() > 0) {
    rv.push_back('(');
    for (auto const& param : parsed_parameters_)
      rv.append(param->GetSerializedExpression(machine));
    rv.push_back(')');
  }
  return rv;
}

// -----------------------------------------------------------------------
// GotoElement
// -----------------------------------------------------------------------

GotoElement::GotoElement(const char* op, const unsigned long& id)
    : CommandElement(op), id_(id) {}

GotoElement::~GotoElement() {}

const size_t GotoElement::GetParamCount() const {
  // The pointer is not counted as a parameter.
  return 0;
}

string GotoElement::GetParam(int i) const { return std::string(); }

const size_t GotoElement::GetPointersCount() const { return 1; }

pointer_t GotoElement::GetPointer(int i) const {
  if (i != 0)
    throw Error("GotoElement has only 1 pointer");
  return pointer_;
}

std::string GotoElement::GetSourceRepresentation(
    IModuleManager* manager) const {
  std::string repr = CommandElement::GetSourceRepresentation(manager);
  repr += " @" + std::to_string(id_);
  return repr;
}

const size_t GotoElement::GetBytecodeLength() const { return 12; }

void GotoElement::SetPointers(ConstructionData& cdata) {
  ConstructionData::offsets_t::const_iterator it = cdata.offsets.find(id_);
  assert(it != cdata.offsets.end());
  pointer_ = it->second;
}

// -----------------------------------------------------------------------
// GotoIfElement
// -----------------------------------------------------------------------
GotoIfElement::GotoIfElement(CommandInfo&& cmd,
                             const unsigned long& id,
                             const size_t& len)
    : CommandElement(std::move(cmd)), id_(id), length_(len) {}

GotoIfElement::~GotoIfElement() {}

const size_t GotoIfElement::GetPointersCount() const { return 1; }

pointer_t GotoIfElement::GetPointer(int i) const {
  if (i != 0)
    throw Error("GotoIfElement has only 1 pointer");
  return pointer_;
}

std::string GotoIfElement::GetSourceRepresentation(
    IModuleManager* manager) const {
  std::string repr = CommandElement::GetSourceRepresentation(manager);
  repr += " @" + std::to_string(id_);
  return repr;
}

const size_t GotoIfElement::GetBytecodeLength() const { return length_; }

void GotoIfElement::SetPointers(ConstructionData& cdata) {
  ConstructionData::offsets_t::const_iterator it = cdata.offsets.find(id_);
  assert(it != cdata.offsets.end());
  pointer_ = it->second;
}

// -----------------------------------------------------------------------
// GotoCaseElement
// -----------------------------------------------------------------------

GotoCaseElement::~GotoCaseElement() {}

const size_t GotoCaseElement::GetParamCount() const {
  // The cases are not counted as parameters.
  return 1;
}

size_t GotoCaseElement::GetCaseCount() const { return parsed_cases_.size(); }

Expression GotoCaseElement::GetCase(int i) const { return parsed_cases_[i]; }

const size_t GotoCaseElement::GetPointersCount() const {
  return targets_.size();
}

std::string GotoCaseElement::GetSourceRepresentation(
    IModuleManager* manager) const {
  std::string repr = CommandElement::GetSourceRepresentation(manager);
  for (int i = 0; i < targets_.idSize(); ++i) {
    std::string param =
        parsed_cases_[i] ? parsed_cases_[i]->GetDebugString() : "";
    repr += " [" + param + "]@" + std::to_string(targets_.target_ids[i]);
  }
  return repr;
}

pointer_t GotoCaseElement::GetPointer(int i) const { return targets_[i]; }

void GotoCaseElement::SetPointers(ConstructionData& cdata) {
  targets_.SetPointers(cdata);
}

const size_t GotoCaseElement::GetBytecodeLength() const { return length_; }

// -----------------------------------------------------------------------
// GotoOnElement
// -----------------------------------------------------------------------
GotoOnElement::GotoOnElement(CommandInfo&& cmd,
                             const Pointers& targets,
                             const size_t& len)
    : CommandElement(std::move(cmd)), targets_(targets), length_(len) {}

const size_t GotoOnElement::GetParamCount() const { return 1; }

const size_t GotoOnElement::GetPointersCount() const { return targets_.size(); }

pointer_t GotoOnElement::GetPointer(int i) const { return targets_[i]; }

void GotoOnElement::SetPointers(ConstructionData& cdata) {
  targets_.SetPointers(cdata);
}

std::string GotoOnElement::GetSourceRepresentation(
    IModuleManager* manager) const {
  std::string repr = CommandElement::GetSourceRepresentation(manager);

  repr += '{';
  for (size_t i = 0; i < targets_.idSize(); ++i) {
    repr += " @" + std::to_string(targets_.target_ids[i]);
  }
  repr += '}';

  return repr;
}

const size_t GotoOnElement::GetBytecodeLength() const { return length_; }

// -----------------------------------------------------------------------
// GosubWithElement
// -----------------------------------------------------------------------
GosubWithElement::GosubWithElement(CommandInfo&& cmd,
                                   const unsigned long& id,
                                   const size_t& len)
    : CommandElement(std::move(cmd)), id_(id), pointer_{}, length_(len) {}

GosubWithElement::~GosubWithElement() {}

const size_t GosubWithElement::GetPointersCount() const { return 1; }

pointer_t GosubWithElement::GetPointer(int i) const {
  if (i != 0)
    throw Error("GosubWithElement has only 1 pointer");
  return pointer_;
}

std::string GosubWithElement::GetSourceRepresentation(
    IModuleManager* manager) const {
  std::string repr = CommandElement::GetSourceRepresentation(manager);
  repr += " @" + std::to_string(id_);
  return repr;
}

const size_t GosubWithElement::GetBytecodeLength() const { return length_; }

void GosubWithElement::SetPointers(ConstructionData& cdata) {
  ConstructionData::offsets_t::const_iterator it = cdata.offsets.find(id_);
  assert(it != cdata.offsets.end());
  pointer_ = it->second;
}

}  // namespace libreallive
