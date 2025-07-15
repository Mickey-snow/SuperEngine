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

#include "utilities/mpl.hpp"
#include "vm/object.hpp"
#include "vm/value.hpp"

#include <stdexcept>
#include <utility>

namespace m6 {

namespace sr = serilang;

// Constructor / Destructor
CodeGenerator::CodeGenerator(sr::GarbageCollector& gc, bool repl)
    : gc_(gc),
      repl_mode_(repl),
      chunk_(gc.Allocate<sr::Code>()),
      locals_(),
      local_depth_(0),
      errors_() {}

CodeGenerator::~CodeGenerator() = default;

// Public status API
bool CodeGenerator::Ok() const { return errors_.empty(); }

std::span<const Error> CodeGenerator::GetErrors() const { return errors_; }

void CodeGenerator::ClearErrors() { errors_.clear(); }

// Code accessors
sr::Code* CodeGenerator::GetChunk() const { return chunk_; }
void CodeGenerator::SetChunk(sr::Code* c) { chunk_ = c; }

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

uint32_t CodeGenerator::intern_name(std::string_view s) {
  for (uint32_t i = 0; i < chunk_->const_pool.size(); ++i) {
    if (auto p = chunk_->const_pool[i].Get_if<std::string>(); p && *p == s) {
      return i;
    }
  }
  return constant(Value{std::string{s}});
}

void CodeGenerator::emit_const(Value v) {
  const auto slot = constant(std::move(v));
  emit(sr::Push{slot});
}

void CodeGenerator::emit_const(std::string_view s) {
  const auto slot = intern_name(s);
  emit(sr::Push{slot});
}

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

CodeGenerator::SCOPE CodeGenerator::get_scope(const std::string& name) {
  auto it = scope_heuristic_.find(name);
  if (it == scope_heuristic_.cend())
    return SCOPE::NONE;
  else
    return it->second;
}

// Expression codegen
void CodeGenerator::emit_expr(std::shared_ptr<ExprAST> n) {
  n->Apply([&](auto&& x) { emit_expr_node(x); });
}

void CodeGenerator::emit_expr_node(const NilLiteral& m) {
  emit_const(std::monostate());
}

void CodeGenerator::emit_expr_node(const IntLiteral& n) { emit_const(n.value); }

void CodeGenerator::emit_expr_node(const StrLiteral& n) {
  emit_const(std::string(n.value));
}

void CodeGenerator::emit_expr_node(const ListLiteral& n) {
  for (const auto& it : n.elements)
    emit_expr(it);
  emit(sr::MakeList{static_cast<uint32_t>(n.elements.size())});
}

void CodeGenerator::emit_expr_node(const DictLiteral& n) {
  for (const auto& [key, val] : n.elements) {
    emit_expr(key);
    emit_expr(val);
  }
  emit(sr::MakeDict{static_cast<uint32_t>(n.elements.size())});
}

void CodeGenerator::emit_expr_node(const Identifier& n) {
  std::string id{n.value};
  if (get_scope(id) == SCOPE::GLOBAL) {
    emit(sr::LoadGlobal{intern_name(id)});
    return;
  }

  if (auto slot = resolve_local(id)) {
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
  for (auto& arg : call.args)
    emit_expr(arg);
  for (auto& [k, arg] : call.kwargs) {
    emit_const(std::string(k));
    emit_expr(arg);
  }
  emit(sr::Call{static_cast<uint32_t>(call.args.size()),
                static_cast<uint32_t>(call.kwargs.size())});
}

void CodeGenerator::emit_expr_node(const SubscriptExpr& s) {
  emit_expr(s.primary);
  emit_expr(s.index);
  emit(sr::GetItem{});
}

void CodeGenerator::emit_expr_node(const MemberExpr& m) {
  emit_expr(m.primary);
  emit(sr::GetField{intern_name(m.member)});
}

void CodeGenerator::emit_expr_node(const SpawnExpr& s) {
  InvokeExpr* invoke = s.invoke->Get_if<InvokeExpr>();
  emit_expr(invoke->fn);  // (fn)
  for (auto& arg : invoke->args)
    emit_expr(arg);
  // (fn, arg...)
  for (auto& [k, arg] : invoke->kwargs) {
    emit_const(std::string(k));
    emit_expr(arg);
  }
  // (fn, arg..., kwarg...)

  emit(sr::MakeFiber{.argcnt = static_cast<uint32_t>(invoke->args.size()),
                     .kwargcnt = static_cast<uint32_t>(invoke->kwargs.size())});
  // -> (fiber)
}

void CodeGenerator::emit_expr_node(const AwaitExpr& s) {
  emit_expr(s.corout);
  // (corout)
  // TODO: support passing argument

  emit(sr::Await{});
}

// Statement codegen
void CodeGenerator::emit_stmt(std::shared_ptr<AST> s) {
  s->Apply([&](auto&& n) { emit_stmt_node(n); });
}

void CodeGenerator::emit_stmt_node(const ScopeStmt& s) {
  for (const auto& it : s.vars)
    scope_heuristic_[it] = SCOPE::GLOBAL;
}

void CodeGenerator::emit_stmt_node(const AssignStmt& s) {
  if (auto id = s.lhs->Get_if<Identifier>()) {
    // simple variable
    emit_expr(s.rhs);
    const std::string name{id->value};

    if (locals_.empty() || get_scope(name) == SCOPE::GLOBAL) {
      emit(sr::StoreGlobal{intern_name(name)});
      return;
    }

    if (auto slot = resolve_local(name); slot.has_value()) {
      emit(sr::StoreLocal{static_cast<uint8_t>(*slot)});
    } else {
      slot = add_local(name);
    }
  } else if (auto mem = s.lhs->Get_if<MemberExpr>()) {
    // object field
    emit_expr(mem->primary);                       // (obj)
    emit_expr(s.rhs);                              // (obj,val)
    emit(sr::SetField{intern_name(mem->member)});  // ->
  } else if (auto sub = s.lhs->Get_if<SubscriptExpr>()) {
    // subscription
    emit_expr(sub->primary);  // (cont)
    emit_expr(sub->index);    // (cont,idx)
    emit_expr(s.rhs);         // (cont,idx,val)
    emit(sr::SetItem{});      // ->
  }
}

void CodeGenerator::emit_stmt_node(const AugStmt& s) {
  if (auto id = s.lhs->Get_if<Identifier>()) {
    // simple variable
    std::string name{id->value};
    auto slot = resolve_local(name);

    if (slot.has_value())
      emit(sr::LoadLocal{static_cast<uint8_t>(*slot)});
    else
      emit(sr::LoadGlobal{intern_name(name)});

    emit_expr(s.rhs);
    emit(sr::BinaryOp{s.GetRmAssignmentOp()});

    if (slot.has_value())
      emit(sr::StoreLocal{static_cast<uint8_t>(*slot)});
    else
      emit(sr::StoreGlobal{intern_name(name)});

  } else if (auto mem = s.lhs->Get_if<MemberExpr>()) {
    // object field
    emit_expr(mem->primary);                       // (obj)
    emit(sr::Dup{});                               // (obj,obj)
    emit(sr::GetField{intern_name(mem->member)});  // (obj,val)
    emit_expr(s.rhs);                              // (obj,val,rhs)
    emit(sr::BinaryOp{s.GetRmAssignmentOp()});     // (obj,result)
    emit(sr::SetField{intern_name(mem->member)});  // ->
  } else if (auto sub = s.lhs->Get_if<SubscriptExpr>()) {
    // subscription
    emit_expr(sub->primary);                    // (cont)
    emit_expr(sub->index);                      // (cont,idx)
    emit(sr::Dup{.top_ofs = 1});                // (cont,idx,cont)
    emit(sr::Dup{.top_ofs = 1});                // (cont,idx,cont,idx)
    emit(sr::GetItem{});                        // (cont,idx,val)
    emit_expr(s.rhs);                           // (cont,idx,val,rhs)
    emit(sr::BinaryOp{s.GetRmAssignmentOp()});  // (cont,idx,result)
    emit(sr::SetItem{});                        // ->
  }
}

void CodeGenerator::emit_stmt_node(const IfStmt& s) {
  emit_expr(s.cond);
  auto jfalse = code_size();
  emit(sr::JumpIfFalse{0});

  emit_stmt(s.then);
  if (s.els) {
    auto jend = code_size();
    emit(sr::Jump{0});

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

  auto jmp = code_size();
  emit(sr::Jump{0});
  patch(jmp, loop_top);
  patch(exitj, code_size());
}

void CodeGenerator::emit_stmt_node(const ForStmt& f) {
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

  auto jmp = code_size();
  emit(sr::Jump{0});
  patch(jmp, condpos);

  patch(exitj, code_size());
}

void CodeGenerator::emit_stmt_node(const BlockStmt& s) {
  for (auto& stmt : s.body)
    emit_stmt(stmt);
}

void CodeGenerator::emit_function(const FuncDecl& fn) {
  // compile body with a fresh compiler
  CodeGenerator nested(gc_, repl_mode_);
  nested.SetChunk(gc_.Allocate<serilang::Code>());
  nested.scope_heuristic_ = scope_heuristic_;
  nested.push_scope();
  nested.add_local(fn.name);
  for (auto& p : fn.params)
    nested.add_local(p);
  for (auto& p : fn.default_params)
    nested.add_local(p.first);
  if (!fn.var_arg.empty())
    nested.add_local(fn.var_arg);
  if (!fn.kw_arg.empty())
    nested.add_local(fn.kw_arg);

  nested.emit_stmt(fn.body);
  nested.emit(sr::Return{});

  emit_const(nested.GetChunk());
  for (auto& p : fn.default_params) {
    emit_const(p.first);
    emit_expr(p.second);
  }
  for (auto& p : fn.params)
    emit_const(p);
  for (auto& p : fn.default_params)
    emit_const(p.first);

  emit(sr::MakeFunction{.entry = 0,
                        .nparam = fn.params.size() + fn.default_params.size(),
                        .ndefault = fn.default_params.size(),
                        .has_vararg = !fn.var_arg.empty(),
                        .has_kwarg = !fn.kw_arg.empty()});
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

void CodeGenerator::emit_stmt_node(const YieldStmt& y) {
  if (y.value)
    emit_expr(y.value);
  else
    emit(sr::Push{constant(Value(std::monostate()))});
  emit(sr::Yield{});
}

void CodeGenerator::emit_stmt_node(const std::shared_ptr<ExprAST>& s) {
  if (repl_mode_) {
    emit(sr::LoadGlobal{intern_name("print")});
    emit_expr(s);
    emit(sr::Call{1, 0});
  } else {
    emit_expr(s);
    emit(sr::Pop{1});
  }
}

namespace {
// -- Offset helper --------------------------------------------------
template <typename offset_t>
static inline constexpr auto rel(offset_t from, offset_t to) -> offset_t {
  return to - from;
}
}  // namespace

void CodeGenerator::patch(std::size_t site,
                          std::size_t target) {  // Patch jumps
  const auto offset =
      rel<int32_t>(site + sizeof(std::byte) + sizeof(int32_t), target);

  switch (static_cast<sr::OpCode>((*chunk_)[site])) {
    case sr::OpCode::Jump:
    case sr::OpCode::JumpIfFalse:
    case sr::OpCode::JumpIfTrue:
      chunk_->Write(site + 1, offset);
      break;
    default:
      throw std::runtime_error(
          "Codegen: invalid patch site (type" +
          std::to_string(static_cast<uint8_t>((*chunk_)[site])) + ')');
  }
}

}  // namespace m6
