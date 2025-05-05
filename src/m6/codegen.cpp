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

#include "m6/codegen.hpp"

#include "vm/value_internal/closure.hpp"

#include <stdexcept>
#include <utility>

namespace m6 {

namespace sr = serilang;

// Constructor / Destructor
CodeGenerator::CodeGenerator(bool repl)
    : repl_mode_(repl),
      chunk_(std::make_shared<sr::Chunk>()),
      locals_(),
      local_depth_(0),
      patch_sites_(),
      errors_() {}

CodeGenerator::~CodeGenerator() = default;

// Public status API
bool CodeGenerator::Ok() const { return errors_.empty(); }

std::span<const Error> CodeGenerator::GetErrors() const { return errors_; }

void CodeGenerator::ClearErrors() { errors_.clear(); }

// Chunk accessors
std::shared_ptr<sr::Chunk> CodeGenerator::GetChunk() const { return chunk_; }

void CodeGenerator::SetChunk(std::shared_ptr<sr::Chunk> c) {
  chunk_ = std::move(c);
}

// Entry point
void CodeGenerator::Gen(std::shared_ptr<AST> ast) { emit_stmt(std::move(ast)); }

// Error recording
void CodeGenerator::AddError(std::string msg,
                             std::optional<SourceLocation> loc) {
  errors_.emplace_back(std::move(msg), std::move(loc));
}

// Constant-pool helpers
uint32_t CodeGenerator::constant(Value v) {
  chunk_->const_pool.emplace_back(std::move(v));
  return static_cast<uint32_t>(chunk_->const_pool.size() - 1);
}

uint32_t CodeGenerator::intern_name(const std::string& s) {
  for (uint32_t i = 0; i < chunk_->const_pool.size(); ++i) {
    if (auto p = chunk_->const_pool[i].Get_if<std::string>(); p && *p == s) {
      return i;
    }
  }
  return constant(Value{std::string{s}});
}

// Emit helpers
std::size_t CodeGenerator::code_size() const { return chunk_->code.size(); }

void CodeGenerator::push_scope() { locals_.emplace_back(); }

void CodeGenerator::pop_scope() { locals_.pop_back(); }

// Identifier resolution
std::optional<std::size_t> CodeGenerator::resolve_local(
    const std::string& name) const {
  for (std::size_t i = locals_.size(); i-- > 0;) {
    auto it = locals_[i].find(name);
    if (it != locals_[i].end())
      return it->second;
  }
  return std::nullopt;
}

std::size_t CodeGenerator::add_local(const std::string& name) {
  locals_.back()[name] = local_depth_;
  return local_depth_++;
}

// Expression codegen
void CodeGenerator::emit_expr(std::shared_ptr<ExprAST> n) {
  n->Apply([&](auto&& x) { emit_expr_node(x); });
}

void CodeGenerator::emit_expr_node(const IntLiteral& n) {
  auto slot = constant(Value(n.value));
  emit(sr::Push{slot});
}

void CodeGenerator::emit_expr_node(const StrLiteral& n) {
  auto slot = constant(Value(std::string(n.value)));
  emit(sr::Push{slot});
}

void CodeGenerator::emit_expr_node(const NilLiteral&) {
  emit(sr::Push{constant(Value())});
}

void CodeGenerator::emit_expr_node(const Identifier& n) {
  std::string id{n.value};
  if (auto slot = resolve_local(id); slot.has_value()) {
    emit(sr::LoadLocal{static_cast<uint8_t>(*slot)});
  } else {
    emit(sr::LoadGlobal{intern_name(id)});
  }
}

void CodeGenerator::emit_expr_node(const UnaryExpr& u) {
  emit_expr(u.sub);
  emit(sr::UnaryOp{u.op});
}

void CodeGenerator::emit_expr_node(const BinaryExpr& b) {
  emit_expr(b.lhs);
  emit_expr(b.rhs);
  emit(sr::BinaryOp{b.op});
}

void CodeGenerator::emit_expr_node(const ParenExpr& p) { emit_expr(p.sub); }

void CodeGenerator::emit_expr_node(const InvokeExpr& call) {
  emit_expr(call.fn);
  for (auto& arg : call.args) {
    emit_expr(arg);
  }
  emit(sr::Call{static_cast<uint8_t>(call.args.size())});
}

void CodeGenerator::emit_expr_node(const SubscriptExpr& s) {
  emit_expr(s.primary);
  emit_expr(s.index);
  emit(sr::GetItem{});
}

void CodeGenerator::emit_expr_node(const MemberExpr& m) {
  emit_expr(m.primary);
  auto id = m.member->Get_if<Identifier>();
  if (!id) {
    AddError("member must be identifier", m.mem_loc);
  }
}

// Statement codegen
void CodeGenerator::emit_stmt(std::shared_ptr<AST> s) {
  s->Apply([&](auto&& n) { emit_stmt_node(n); });
}

void CodeGenerator::emit_stmt_node(const AssignStmt& s) {
  emit_expr(s.rhs);
  std::string id{s.lhs->Get_if<Identifier>()->value};
  if (auto slot = resolve_local(id); slot.has_value()) {
    emit(sr::StoreLocal{static_cast<uint8_t>(*slot)});
  } else {
    emit(sr::StoreGlobal{intern_name(id)});
  }
}

void CodeGenerator::emit_stmt_node(const AugStmt& s) {
  std::string id{s.lhs->Get_if<Identifier>()->value};
  auto slot = resolve_local(id);
  if (slot.has_value())
    emit(sr::LoadLocal{static_cast<uint8_t>(*slot)});
  else
    emit(sr::LoadGlobal{intern_name(id)});

  emit_expr(s.rhs);
  emit(sr::BinaryOp{s.GetRmAssignmentOp()});

  if (slot.has_value())
    emit(sr::StoreLocal{static_cast<uint8_t>(*slot)});
  else
    emit(sr::StoreGlobal{intern_name(id)});
}

void CodeGenerator::emit_stmt_node(const IfStmt& s) {
  emit_expr(s.cond);
  emit(sr::JumpIfFalse{0});
  auto jfalse = code_size() - 1;

  emit_stmt(s.then);
  if (s.els) {
    emit(sr::Jump{0});
    auto jend = code_size() - 1;
    patch(jfalse, code_size());
    emit_stmt(s.els);
    patch(jend, code_size());
  } else {
    patch(jfalse, code_size());
  }
}

void CodeGenerator::emit_stmt_node(const WhileStmt& s) {
  auto loop_top = code_size();
  emit_expr(s.cond);
  auto exitj = code_size();
  emit(sr::JumpIfFalse{0});
  emit_stmt(s.body);
  emit(sr::Jump{rel<int32_t>(code_size(), loop_top) - 1});
  patch(exitj, code_size());
}

void CodeGenerator::emit_stmt_node(const ForStmt& f) {
  push_scope();
  if (f.init)
    emit_stmt(f.init);

  auto condpos = code_size();
  if (f.cond)
    emit_expr(f.cond);
  else
    emit(sr::Push{constant(Value(true))});

  auto exitj = code_size();
  emit(sr::JumpIfFalse{0});
  emit_stmt(f.body);
  if (f.inc)
    emit_stmt(f.inc);

  emit(sr::Jump{rel<int32_t>(code_size(), condpos) - 1});
  patch(exitj, code_size());
  pop_scope();
}

void CodeGenerator::emit_stmt_node(const BlockStmt& s) {
  push_scope();
  for (auto& stmt : s.body)
    emit_stmt(stmt);
  pop_scope();
}

void CodeGenerator::emit_function(const FuncDecl& fn) {
  auto jFnEnd = code_size();
  emit(sr::Jump{0});
  auto entryIp = code_size();

  // compile body with a fresh compiler
  CodeGenerator nested;
  nested.SetChunk(chunk_);
  nested.push_scope();
  nested.add_local(fn.name);
  for (auto& p : fn.params)
    nested.add_local(p);
  nested.emit_stmt(fn.body);
  nested.emit(sr::Return{});
  patch(jFnEnd, code_size());

  emit(sr::MakeClosure{
      .entry = static_cast<uint32_t>(entryIp),
      .nparams = static_cast<uint32_t>(fn.params.size()),
      .nlocals = static_cast<uint32_t>(nested.locals_.front().size()),
      .nupvals = 0});
}

void CodeGenerator::emit_stmt_node(const FuncDecl& fn) {
  emit_function(fn);
  emit(sr::StoreGlobal{intern_name(fn.name)});
}

void CodeGenerator::emit_stmt_node(const ClassDecl& cd) {
  for (auto& m : cd.members) {
    emit(sr::Push{constant(Value(m.name))});
    emit_function(m);
  }
  emit(sr::MakeClass{intern_name(cd.name),
                     static_cast<uint16_t>(cd.members.size())});
  emit(sr::StoreGlobal{intern_name(cd.name)});
}

void CodeGenerator::emit_stmt_node(const ReturnStmt& r) {
  if (r.value)
    emit_expr(r.value);
  else
    emit(sr::Push{constant(Value(std::monostate()))});
  emit(sr::Return{});
}

void CodeGenerator::emit_stmt_node(const std::shared_ptr<ExprAST>& s) {
  if (repl_mode_) {
    emit(sr::LoadGlobal{intern_name("print")});
    emit_expr(s);
    emit(sr::Call{1});
  } else {
    emit_expr(s);
    emit(sr::Pop{1});
  }
}

// Patch jumps
void CodeGenerator::patch(std::size_t site, std::size_t target) {
  auto& var = chunk_->code[site];
  auto offset = rel<int32_t>(site + 1, target);
  std::visit(
      [&](auto& ins) {
        using T = std::decay_t<decltype(ins)>;
        if constexpr (std::is_same_v<T, sr::JumpIfFalse>)
          ins.offset = offset;
        else if constexpr (std::is_same_v<T, sr::JumpIfTrue>)
          ins.offset = offset;
        else if constexpr (std::is_same_v<T, sr::Jump>)
          ins.offset = offset;
        else
          throw std::runtime_error("Codegen: invalid patch site");
      },
      var);
}

}  // namespace m6
