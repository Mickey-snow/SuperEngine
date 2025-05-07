// -----------------------------------------------------------------------
//
// This file is part of libreallive, a dependency of RLVM.
//
// -----------------------------------------------------------------------
//
// Copyright (c) 2025 Serina Sakurai
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

#include "libreallive/expression_visitor.hpp"

#include "libreallive/expression.hpp"

#include <sstream>
#include <string>

namespace libreallive {

// visit helper
template <typename Visitor>
inline static decltype(auto) visit(const Expression& expr, Visitor&& v) {
  return std::visit(std::forward<Visitor>(v), expr->AsVariant());
}

std::string DebugVisitor::operator()(StoreRegisterEx* e) { return "<store>"; }
std::string DebugVisitor::operator()(IntConstantEx* e) {
  return std::to_string(e->value_);
}
std::string DebugVisitor::operator()(StringConstantEx* e) {
  return '"' + e->value_ + '"';
}
std::string DebugVisitor::operator()(MemoryReferenceEx* e) {
  return std::format("{}[{}]", GetBankName(e->type_),
                     visit(e->location_, *this));
}
std::string DebugVisitor::operator()(SimpleMemRefEx* e) {
  return std::format("{}[{}]", GetBankName(e->type_), e->location_);
}
std::string DebugVisitor::operator()(SimpleAssignEx* e) {
  return std::format("{}[{}] = {}", GetBankName(e->type_), e->location_,
                     e->value_);
}
std::string DebugVisitor::operator()(UnaryEx* e) {
  return (e->operation_ == 0x01 ? "-" : "") + visit(e->operand_, *this);
}
std::string DebugVisitor::operator()(BinaryExpressionEx* e) {
  return std::format("{} {} {}", visit(e->left_, *this),
                     ToString(static_cast<Op>(e->operation_)),
                     visit(e->right_, *this));
}
std::string DebugVisitor::operator()(ComplexEx* e) {
  std::string s = "(";
  for (size_t i = 0; i < e->expression_.size(); ++i) {
    s += visit(e->expression_[i], *this);
    if (i + 1 < e->expression_.size())
      s += ", ";
  }
  return s + ")";
}
std::string DebugVisitor::operator()(SpecialEx* e) {
  std::ostringstream oss;
  oss << static_cast<int>(e->overload_tag_) << ":{";
  for (size_t i = 0; i < e->expression_.size(); ++i) {
    oss << visit(e->expression_[i], *this);
    if (i + 1 < e->expression_.size())
      oss << ", ";
  }
  oss << "}";
  return oss.str();
}

}  // namespace libreallive
