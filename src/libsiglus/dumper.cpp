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

namespace fs = std::filesystem;

namespace libsiglus {

Dumper::Dumper(const std::filesystem::path& gexe_path,
               const std::filesystem::path& scene_path)
    : gexe_data_(gexe_path),
      archive_data_(scene_path),
      gexe_(CreateGexe(gexe_data_.Read())),
      archive_(Archive::Create(archive_data_.Read())) {}

std::vector<IDumper::Task> Dumper::GetTasks() {
  std::vector<IDumper::Task> result;
  result.reserve(archive_.scndata_.size() + 2);
  using tsk_t = std::packaged_task<std::string()>;

  result.emplace_back("gameexe.txt", tsk_t(std::bind(&Dumper::DumpGexe, this)));
  result.emplace_back("archive.txt",
                      tsk_t(std::bind(&Dumper::DumpArchive, this)));
  for (size_t i = 0; i < archive_.scndata_.size(); ++i)
    result.emplace_back(std::format("s{:04}.txt", i),
                        tsk_t(std::bind(&Dumper::DumpScene, this, i)));
  return result;
}

std::string Dumper::DumpGexe() {
  std::string result;
  for (const auto& it : gexe_.Filter(""))
    result += it.key() + " = " + Join(",", it.ToStrVector()) + '\n';
  return result;
}

std::string Dumper::DumpArchive() {
  std::string result;
  for (const auto& p : archive_.prop_) {
    result += "prop " + p.name + " {" + ToString(p.form) + ',' +
              std::to_string(p.size) + "}\n";
  }
  for (const auto& c : archive_.cmd_) {
    result += std::format("cmd {} @{}-{}\n", c.name, c.scene_id, c.offset);
  }

  return result;
}

std::string Dumper::DumpScene(size_t id) {
  Scene& scn = archive_.scndata_[id];
  std::string result;
  result += std::to_string(id) + ' ' + scn.scnname_ + '\n';

  struct TokenDumper : public Parser::OutputBuffer {
    size_t idx;
    std::string& result;
    TokenDumper(std::string& r) : idx(1), result(r) {}
    void operator=(token::Token_t tok) final {
      this->result += std::to_string(idx++) + ": " + ToString(tok) + '\n';
    }
  };

  Parser parser(archive_, scn, std::make_shared<TokenDumper>(result));

  try {
    parser.ParseAll();
  } catch (std::exception& e) {
    result += '\n';
    result += e.what();
    result += '\n';
  }

  return result;
}

}  // namespace libsiglus
