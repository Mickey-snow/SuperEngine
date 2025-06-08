# m6 Directory Overview

This folder implements the serilang scripting language frontend.

## File Guide

### `ast.hpp` / `ast.cpp`
Defines the complete abstract syntax tree used by the compiler.  Expression and
statement nodes are stored in `std::variant` based classes `ExprAST` and `AST`.
The `.cpp` file implements `DebugString` helpers and AST dumping logic.

### `codegen.hpp` / `codegen.cpp`
`CodeGenerator` transforms AST nodes into VM bytecode.

### `compiler_pipeline.hpp`
High level helper that wires `Tokenizer`, `Parser` and `CodeGenerator` into a
single pipeline.

### `error_formatter.hpp` / `error_formatter.cpp`
Renders diagnostic messages with line and column information.

### `line_table.hpp` / `line_table.cpp`
`LineTable` maps byte offsets to line/column pairs for a source string.  Used by
`SourceBuffer` and diagnostics.

### `parser.hpp` / `parser.cpp`
Recursive‑descent parser producing `AST` nodes from the token stream.

### `source_buffer.hpp` / `source_buffer.cpp`
`SourceBuffer` owns the original source text and produces `SourceLocation`
references.

### `source_location.hpp` / `source_location.cpp`
Lightweight struct representing a location in a `SourceBuffer`.

### `token.hpp` / `token.cpp`
Token types produced by the lexer.  `Token` wraps a `std::variant` of token
kinds with helpers.

### `tokenizer.hpp` / `tokenizer.cpp`
Converts a `SourceBuffer` into a vector of tokens.  Handles whitespace,
identifiers, literals, operators and reports lexical errors.

## Testing
Tests for this directory live under `test/m6`.
