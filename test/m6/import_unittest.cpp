// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------

#include <gtest/gtest.h>

#include "m6/compiler_pipeline.hpp"
#include "m6/source_buffer.hpp"
#include "utilities/string_utilities.hpp"
#include "vm/disassembler.hpp"
#include "vm/object.hpp"
#include "vm/value.hpp"
#include "vm/vm.hpp"

#include <boost/filesystem.hpp>
#include <fstream>
#include <sstream>

namespace m6test {

using namespace m6;
using namespace serilang;
namespace fs = boost::filesystem;

struct ExecutionResult {
  Value last;
  std::string stdout;
  std::string stderr;
  std::string disasm;
};

static ExecutionResult Run(std::string source) {
  std::stringstream inBuf, outBuf, errBuf;
  auto vm = VM::Create(outBuf, inBuf, errBuf);
  CompilerPipeline pipe(vm.gc_, false);
  auto sb = SourceBuffer::Create(std::move(source), "<import_test>");
  pipe.compile(sb);
  ExecutionResult r{};
  if (!pipe.Ok()) {
    r.stderr = pipe.FormatErrors();
    return r;
  }
  auto chunk = pipe.Get();
  r.disasm = Disassembler().Dump(*chunk);
  try {
    r.last = vm.Evaluate(chunk);
  } catch (std::exception const& ex) {
    errBuf << ex.what();
  }
  r.stdout = outBuf.str();
  r.stderr += errBuf.str();
  return r;
}

TEST(ImportTest, ImportOnce) {
  fs::path tmp = fs::temp_directory_path() / fs::unique_path("modXXXX");
  std::string modname = tmp.string();
  std::ofstream(modname + ".seri") << "print(\"loaded\\n\"); value=42;";

  std::string script = "import " + modname + ";\nimport " + modname +
                       ";\nprint(" + modname + ".value);";
  auto res = Run(script);
  fs::remove(modname + ".seri");

  EXPECT_EQ(res.stderr, "");
  EXPECT_EQ(res.stdout, "loaded\n42\n");
}

TEST(ImportTest, FromImport) {
  fs::path tmp = fs::temp_directory_path() / fs::unique_path("modXXXX");
  std::string modname = tmp.string();
  std::ofstream(modname + ".seri") << "val=1";
  std::string script =
      "from " + modname + " import val; val = val + 1; print(val);";
  auto res = Run(script);
  fs::remove(modname + ".seri");

  EXPECT_EQ(res.stderr, "");
  EXPECT_EQ(res.stdout, "2\n");
}

}  // namespace m6test
