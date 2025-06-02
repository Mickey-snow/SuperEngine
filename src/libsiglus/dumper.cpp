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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
// -----------------------------------------------------------------------

#include "libsiglus/dumper.hpp"

#include "core/gameexe.hpp"
#include "libsiglus/archive.hpp"
#include "libsiglus/gexedat.hpp"
#include "libsiglus/parser.hpp"
#include "libsiglus/xorkey.hpp"
#include "utilities/string_utilities.hpp"

#include <filesystem>
#include <format>
#include <numeric>

namespace fs = std::filesystem;
using namespace std::placeholders;

namespace libsiglus {

Dumper::Dumper(const std::filesystem::path& gexe_path,
               const std::filesystem::path& scene_path)
    : gexe_data_(gexe_path),
      archive_data_(scene_path),
      gexe_(CreateGexe(gexe_data_.Read())),
      archive_(Archive::Create(archive_data_.Read())) {}

std::vector<IDumper::Task> Dumper::GetTasks(std::vector<int> scenarios) {
  bool dump_metadata = false;
  if (scenarios.empty()) {
    dump_metadata = true;
    scenarios.resize(archive_.scndata_.size());
    std::iota(scenarios.begin(), scenarios.end(), 0);
  }

  std::vector<IDumper::Task> result;
  result.reserve(archive_.scndata_.size() + 2);
  using tsk_t = typename IDumper::task_t;

  if (dump_metadata) {
    result.emplace_back("gameexe.txt",
                        tsk_t(std::bind(&Dumper::DumpGexe, this, _1)));
    result.emplace_back("archive.txt",
                        tsk_t(std::bind(&Dumper::DumpArchive, this, _1)));
  }

  for (const auto& i : scenarios) {
    result.emplace_back(std::format("s{:04}.txt", i),
                        tsk_t(std::bind(&Dumper::DumpScene, this, i, _1)));
  }
  return result;
}

void Dumper::DumpGexe(std::ostream& out) {
  for (const auto& it : gexe_.Filter(""))
    out << it.key() << " = " << Join(",", it.ToStrVector()) << std::endl;
}

void Dumper::DumpArchive(std::ostream& out) {
  std::string result;
  for (const auto& p : archive_.prop_)
    out << "prop " << p.name << " {" << ToString(p.form) << ',' << p.size << '}'
        << std::endl;
  for (const auto& c : archive_.cmd_)
    out << std::format("cmd {} @{}-{}", c.name, c.scene_id, c.offset)
        << std::endl;
  ;
}

void Dumper::DumpScene(size_t id, std::ostream& out) {
  Scene& scn = archive_.scndata_[id];
  out << id << ' ' << scn.scnname_ << std::endl;

  struct TokenDumper : public Parser::OutputBuffer {
    size_t idx;
    std::ostream& out;
    TokenDumper(std::ostream& o) : idx(1), out(o) {}
    void operator=(token::Token_t tok) final {
      out << idx++ << ": " << ToString(tok) << std::endl;
    }
  };

  Parser parser(archive_, scn, std::make_shared<TokenDumper>(out));

  try {
    parser.ParseAll();
  } catch (std::exception& e) {
    out << '\n' << e.what() << std::endl;
  }
}

}  // namespace libsiglus
