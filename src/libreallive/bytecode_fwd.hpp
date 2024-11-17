// This file is part of libreallive, a dependency of RLVM.
//
// -----------------------------------------------------------------------
//
// Copyright (c) 2006 Peter Jolly
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

#include <forward_list>
#include <memory>
#include <variant>

namespace libreallive {

// List definitions.
class ExpressionPiece;
class BytecodeElement;
typedef std::forward_list<std::unique_ptr<BytecodeElement>> BytecodeList;
typedef BytecodeList::iterator pointer_t;

struct ConstructionData;
class Pointers;

class DataElement;
class MetaElement;
class TextoutElement;
class ExpressionElement;
class CommaElement;
class CommandElement;
class SelectElement;
class FunctionElement;
class PointerElement;
class GotoElement;
class GotoCaseElement;
class GotoOnElement;
class GosubWithElement;

using Bytecode_ptr = std::variant<CommandElement const*,
                                  CommaElement const*,
                                  MetaElement const*,
                                  TextoutElement const*,
                                  ExpressionElement const*>;
}  // namespace libreallive
