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

#include "libsiglus/sgvm_factory.hpp"
#include "m6/compiler_pipeline.hpp"
#include "m6/vm_factory.hpp"
#include "utilities/string_utilities.hpp"
#include "vm/disassembler.hpp"

#include <boost/program_options.hpp>
#include <chrono>
#include <fstream>
#include <iostream>

namespace po = boost::program_options;

using namespace m6;
using namespace serilang;

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

static void run_repl(VM vm) {
  CompilerPipeline pipeline(vm.gc_, /*repl_mode=*/true);

  std::string line, line_trimmed;
  for (size_t lineno = 1; std::cout << ">> " && std::getline(std::cin, line);
       ++lineno) {
    if (line.empty())
      continue;

    line_trimmed = trim_cp(line);

    if (line_trimmed == "exit")
      break;
    else if (line_trimmed.starts_with("run")) {
      // helper to paste and run a file
      std::string file_name = line_trimmed.substr(3);
      trim(file_name);
      if (!file_name.ends_with(".sr"))
        file_name += ".sr";
      try {
        std::ifstream ifs(file_name);
        if (!ifs.is_open())
          throw std::runtime_error("file not found: " + file_name);
        line = std::string(std::istreambuf_iterator<char>(ifs),
                           std::istreambuf_iterator<char>());
      } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        continue;
      }
    } else if (line_trimmed.starts_with("dis")) {
      // helper to compile and dump a file
      std::string file_name = trim_cp(line).substr(3);
      trim(file_name);
      if (!file_name.ends_with(".sr"))
        file_name += ".sr";
      try {
        std::ifstream ifs(file_name);
        if (!ifs.is_open())
          throw std::runtime_error("file not found: " + file_name);
        line = std::string(std::istreambuf_iterator<char>(ifs),
                           std::istreambuf_iterator<char>());
      } catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
        continue;
      }

      pipeline.compile(SourceBuffer::Create(
          std::move(line), "<input-" + std::to_string(lineno) + '>'));
      if (!pipeline.Ok()) {
        std::cerr << pipeline.FormatErrors() << std::flush;
        continue;
      }

      auto chunk = pipeline.Get();
      if (!chunk)
        continue;

      std::cout << Disassembler().Dump(*chunk) << std::endl;
      continue;
    }

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

int main(int argc, char* argv[]) {
  bool use_siglus;
  std::cout << copyright_info << "\n\n" << help_info << std::endl;
  try {
    // Define options
    po::options_description desc("Allowed options");
    desc.add_options()("help,h", "Produce help message")(
        "siglus", po::value<bool>(&use_siglus)->default_value(true));

    // Parse arguments
    po::variables_map vm;
    po::store(po::command_line_parser(argc, argv).options(desc).run(), vm);

    // Handle --help
    if (vm.count("help")) {
      std::cout << "rlkp" << '\n';
      std::cout << desc << '\n';
      return 0;
    }

    po::notify(vm);
  } catch (const po::error& e) {
    std::cerr << "Error: " << e.what() << "\n";
    return 1;
  } catch (const std::exception& e) {
    std::cerr << "Exception: " << e.what() << "\n";
    return 1;
  }

  try {
    run_repl(use_siglus ? libsiglus::SGVMFactory().Create()
                        : m6::VMFactory::Create());
  } catch (std::exception const& ex) {
    std::cerr << "fatal: " << ex.what() << '\n';
    return 1;
  }
  return 0;
}
