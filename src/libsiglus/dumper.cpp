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

#include "core/avdec/audio_decoder.hpp"
#include "core/avdec/image_decoder.hpp"
#include "core/avdec/wav.hpp"
#include "core/gameexe.hpp"
#include "libsiglus/archive.hpp"
#include "libsiglus/gexedat.hpp"
#include "libsiglus/parser.hpp"
#include "parser_context.hpp"
#include "utilities/mapped_file.hpp"
#include "utilities/string_utilities.hpp"

#include <filesystem>
#include <format>
#include <numeric>
#include <ostream>

using namespace std::placeholders;

namespace libsiglus {

Dumper::Dumper(const std::filesystem::path& gexe_path,
               const std::filesystem::path& scene_path,
               const std::filesystem::path& root_path)
    : gexe_data_(gexe_path),
      archive_data_(scene_path),
      gexe_(CreateGexe(gexe_data_.Read())),
      archive_(Archive::Create(archive_data_.Read())) {
  scanner_.IndexDirectory(root_path);
}

std::vector<IDumper::Task> Dumper::GetTasks(std::vector<int> scenarios) {
  bool dump_metadata = false;
  if (scenarios.empty()) {
    dump_metadata = true;
    scenarios.resize(archive_.GetScenarioCount());
    std::iota(scenarios.begin(), scenarios.end(), 0);
  }

  std::vector<IDumper::Task> result;
  result.reserve(archive_.GetScenarioCount() + 2);
  using tsk_t = typename IDumper::task_t;

  for (const auto& i : scenarios) {
    result.emplace_back(std::format("s{:04}.txt", i),
                        tsk_t(std::bind(&Dumper::DumpScene, this, i, _1)));
  }

  if (dump_metadata) {
    result.emplace_back("gameexe.txt",
                        tsk_t(std::bind(&Dumper::DumpGexe, this, _1)));
    result.emplace_back("archive.txt",
                        tsk_t(std::bind(&Dumper::DumpArchive, this, _1)));

    for (const auto& it : scanner_.filesystem_cache_) {
      static const std::set<std::string> audio_ext{"nwa", "wav", "ogg", "mp3",
                                                   "ovk", "koe", "nwk"};
      static const std::set<std::string> image_ext{"g00", "pdt"};
      auto name = it.first;
      auto [ext, path] = it.second;

      if (audio_ext.contains(ext)) {
        result.emplace_back(
            std::filesystem::path("audio") / (name + '.' + ext),
            tsk_t(std::bind(&Dumper::DumpAudio, this, path, _1)));
      } else if (image_ext.contains(ext)) {
        result.emplace_back(
            std::filesystem::path("image") / (name + '.' + ext),
            tsk_t(std::bind(&Dumper::DumpImage, this, path, _1)));
      }
    }
  }

  return result;
}

void Dumper::DumpGexe(std::ostream& out) {
  for (const auto& it : gexe_.Filter(""))
    out << it.key() << " = " << Join(",", it.ToStrVec()) << '\n';
  out.flush();
}

void Dumper::DumpArchive(std::ostream& out) {
  for (size_t i = 0; i < archive_.prop_.size(); ++i) {
    auto& p = archive_.prop_[i];
    out << std::format("prop.{} {}\n", i, p.ToDebugString());
  }
  for (size_t i = 0; i < archive_.cmd_.size(); ++i) {
    auto& c = archive_.cmd_[i];
    out << std::format("cmd.{} {}\n", i, c.ToDebugString());
  }
  out.flush();
}

void Dumper::DumpScene(size_t id, std::ostream& out) {
  Scene scn = archive_.ParseScene(id);
  out << id << ' ' << scn.scnname_ << '\n';  // title
  out << '\n'
      << std::string(3, '=') << " header " << std::string(20, '=') << '\n';
  for (size_t i = 0; i < scn.property.size(); ++i) {
    auto& p = scn.property[i];
    out << std::format("prop.{} {}\n", i, p.ToDebugString());
  }
  for (size_t i = 0; i < scn.cmd.size(); ++i) {
    auto& c = scn.cmd[i];
    out << std::format("cmd.{} {}\n", i, c.ToDebugString());
  }

  out << '\n'
      << std::string(3, '=') << " script " << std::string(20, '=') << '\n';
  struct Context : public ParserContext {
    Context(Archive& ar, Scene& sc, std::ostream& o, std::ostream& e)
        : ParserContext(ar, sc), idx(1), out(o), err(e) {}
    size_t idx;
    std::ostream& out;
    std::ostream& err;

    void Emit(token::Token_t tok) final {
      out << idx++ << ": " << ToString(tok) << '\n';
    }
    void Warn(std::string msg) final { err << msg << '\n'; }
  };

  Context ctx(archive_, scn, out, out);
  Parser parser(ctx);

  try {
    parser.ParseAll();
  } catch (std::exception& e) {
    out << '\n' << e.what() << '\n';
  }
  out.flush();
}

void Dumper::DumpAudio(std::filesystem::path path, std::ostream& s) {
  AudioDecoder decoder(path);
  AudioData data = decoder.DecodeAll();
  std::vector<uint8_t> wav = EncodeWav(std::move(data));
  s.write(reinterpret_cast<const char*>(wav.data()),
          static_cast<std::streamsize>(wav.size()));
}

void Dumper::DumpImage(std::filesystem::path path, std::ostream& s) {
  MappedFile mfile(path);
  ImageDecoder decoder(mfile.Read());
  saveBGRAasPPM(s, decoder.width, decoder.height, decoder.mem);
}

}  // namespace libsiglus
