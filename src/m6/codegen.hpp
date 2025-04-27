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

#include "m6/ast.hpp"
#include "m6/exception.hpp"
#include "vm/instruction.hpp"
#include "vm/value.hpp"

#include <concepts>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace m6 {

class CodeGenerator {
  using ChunkPtr = std::shared_ptr<serilang::Chunk>;
  using Scope = std::unordered_map<std::string, std::size_t>;  // name->slot

 public:
  static std::shared_ptr<serilang::Chunk> Gen(std::shared_ptr<AST> ast) {
    CodeGenerator gen;
    gen.emit_stmt(ast);
    gen.emit(serilang::Return{});
    return gen.chunk_;
  }

  CodeGenerator()
      : chunk_(std::make_shared<serilang::Chunk>()),
        locals_{},
        local_depth_(0),
        patch_sites_() {}
  ~CodeGenerator() = default;

 private:  // data members
  std::shared_ptr<serilang::Chunk> chunk_;
  std::vector<Scope> locals_;  // one per lexical block
  std::size_t local_depth_;    // current stack depth
  std::unordered_map<std::string, int32_t> patch_sites_;  // for break/continue

 private:  // constant-pool helpers
  uint32_t constant(Value v) {
    chunk_->const_pool.emplace_back(std::move(v));
    return static_cast<uint32_t>(chunk_->const_pool.size() - 1);
  }

  uint32_t intern_name(const std::string& s) {
    // keep identical strings unified – optional optimisation
    for (uint32_t i = 0; i < chunk_->const_pool.size(); ++i)
      if (auto p = chunk_->const_pool[i].Get_if<std::string>(); p && *p == s)
        return i;
    return constant(Value{std::string{s}});
  }

 private:  // emit helpers
  template <class T>
    requires std::constructible_from<serilang::Instruction, T>
  void emit(T&& ins) {
    chunk_->code.emplace_back(std::forward<T>(ins));
  }
  std::size_t code_size() const { return chunk_->code.size(); }

  void push_scope() { locals_.emplace_back(); }
  void pop_scope() { locals_.pop_back(); }

 private:  // identifier resolution
  std::optional<std::size_t> resolve_local(const std::string& name) const {
    for (std::size_t i = locals_.size(); i-- > 0;) {
      auto it = locals_[i].find(name);
      if (it != locals_[i].end())
        return it->second;
    }
    return std::nullopt;
  }
  std::size_t add_local(const std::string& name) {
    locals_.back()[name] = local_depth_;
    return local_depth_++;
  }

 private:  // expression codegen
  void emit_expr(std::shared_ptr<ExprAST> n) {
    n->Apply([&](auto&& x) { emit_expr_node(x); });
  }
  void emit_expr_node(const IntLiteral& n) {
    const auto slot = constant(Value(n.value));
    emit(serilang::Push{slot});
  }
  void emit_expr_node(const StrLiteral& n) {
    const auto slot = constant(Value(std::string(n.value)));
    emit(serilang::Push{slot});
  }
  void emit_expr_node(const NilLiteral&) {
    emit(serilang::Push{constant(Value())});
  }
  void emit_expr_node(const Identifier& n) {
    std::string id = std::string(n.value);
    if (auto slot = resolve_local(id); slot.has_value())
      emit(serilang::LoadLocal{static_cast<uint8_t>(*slot)});
    else
      emit(serilang::LoadGlobal{intern_name(id)});
  }
  void emit_expr_node(const UnaryExpr& u) {
    emit_expr(u.sub);
    emit(serilang::UnaryOp{u.op});
  }
  void emit_expr_node(const BinaryExpr& b) {
    emit_expr(b.lhs);
    emit_expr(b.rhs);
    emit(serilang::BinaryOp{b.op});
  }
  void emit_expr_node(const ParenExpr& p) { emit_expr(p.sub); }
  void emit_expr_node(const InvokeExpr& call) {
    emit_expr(call.fn);
    for (const auto& arg : call.args)
      emit_expr(arg);
    emit(serilang::Call{static_cast<uint8_t>(call.args.size())});
  }
  void emit_expr_node(const SubscriptExpr& s) {  // experimental
    emit_expr(s.primary);
    emit_expr(s.index);
    emit(serilang::GetItem{});
  }
  void emit_expr_node(const MemberExpr& m) {  // experimental
    emit_expr(m.primary);
    auto id = m.member->Get_if<Identifier>();
    if (!id)
      throw SyntaxError("member must be identifier", m.mem_loc);
  }

 private:  // statement code-gen
  void emit_stmt(std::shared_ptr<AST> s) {
    s->Apply([&](auto&& n) { emit_stmt_node(n); });
  }

  void emit_stmt_node(const AssignStmt& s) {
    emit_expr(s.rhs);
    std::string id = std::string(s.lhs->Get_if<Identifier>()->value);

    if (auto slot = resolve_local(id); slot.has_value())
      emit(serilang::StoreLocal{static_cast<uint8_t>(slot.value())});
    else
      emit(serilang::StoreGlobal{intern_name(id)});
  }
  void emit_stmt_node(const AugStmt& s) {
    std::string id = std::string(s.lhs->Get_if<Identifier>()->value);

    auto slot = resolve_local(id);
    if (slot.has_value())
      emit(serilang::LoadLocal{static_cast<uint8_t>(slot.value())});
    else
      emit(serilang::LoadGlobal{intern_name(id)});

    emit_expr(s.rhs);
    emit(serilang::BinaryOp{s.op});

    // store back
    if (slot.has_value())
      emit(serilang::StoreLocal{static_cast<uint8_t>(slot.value())});
    else
      emit(serilang::StoreGlobal{intern_name(id)});
  }
  void emit_stmt_node(const IfStmt& s) {
    emit_expr(s.cond);

    emit(serilang::JumpIfFalse{0});
    auto jfalse = code_size() - 1;
    emit(serilang::Pop{});

    emit_stmt(s.then);
    if (s.els) {
      emit(serilang::Jump{0});
      auto jend = code_size() - 1;

      patch(jfalse, code_size());
      // emit(Instr{ Pop{} });
      emit_stmt(s.els);
      patch(jend, code_size());
    } else {
      patch(jfalse, code_size());
      // emit(Instr{ Pop{} });
    }
  }
  void emit_stmt_node(const WhileStmt& s) {
    auto loop_top = code_size();

    emit_expr(s.cond);
    emit(serilang::JumpIfFalse{0});
    auto exitj = code_size() - 1;
    emit(serilang::Pop{});

    emit_stmt(s.body);
    emit(serilang::Jump{rel<int32_t>(code_size(), loop_top) - 1});
    patch(exitj, code_size());
    // emit(Pop{});
  }
  void emit_stmt_node(const ForStmt& f) {
    push_scope();
    if (f.init)
      emit_stmt(f.init);

    auto condpos = code_size();
    if (f.cond)
      emit_expr(f.cond);
    else
      emit(serilang::Push{constant(Value(true))});
    emit(serilang::JumpIfFalse{0});
    auto exitj = code_size();
    emit(serilang::Pop{});

    emit_stmt(f.body);
    if (f.inc)
      emit_stmt(f.inc);

    emit(serilang::Jump{rel<int32_t>(code_size(), condpos) - 1});

    patch(exitj, code_size());
    pop_scope();
  }
  void emit_stmt_node(const BlockStmt& s) {
    push_scope();
    for (const auto& stmt : s.body)
      emit_stmt(stmt);
    pop_scope();
  }
  void emit_stmt_node(const FuncDecl& fn) {  // experimental
    // compile body with a fresh compiler
    CodeGenerator nested;
    nested.push_scope();
    // parameters become locals 0…N-1
    for (auto& p : fn.params)
      nested.add_local(p);
    nested.emit_stmt(fn.body);
    nested.emit(serilang::Return{});
    auto fnChunk = nested.chunk_;
    // push as constant
    uint32_t constSlot =
        constant(Value{std::make_shared<serilang::Closure>(fnChunk)});
    emit(serilang::Push{constSlot});
    emit(serilang::MakeClosure{0, static_cast<uint32_t>(fn.params.size()),
                               static_cast<uint32_t>(nested.local_depth_),
                               0 /*nupvals*/});
    // store to global (function hoisting)
    emit(serilang::StoreGlobal{intern_name(fn.name)});
  }
  void emit_stmt_node(const ClassDecl& cd) {  // experimental
    // compile each method as closures, left on stack
    for (auto& m : cd.members)
      emit_stmt_node(m);
    emit(serilang::MakeClass{intern_name(cd.name),
                             static_cast<uint16_t>(cd.members.size())});
    emit(serilang::StoreGlobal{intern_name(cd.name)});
  }
  void emit_stmt_node(const std::shared_ptr<ExprAST>& s) {
    emit_expr(s);
    emit(serilang::Pop{});
  }

 private:  // helper functions
  template <typename offset_t>
  static inline auto rel(offset_t from, offset_t to) -> offset_t {
    return to - from;
  }
  void patch(std::size_t site, std::size_t target) {
    auto& var = chunk_->code[site];
    auto offset = rel<int32_t>(site + 1, target);
    std::visit(
        [&](auto& ins) {
          using T = std::decay_t<decltype(ins)>;
          if constexpr (std::is_same_v<T, serilang::JumpIfFalse>)
            ins.offset = offset;
          else if constexpr (std::is_same_v<T, serilang::JumpIfTrue>)
            ins.offset = offset;
          else if constexpr (std::is_same_v<T, serilang::Jump>)
            ins.offset = offset;
          else
            throw std::runtime_error("Codegen: invalid patch site");
        },
        var);
  }
};

}  // namespace m6
