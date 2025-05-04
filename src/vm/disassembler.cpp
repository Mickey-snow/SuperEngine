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

#include "vm/disassembler.hpp"

#include "vm/instruction.hpp"
#include "vm/value_internal/closure.hpp"

#include <iomanip>

namespace serilang {

Disassembler::Disassembler(size_t indent) : indent_size_(indent) {}

void Disassembler::PrintIns(Chunk& chunk,
                            size_t ip,
                            const std::string& indent) {
  using std::left;
  using std::right;
  using std::setw;

  // helpers -------------------------------------------------------
  const auto emit_addr = [&](size_t addr) {
    static constexpr int COL_ADDR = 4;
    out_ << indent << right << setw(COL_ADDR) << addr << "  ";
  };
  const auto emit_mnemonic = [&](std::string_view m) {
    static constexpr int COL_MNEMON = 12;
    out_ << left << setw(COL_MNEMON) << m;
  };
  const auto emit_operand = [&](auto v) {
    static constexpr int COL_OPERND = 10;
    out_ << right << setw(COL_OPERND) << v;
  };

  // header (address column) --------------------------------------
  emit_addr(ip);

  // body ----------------------------------------------------------
  std::visit(
      [&](const auto& ins) {
        using T = std::decay_t<decltype(ins)>;

        // ── 1. stack manipulation ───────────────────────────────
        if constexpr (std::is_same_v<T, Push>) {
          emit_mnemonic("PUSH");
          emit_operand(ins.const_index);
          if (ins.const_index < chunk.const_pool.size())
            out_ << "  ; " << chunk.const_pool[ins.const_index].Desc();

        } else if constexpr (std::is_same_v<T, Dup>) {
          emit_mnemonic("DUP");

        } else if constexpr (std::is_same_v<T, Swap>) {
          emit_mnemonic("SWAP");

        } else if constexpr (std::is_same_v<T, Pop>) {
          emit_mnemonic("POP");
          emit_operand(+ins.count);

          // ── 2. arithmetic / logic ───────────────────────────────
        } else if constexpr (std::is_same_v<T, UnaryOp>) {
          emit_mnemonic("UNARY");
          emit_operand(ToString(ins.op));

        } else if constexpr (std::is_same_v<T, BinaryOp>) {
          emit_mnemonic("BINARY");
          emit_operand(ToString(ins.op));

          // ── 3. locals / globals / up-values ─────────────────────
        } else if constexpr (std::is_same_v<T, LoadLocal>) {
          emit_mnemonic("LD_LOCAL");
          emit_operand(+ins.slot);

        } else if constexpr (std::is_same_v<T, StoreLocal>) {
          emit_mnemonic("ST_LOCAL");
          emit_operand(+ins.slot);

        } else if constexpr (std::is_same_v<T, LoadGlobal>) {
          emit_mnemonic("LD_GLOBAL");
          emit_operand(ins.name_index);
          if (ins.name_index < chunk.const_pool.size())
            out_
                << "  ; "
                << chunk.const_pool[ins.name_index].template Get<std::string>();

        } else if constexpr (std::is_same_v<T, StoreGlobal>) {
          emit_mnemonic("ST_GLOBAL");
          emit_operand(ins.name_index);
          if (ins.name_index < chunk.const_pool.size())
            out_
                << "  ; "
                << chunk.const_pool[ins.name_index].template Get<std::string>();

        } else if constexpr (std::is_same_v<T, LoadUpvalue>) {
          emit_mnemonic("LD_UPVAL");
          emit_operand(+ins.slot);

        } else if constexpr (std::is_same_v<T, StoreUpvalue>) {
          emit_mnemonic("ST_UPVAL");
          emit_operand(+ins.slot);

        } else if constexpr (std::is_same_v<T, CloseUpvalues>) {
          emit_mnemonic("CLOSE_UPS");
          emit_operand(+ins.from_slot);

          // ── 4. control-flow ─────────────────────────────────────
        } else if constexpr (std::is_same_v<T, Jump>) {
          long tgt = static_cast<long>(ip) + 1 + ins.offset;
          emit_mnemonic("JUMP");
          emit_operand((ins.offset >= 0 ? "+" : "") +
                       std::to_string(ins.offset));
          out_ << " -> " << tgt;

        } else if constexpr (std::is_same_v<T, JumpIfTrue>) {
          long tgt = static_cast<long>(ip) + 1 + ins.offset;
          emit_mnemonic("JIF_TRUE");
          emit_operand((ins.offset >= 0 ? "+" : "") +
                       std::to_string(ins.offset));
          out_ << " -> " << tgt;

        } else if constexpr (std::is_same_v<T, JumpIfFalse>) {
          long tgt = static_cast<long>(ip) + 1 + ins.offset;
          emit_mnemonic("JIF_FALSE");
          emit_operand((ins.offset >= 0 ? "+" : "") +
                       std::to_string(ins.offset));
          out_ << " -> " << tgt;

        } else if constexpr (std::is_same_v<T, Return>) {
          emit_mnemonic("RETURN");

          // ── 5. functions & calls ────────────────────────────────
        } else if constexpr (std::is_same_v<T, MakeClosure>) {
          emit_mnemonic("MAKE_CLOS");
          emit_operand("");  // keep column spacing
          out_ << "entry=" << ins.entry << "  nargs=" << ins.nparams
               << "  nlocals=" << ins.nlocals << "  nup=" << ins.nupvals;

        } else if constexpr (std::is_same_v<T, Call>) {
          emit_mnemonic("CALL");
          emit_operand(+ins.arity);

        } else if constexpr (std::is_same_v<T, TailCall>) {
          emit_mnemonic("TAIL_CALL");
          emit_operand(+ins.arity);

          // ── 6. objects / classes ───────────────────────────────
        } else if constexpr (std::is_same_v<T, MakeClass>) {
          emit_mnemonic("MAKE_CLASS");
          emit_operand("");  // keep spacing
          out_ << "name=" << ins.name_index << "  nmethods=" << ins.nmethods;

        } else if constexpr (std::is_same_v<T, New>) {
          emit_mnemonic("NEW");

        } else if constexpr (std::is_same_v<T, GetField>) {
          emit_mnemonic("GET_FIELD");
          emit_operand(ins.name_index);

        } else if constexpr (std::is_same_v<T, SetField>) {
          emit_mnemonic("SET_FIELD");
          emit_operand(ins.name_index);

        } else if constexpr (std::is_same_v<T, GetItem>) {
          emit_mnemonic("GET_ITEM");

        } else if constexpr (std::is_same_v<T, SetItem>) {
          emit_mnemonic("SET_ITEM");

          // ── 7. coroutines / fibers ──────────────────────────────
        } else if constexpr (std::is_same_v<T, MakeFiber>) {
          emit_mnemonic("MAKE_FIBER");
          emit_operand("");
          out_ << "entry=" << ins.entry << "  nargs=" << ins.nparams
               << "  nlocals=" << ins.nlocals << "  nup=" << ins.nupvals;

        } else if constexpr (std::is_same_v<T, Resume>) {
          emit_mnemonic("RESUME");
          emit_operand(+ins.arity);

        } else if constexpr (std::is_same_v<T, Yield>) {
          emit_mnemonic("YIELD");

          // ── 8. exceptions ──────────────────────────────────────
        } else if constexpr (std::is_same_v<T, Throw>) {
          emit_mnemonic("THROW");

        } else if constexpr (std::is_same_v<T, TryBegin>) {
          long tgt = static_cast<long>(ip) + 1 + ins.handler_rel_ofs;
          emit_mnemonic("TRY_BEGIN");
          emit_operand("+" + std::to_string(ins.handler_rel_ofs));
          out_ << " -> " << tgt;

        } else if constexpr (std::is_same_v<T, TryEnd>) {
          emit_mnemonic("TRY_END");

          // ── unknown op ─────────────────────────────────────────
        } else {
          emit_mnemonic("<unknown op>");
        }
      },
      chunk.code[ip]);

  out_ << '\n';
}

void Disassembler::DumpImpl(Chunk& chunk, const std::string& indent) {
  if (seen_.contains(&chunk))
    return;
  seen_.insert(&chunk);

  // --- top-level disassembly of this chunk -----------------------------
  for (std::size_t ip = 0; ip < chunk.code.size(); ++ip)
    PrintIns(chunk, ip, indent);

  // --- dive into nested chunks found in the constant-pool --------------
  for (std::size_t idx = 0; idx < chunk.const_pool.size(); ++idx) {
    const std::string sub_indent = indent + std::string(indent_size_, ' ');
    Value& v = chunk.const_pool[idx];
    if (auto clos = v.Get_if<std::shared_ptr<Closure>>(); clos) {
      std::shared_ptr<Chunk> subChunk = clos->chunk;
      if (!subChunk)
        continue;

      out_ << '\n'
           << indent << "; ── nested chunk @const[" << idx << "] ───────────\n";
      DumpImpl(*subChunk, sub_indent);
    }
  }
}

std::string Disassembler::Dump(Chunk& chunk) {
  DumpImpl(chunk, "");
  std::string result = out_.str();

  out_.clear();
  out_.str("");
  seen_.clear();

  return result;
}

}  // namespace serilang
