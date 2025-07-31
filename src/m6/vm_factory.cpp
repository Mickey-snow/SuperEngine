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

#include <chrono>
#include <fstream>

namespace m6 {
namespace sr = serilang;

sr::VM VMFactory::Create(std::shared_ptr<sr::GarbageCollector> gc,
                         std::ostream& stdout,
                         std::istream& stdin,
                         std::ostream& stderr) {
  if (!gc)
    gc = std::make_shared<sr::GarbageCollector>();
  sr::VM vm(gc);

  sr::Dict* builtins =
      gc->Allocate<sr::Dict>(std::unordered_map<std::string, sr::Value>{
          {"time",
           sr::Value(gc->Allocate<sr::NativeFunction>(
               "time",
               [](sr::VM& vm, sr::Fiber& f, sr::Value self,
                  std::vector<sr::Value> args,
                  std::unordered_map<std::string, sr::Value> /*kwargs*/) {
                 if (!args.empty())
                   throw std::runtime_error("time() takes no arguments");
                 using namespace std::chrono;
                 auto now = system_clock::now().time_since_epoch();
                 auto secs = duration_cast<seconds>(now).count();
                 return sr::Value(static_cast<int>(secs));
               }))},

          {"print",
           sr::Value(gc->Allocate<sr::NativeFunction>(
               "print",
               [&stdout](sr::VM& vm, sr::Fiber& f, sr::Value self,
                         std::vector<sr::Value> args,
                         std::unordered_map<std::string, sr::Value> kwargs) {
                 std::string sep = " ";
                 if (auto it = kwargs.find("sep"); it != kwargs.end())
                   sep = it->second.Str();

                 std::string end = "\n";
                 if (auto it = kwargs.find("end"); it != kwargs.end())
                   end = it->second.Str();

                 bool do_flush = false;
                 if (auto it = kwargs.find("flush"); it != kwargs.end())
                   do_flush = it->second.IsTruthy();

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

                 return sr::nil;
               }))},

          {"input",
           sr::Value(gc->Allocate<sr::NativeFunction>(
               "input",
               [&stdin](sr::VM& vm, sr::Fiber& f, sr::Value self,
                        std::vector<sr::Value> args,
                        std::unordered_map<std::string, sr::Value> kwargs) {
                 std::string inp;
                 stdin >> inp;
                 return sr::Value(std::move(inp));
               }))},

          {"import",
           sr::Value(gc->Allocate<sr::NativeFunction>(
               "import",
               [&vm, &stdin, &stdout, &stderr](
                   sr::VM& vm2, sr::Fiber& f, sr::Value self,
                   std::vector<sr::Value> args,
                   std::unordered_map<std::string, sr::Value> /*kwargs*/) {
                 if (args.size() != 1)
                   throw std::runtime_error("import() expects module name");
                 std::string modstr = args[0].Str();
                 if (auto it = vm.module_cache_.find(modstr);
                     it != vm.module_cache_.end())
                   return sr::Value(it->second);

                 sr::VM mvm = Create(vm.gc_, stdout, stdin, stderr);
                 mvm.gc_threshold_ = 0;  // disable garbage collector

                 CompilerPipeline pipe(mvm.gc_, false);
                 std::ifstream file(modstr + ".sr");
                 if (!file.is_open())
                   throw std::runtime_error("module not found: " + modstr);
                 std::string src((std::istreambuf_iterator<char>(file)),
                                 std::istreambuf_iterator<char>());
                 file.close();
                 auto sb = m6::SourceBuffer::Create(std::move(src), modstr);
                 pipe.compile(sb);
                 if (!pipe.Ok())
                   throw std::runtime_error(pipe.FormatErrors());
                 sr::Code* chunk = pipe.Get();

                 sr::Module* mod =
                     vm.gc_->Allocate<sr::Module>(modstr, mvm.globals_);
                 vm.module_cache_[modstr] = mod;
                 mvm.module_cache_ = vm.module_cache_;
                 mvm.Evaluate(chunk);

                 return sr::Value(mod);
               }))}});

  vm.builtins_ = builtins;

  return vm;
}

}  // namespace m6
