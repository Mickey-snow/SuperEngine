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

#include "m6/compiler_pipeline.hpp"
#include "vm/vm.hpp"

#include <iostream>

using namespace m6;

static constexpr std::string_view copyright_info = R"(
Copyright (C) 2025 Serina Sakurai

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.)";

static constexpr std::string_view help_info =
    R"(Reallive REPL – enter code, Ctrl-D or \"exit\" to quit)";

static void run_repl() {
  CompilerPipeline pipeline(/*repl_mode=*/true);
  auto dummy = std::make_shared<serilang::Chunk>();
  serilang::VM vm(dummy);

  std::string line;
  for (size_t lineno = 1; std::cout << ">> " && std::getline(std::cin, line);
       ++lineno) {
    if (line == "exit")
      break;
    if (line.empty())
      continue;

    pipeline.compile(SourceBuffer::Create(
        std::move(line), "<input-" + std::to_string(lineno) + '>'));
    if (!pipeline.Ok()) {
      std::cerr << pipeline.FormatErrors() << std::flush;
      continue;
    }

    auto chunk = pipeline.Get();
    if (!chunk)
      continue;

    try {
      // run just this snippet on the existing VM, preserving globals, etc.
      std::ignore = vm.Evaluate(chunk);
    } catch (const std::exception& ex) {
      std::cerr << "runtime: " << ex.what() << '\n';
    }
  }
}

// ──────────────────────────────────────────────────────────────────────────────
int main() {
  std::cout << copyright_info << "\n\n" << help_info << std::endl;

  try {
    run_repl();
  } catch (std::exception const& ex) {
    std::cerr << "fatal: " << ex.what() << '\n';
    return 1;
  }
  return 0;
}
