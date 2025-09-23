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

#include "m6/vm_factory.hpp"

#include "m6/compiler_pipeline.hpp"
#include "srbind/srbind.hpp"
#include "vm/future.hpp"

#include <chrono>
#include <fstream>

namespace m6 {
namespace sr = serilang;
namespace sb = srbind;

static double Time() {
  using namespace std::chrono;
  auto now = system_clock::now().time_since_epoch();
  auto secs = duration_cast<seconds>(now).count();
  return secs;
}

sr::VM VMFactory::Create(std::shared_ptr<sr::GarbageCollector> gc,
                         std::ostream& stdout,
                         std::istream& stdin,
                         std::ostream& stderr) {
  if (!gc)
    gc = std::make_shared<sr::GarbageCollector>();
  sr::VM vm(gc);

  sb::module_ m(vm.gc_.get(), vm.builtins_);
  m.def("time", &Time);

  m.def("__repl_print__", [&stdout](sr::Value result) {
    if (result.Type() != sr::ObjType::Nil)
      stdout << result.Str() << std::endl;
  });
  m.def(
      "print",
      [&stdout](std::string sep, std::string end, bool do_flush,
                std::vector<sr::Value> args) {
        bool firstPrinted = false;
        for (auto const& v : args) {
          if (firstPrinted) {
            stdout << sep;
          }
          stdout << v.Str();
          firstPrinted = true;
        }

        stdout << end;

        if (do_flush)
          stdout.flush();
      },

      sb::kw_arg("sep") = " ", sb::kw_arg("end") = "\n",
      sb::kw_arg("flush") = false, sb::vararg);

  m.def(
      "input",
      [&stdin, &stdout](std::string prompt) {
        if (!prompt.empty())
          stdout << std::move(prompt);
        std::string line;
        std::getline(stdin, line);
        return line;
      },

      sb::kw_arg("prompt") = "");

  vm.builtins_->map.try_emplace(
      "import",
      sr::Value(gc->Allocate<sr::NativeFunction>(
          "import",
          [&vm, &stdin, &stdout, &stderr](sr::VM& vm2, sr::Fiber& f,
                                          uint8_t nargs,
                                          uint8_t nkwargs) -> sr::TempValue {
            if (!(nargs == 1 && nkwargs == 0))
              throw std::runtime_error("import() expects module name");
            sr::Value argv = f.stack.back();
            f.stack.pop_back();
            std::string* modstr = argv.Get_if<std::string>();
            if (auto it = vm.module_cache_.find(*modstr);
                it != vm.module_cache_.end())
              return sr::Value(it->second);

            sr::VM mvm = Create(vm.gc_, stdout, stdin, stderr);
            mvm.gc_threshold_ = 0;  // disable garbage collector

            CompilerPipeline pipe(mvm.gc_, false);
            std::ifstream file(*modstr + ".sr");
            if (!file.is_open())
              throw std::runtime_error("module not found: " + *modstr);
            std::string src((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
            file.close();
            auto sb = m6::SourceBuffer::Create(std::move(src), *modstr);
            pipe.compile(sb);
            if (!pipe.Ok())
              throw std::runtime_error(pipe.FormatErrors());
            sr::Code* chunk = pipe.Get();

            auto mod = std::make_unique<sr::Module>(*modstr, mvm.globals_);
            vm.module_cache_[*modstr] = mod.get();
            mvm.module_cache_ = vm.module_cache_;
            mvm.Evaluate(chunk);

            return std::move(mod);
          })));

  serilang::InstallAsyncBuiltins(vm);
  return vm;
}

}  // namespace m6
