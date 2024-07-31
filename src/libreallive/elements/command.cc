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

#include <iomanip>
#include <vector>

#include "libreallive/elements/command.h"
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

CommandElement::CommandElement(const char* src) { memcpy(command, src, 8); }

CommandElement::~CommandElement() {}

std::vector<std::string> CommandElement::GetUnparsedParameters() const {
  std::vector<std::string> parameters;
  size_t param_count = GetParamCount();
  for (size_t i = 0; i < param_count; ++i)
    parameters.push_back(GetParam(i));
  return parameters;
}

bool CommandElement::AreParametersParsed() const {
  return GetParamCount() == parsed_parameters_.size();
}

void CommandElement::SetParsedParameters(
    ExpressionPiecesVector parsedParameters) const {
  parsed_parameters_ = std::move(parsedParameters);
}

const ExpressionPiecesVector& CommandElement::GetParsedParameters() const {
  return parsed_parameters_;
}

const size_t CommandElement::GetPointersCount() const { return 0; }

pointer_t CommandElement::GetPointer(int i) const { return pointer_t(); }

const size_t CommandElement::GetCaseCount() const { return 0; }

const string CommandElement::GetCase(int i) const { return ""; }

std::string CommandElement::GetSourceRepresentation(RLMachine* machine) const {
  std::string repr = machine ? machine->GetCommandName(*this) : "";
  if (repr == "") {
    std::ostringstream op;
    op << "op<" << modtype() << ":" << std::setw(3) << std::setfill('0')
       << module() << ":" << std::setw(5) << std::setfill('0') << opcode()
       << ", " << overload() << ">";
    repr += op.str();
  }

  std::ostringstream param;
  PrintParameterString(param, GetUnparsedParameters());

  repr += param.str();
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

FunctionElement::FunctionElement(const char* src,
                                 const std::vector<string>& params)
    : CommandElement(src), params(params) {}

FunctionElement::~FunctionElement() {}

const size_t FunctionElement::GetParamCount() const {
  // Because line number metaelements can be placed inside parameters (!?!?!),
  // it's possible that our last parameter consists only of the data for a
  // source line MetaElement. We can't detect this during parsing (because just
  // dropping the parameter will put the stream cursor in the wrong place), so
  // hack this here.
  if (!params.empty()) {
    string final = params.back();
    if (final.size() == 3 && final[0] == '\n')
      return params.size() - 1;
  }
  return params.size();
}

string FunctionElement::GetParam(int i) const { return params[i]; }

const size_t FunctionElement::GetBytecodeLength() const {
  if (params.size() > 0) {
    size_t rv(COMMAND_SIZE + 2);
    for (std::string const& param : params)
      rv += param.size();
    return rv;
  } else {
    return COMMAND_SIZE;
  }
}

std::string FunctionElement::GetSerializedCommand(RLMachine& machine) const {
  string rv;
  for (int i = 0; i < COMMAND_SIZE; ++i)
    rv.push_back(command[i]);
  if (params.size() > 0) {
    rv.push_back('(');
    for (string const& param : params) {
      const char* data = param.c_str();
      Expression expression(ExpressionParser::GetData(data));
      rv.append(expression->GetSerializedExpression(machine));
    }
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
  assert(i == 0);
  return pointer_;
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

GotoIfElement::~GotoIfElement() {}

const size_t GotoIfElement::GetParamCount() const {
  // The pointer is not counted as a parameter.
  return repr.size() == 8 ? 0 : 1;
}

string GotoIfElement::GetParam(int i) const {
  return i == 0
             ? (repr.size() == 8 ? string() : repr.substr(9, repr.size() - 10))
             : string();
}

const size_t GotoIfElement::GetPointersCount() const { return 1; }

pointer_t GotoIfElement::GetPointer(int i) const {
  assert(i == 0);
  return pointer_;
}

const size_t GotoIfElement::GetBytecodeLength() const {
  return repr.size() + 4;
}

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

string GotoCaseElement::GetParam(int i) const {
  return i == 0 ? repr_.substr(8, repr_.size() - 8) : string();
}

const size_t GotoCaseElement::GetCaseCount() const { return cases_.size(); }

const string GotoCaseElement::GetCase(int i) const { return cases_[i]; }

const size_t GotoCaseElement::GetPointersCount() const {
  return targets_.size();
}

pointer_t GotoCaseElement::GetPointer(int i) const { return targets_[i]; }

void GotoCaseElement::SetPointers(ConstructionData& cdata) {
  targets_.SetPointers(cdata);
}

const size_t GotoCaseElement::GetBytecodeLength() const {
  size_t rv = repr_.size() + 2;
  for (unsigned int i = 0; i < cases_.size(); ++i)
    rv += cases_[i].size() + 4;
  return rv;
}

// -----------------------------------------------------------------------
// GotoOnElement
// -----------------------------------------------------------------------

GotoOnElement::GotoOnElement(const std::string& repr,
                             Expression cond,
                             const Pointers& targets)
    : CommandElement(repr.c_str()),
      targets_(targets),
      param_(cond),
      repr_(repr) {
  if (targets.idSize() != argc())
    throw Error("Unexpected number of arguments");
}

GotoOnElement::~GotoOnElement() {}

const size_t GotoOnElement::GetParamCount() const { return 1; }

string GotoOnElement::GetParam(int i) const {
  return i == 0 ? repr_.substr(8, repr_.size() - 8) : string();
}

const size_t GotoOnElement::GetPointersCount() const { return targets_.size(); }

pointer_t GotoOnElement::GetPointer(int i) const { return targets_[i]; }

void GotoOnElement::SetPointers(ConstructionData& cdata) {
  targets_.SetPointers(cdata);
}

const size_t GotoOnElement::GetBytecodeLength() const {
  return repr_.size() + argc() * 4 + 2;
}

// -----------------------------------------------------------------------
// GosubWithElement
// -----------------------------------------------------------------------

GosubWithElement::GosubWithElement(const char* src, ConstructionData& cdata)
    : CommandElement(src), repr_size(8) {
  src += 8;
  if (*src == '(') {
    src++;
    repr_size++;

    while (*src != ')') {
      int expr = NextData(src);
      repr_size += expr;
      params_.emplace_back(src, expr);
      src += expr;
    }
    src++;

    repr_size++;
  }

  id_ = read_i32(src);
}

GosubWithElement::~GosubWithElement() {}

const size_t GosubWithElement::GetParamCount() const {
  // The pointer is not counted as a parameter.
  return params_.size();
}

string GosubWithElement::GetParam(int i) const { return params_[i]; }

const size_t GosubWithElement::GetPointersCount() const { return 1; }

pointer_t GosubWithElement::GetPointer(int i) const {
  assert(i == 0);
  return pointer_;
}

const size_t GosubWithElement::GetBytecodeLength() const {
  static constexpr size_t label_size = 4;
  return repr_size + label_size;
}

void GosubWithElement::SetPointers(ConstructionData& cdata) {
  ConstructionData::offsets_t::const_iterator it = cdata.offsets.find(id_);
  assert(it != cdata.offsets.end());
  pointer_ = it->second;
}

}  // namespace libreallive
