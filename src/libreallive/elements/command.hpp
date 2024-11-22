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

#pragma once

#include <array>
#include <ostream>
#include <string>

#include "libreallive/elements/bytecode.hpp"
#include "libreallive/expression.hpp"

namespace libreallive {

struct CommandInfo {
  std::array<unsigned char, 8> cmd;
  std::vector<Expression> param;
};

// Command elements.
class CommandElement : public BytecodeElement {
 public:
  explicit CommandElement(const char* src);
  explicit CommandElement(CommandInfo&& cmd);
  virtual ~CommandElement();

  // Identity information.
  virtual int modtype() const { return command[1]; }
  virtual int module() const { return command[2]; }
  virtual int opcode() const { return command[3] | (command[4] << 8); }
  virtual int argc() const { return command[5] | (command[6] << 8); }
  virtual int overload() const { return command[7]; }

  // Gets/Sets the cached parameters.
  void SetParsedParameters(ExpressionPiecesVector p) const;
  const ExpressionPiecesVector& GetParsedParameters() const;

  // Returns the number of parameters.
  virtual size_t GetParamCount() const;
  std::string GetParam(int index) const;

  // Methods that deal with pointers.
  virtual size_t GetLocationCount() const;
  virtual unsigned long GetLocation(int i) const;

  // Fat interface stuff for GotoCase. Prevents casting, etc.
  virtual size_t GetCaseCount() const;
  virtual Expression GetCase(int i) const;

  // Overridden from BytecodeElement:
  virtual std::string GetSourceRepresentation(IModuleManager*) const final {
    return "???";
  }

  virtual std::string GetTagsRepresentation() const;

  virtual Bytecode_ptr DownCast() const override;

 protected:
  static constexpr size_t COMMAND_SIZE = 8;
  std::array<unsigned char, COMMAND_SIZE> command;

  mutable std::vector<Expression> parsed_parameters_;
};

class SelectElement : public CommandElement {
 public:
  static const int OPTION_COLOUR = 0x30;
  static const int OPTION_TITLE = 0x31;
  static const int OPTION_HIDE = 0x32;
  static const int OPTION_BLANK = 0x33;
  static const int OPTION_CURSOR = 0x34;

  struct Condition {
    std::string condition;
    uint8_t effect;
    std::string effect_argument;
  };

  struct Param {
    std::vector<Condition> cond_parsed;
    std::string cond_text;
    std::string text;
    int line;
    Param() : cond_text(), text(), line(0) {}
    Param(const char* tsrc, const size_t tlen, const int lnum)
        : cond_text(), text(tsrc, tlen), line(lnum) {}
    Param(const std::vector<Condition>& conditions,
          const char* csrc,
          const size_t clen,
          const char* tsrc,
          const size_t tlen,
          const int lnum)
        : cond_parsed(conditions),
          cond_text(csrc, clen),
          text(tsrc, tlen),
          line(lnum) {}
  };
  typedef std::vector<Param> params_t;

  explicit SelectElement(const char* src);
  virtual ~SelectElement();

  // Returns the expression in the source code which refers to which window to
  // display.
  Expression GetWindowExpression() const;

  const params_t& raw_params() const { return params; }

  // Overridden from CommandElement:
  virtual size_t GetParamCount() const final;
  virtual std::string GetParam(int i) const final;

  // Overridden from BytecodeElement:
  virtual size_t GetBytecodeLength() const final;

 private:
  std::string repr;
  params_t params;
  int firstline;
  int uselessjunk;
};

class FunctionElement : public CommandElement {
 public:
  FunctionElement(CommandInfo&& cmd, size_t len = 0);
  virtual ~FunctionElement() = default;

  // Overridden from BytecodeElement:
  virtual size_t GetBytecodeLength() const final;
  virtual std::string GetSerializedCommand(RLMachine& machine) const final;

 private:
  size_t length_;
};

class GotoElement : public CommandElement {
 public:
  GotoElement(const char* opcode, const unsigned long& id);
  virtual ~GotoElement();

  // Overridden from CommandElement:
  virtual size_t GetParamCount() const final;
  virtual std::string GetParam(int i) const final;
  virtual size_t GetLocationCount() const final;
  virtual unsigned long GetLocation(int i) const final;
  virtual std::string GetTagsRepresentation() const override;

  // Overridden from BytecodeElement:
  virtual size_t GetBytecodeLength() const final;

 private:
  unsigned long id_;
};

class GotoIfElement : public CommandElement {
 public:
  GotoIfElement(CommandInfo&& cmd, const unsigned long& id, const size_t& len);
  virtual ~GotoIfElement();

  // Overridden from CommandElement:
  virtual size_t GetLocationCount() const final;
  virtual unsigned long GetLocation(int i) const final;
  virtual std::string GetTagsRepresentation() const override;

  // Overridden from BytecodeElement:
  virtual size_t GetBytecodeLength() const final;

 private:
  unsigned long id_;
  size_t length_;
};

class GotoCaseElement : public CommandElement {
 public:
  GotoCaseElement(CommandInfo&& cmd,
                  const size_t& len,
                  const std::vector<unsigned long> ids,
                  const std::vector<Expression>& parsed_cases)
      : CommandElement(std::move(cmd)),
        length_(len),
        id_(std::move(ids)),
        parsed_cases_(parsed_cases) {}

  virtual ~GotoCaseElement();

  // Overridden from CommandElement:
  virtual size_t GetParamCount() const final;
  virtual size_t GetCaseCount() const final;
  virtual Expression GetCase(int i) const final;
  virtual std::string GetTagsRepresentation() const override;

  size_t GetLocationCount() const final;
  unsigned long GetLocation(int) const final;

  // Overridden from BytecodeElement:
  virtual size_t GetBytecodeLength() const final;

 private:
  size_t length_;
  std::vector<unsigned long> id_;
  std::vector<Expression> parsed_cases_;
};

class GotoOnElement : public CommandElement {
 public:
  GotoOnElement(CommandInfo&& cmd,
                std::vector<unsigned long> ids,
                const size_t& len);

  virtual ~GotoOnElement() = default;

  // Overridden from CommandElement:
  size_t GetParamCount() const final;

  size_t GetLocationCount() const final;
  unsigned long GetLocation(int) const final;

  virtual std::string GetTagsRepresentation() const override;

  // Overridden from BytecodeElement:
  size_t GetBytecodeLength() const final;

 private:
  std::vector<unsigned long> id_;
  Expression param_;
  size_t length_;
};

class GosubWithElement : public CommandElement {
 public:
  GosubWithElement(CommandInfo&& cmd,
                   const unsigned long& id,
                   const size_t& len);

  virtual ~GosubWithElement();

  // Overridden from CommandElement:
  virtual size_t GetLocationCount() const final;
  virtual unsigned long GetLocation(int i) const final;
  virtual std::string GetTagsRepresentation() const override;

  // Overridden from BytecodeElement:
  virtual size_t GetBytecodeLength() const final;

 private:
  unsigned long id_;
  size_t length_;
};

}  // namespace libreallive
