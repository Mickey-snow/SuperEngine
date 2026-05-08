// -----------------------------------------------------------------------
//
// This file is part of RLVM
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2026 Serina Sakurai
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
// -----------------------------------------------------------------------

#include "libsiglus/bindings/util.hpp"

#include "m6/compiler_pipeline.hpp"
#include "m6/source_buffer.hpp"
#include "vm/vm.hpp"

#include <stdexcept>
#include <string>
#include <utility>

namespace libsiglus::binding {

namespace sr = serilang;

// for code injection
sr::Value Execute(sr::VM& vm, std::string src) {
  m6::CompilerPipeline pipe(vm.gc_, false);
  auto sb = m6::SourceBuffer::Create(std::move(src), "<siglus bootstrap>");
  pipe.compile(sb);

  if (!pipe.Ok())
    throw std::runtime_error(pipe.FormatErrors());

  auto* chunk = pipe.Get();
  if (!chunk)
    throw std::runtime_error("pipeline returned null chunk\n" + sb->GetStr());

  sr::Value result = vm.Evaluate(chunk);
  return result;
}

}  // namespace libsiglus::binding
