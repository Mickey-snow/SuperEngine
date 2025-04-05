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

#include "m6/exception.hpp"
#include "m6/token.hpp"
#include "m6/tokenizer.hpp"
#include "machine/instruction.hpp"

#include <memory>
#include <span>
#include <string>
#include <vector>

class RLMachine;

namespace m6 {

class Compiler;

/**
 * @brief  High‑level facade for the full script processing pipeline.
 *
 * This class encapsulates the stages of:
 *   1. Tokenization
 *   2. Parsing
 *   3. (Optional) Compilation
 *   4. (Optional) Execution
 *
 * You can construct it with progressively more capabilities:
 *  - No args: tokenize & parse only
 *  - With a Compiler: also compile
 *  - With a Compiler + RLMachine: compile & execute
 */
class ScriptEngine {
 public:
  /**
   * @brief Create a ScriptEngine with only tokenization & parsing enabled.
   */
  ScriptEngine();

  /**
   * @brief Create a ScriptEngine with compilation enabled.
   *
   * If `compiler` is nullptr, a default Compiler will be created.
   *
   * @param compiler  Shared pointer to a Compiler instance (or nullptr).
   */
  explicit ScriptEngine(std::shared_ptr<Compiler> compiler);

  /**
   * @brief Create a ScriptEngine with both compilation and execution enabled.
   *
   * If `compiler` or `machine` is nullptr, defaults will be constructed.
   *
   * @param compiler  Shared pointer to a Compiler instance (or nullptr).
   * @param machine   Shared pointer to an RLMachine instance (or nullptr).
   */
  explicit ScriptEngine(std::shared_ptr<Compiler> compiler,
                        std::shared_ptr<RLMachine> machine);

  /**
   * @brief Holds the result of running a script through the pipeline.
   */
  struct ExecutionResult {
    std::vector<Token> tokens;  ///< Tokens produced by the tokenizer.
    std::vector<std::shared_ptr<AST>> asts;  ///< ASTs produced by the parser.
    std::vector<Instruction>
        instructions;  ///< Bytecode/instructions (empty if no compiler).
    std::vector<Value>
        intermediateValues;  ///< Values from evaluated expression statements.
    std::span<CompileError>
        errors;  ///< Compilation or runtime errors (if any).
  };

  /**
   * @brief Process an input script: tokenize, parse, compile, and run.
   *
   * Ownership of the input string is transferred; on error, partial results are
   * discarded.
   *
   * @param input  The source code to execute.
   * @return       An ExecutionResult containing tokens, ASTs, instructions,
   *               intermediate values, and any errors.
   */
  ExecutionResult Execute(std::string input);

  /**
   * @brief Retrieve and format any accumulated errors as a human‑readable
   * string.
   *
   * This also resets the internal error log, so subsequent calls will not
   * include already‑flushed errors.
   *
   * @return  A formatted error report, or an empty string if no errors.
   */
  std::string FlushErrors();

 private:
  std::string src_;   ///< Raw source buffer.
  size_t valid_len_;  ///< Length up to which `src_` was successfully processed.
  std::vector<CompileError>
      errors_;  ///< Collected errors during compile/execute.
  std::shared_ptr<Compiler>
      compiler_;  ///< Optional compiler (nullptr = disabled).
  std::shared_ptr<RLMachine>
      machine_;  ///< Optional runtime (nullptr = disabled).
};

}  // namespace m6
