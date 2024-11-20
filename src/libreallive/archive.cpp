// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of libreallive, a dependency of RLVM.
//
// -----------------------------------------------------------------------
//
// Copyright (c) 2006, 2007 Peter Jolly
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use, copy,
// modify, merge, publish, distribute, sublicense, and/or sell copies
// of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
// ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// -----------------------------------------------------------------------

#include <boost/algorithm/string.hpp>
#include <cstring>
#include <filesystem>
#include <string>

#include "libreallive/archive.hpp"
#include "libreallive/xorkey.hpp"
#include "utilities/byte_reader.hpp"
#include "utilities/mapped_file.hpp"

using boost::iends_with;
using boost::istarts_with;
namespace fs = std::filesystem;

namespace libreallive {

Archive::Archive(const fs::path& filepath, const std::string& regname)
    : second_level_xor_key_(NULL), regname_(regname) {
  ReadTOC(filepath);
  ReadOverrides(filepath);
  FindXorKey();
}

Archive::~Archive() {}

Scenario* Archive::GetScenario(int index) {
  auto at = scenario_.find(index);
  if (at != scenario_.end())
    return at->second.get();
  else {  // make the scenario on demand
    toc_t::const_iterator st = toc_.find(index);
    if (st != toc_.end()) {
      auto scene_ptr =
          ParseScenario(st->second, index, regname_, second_level_xor_key_);
      auto raw_ptr = scene_ptr.get();
      scenario_[index] = std::move(scene_ptr);
      return raw_ptr;
    }
    return NULL;
  }
}

int Archive::GetFirstScenarioID() const { return toc_.cbegin()->first; }

int Archive::GetProbableEncodingType() const {
  // Directly create Header objects instead of Scenarios. We don't want to
  // parse the entire SEEN file here.
  for (auto it = toc_.cbegin(); it != toc_.cend(); ++it) {
    FilePos filepos = it->second;
    Header header(filepos.Read());
    if (header.rldev_metadata_.text_encoding() != 0)
      return header.rldev_metadata_.text_encoding();
  }

  return 0;
}

void Archive::ReadTOC(const fs::path& filepath) {
  std::shared_ptr<MappedFile> header = std::make_shared<MappedFile>(filepath);
  static constexpr int TOC_COUNT = 10000;
  static constexpr std::size_t TOC_SIZE = 8;

  ByteReader reader(header->Read(0, TOC_COUNT * TOC_SIZE));
  for (int i = 0; i < TOC_COUNT; ++i) {
    const int offset = reader.PopAs<int>(4);
    const int length = reader.PopAs<int>(4);

    if (offset)
      toc_[i] = FilePos{.file_ = header, .position = offset, .length = length};
  }
}

void Archive::ReadOverrides(const fs::path& filepath) {
  // Iterate over all files in the directory and override the table of contents
  // if there is a free SEENXXXX.TXT file.
  const fs::path seen_dir = filepath.parent_path();
  fs::directory_iterator end;
  for (fs::directory_iterator it(seen_dir); it != end; ++it) {
    std::string filename = it->path().filename().string();
    if (filename.size() == 12 && istarts_with(filename, "seen") &&
        iends_with(filename, ".txt") && isdigit(filename[4]) &&
        isdigit(filename[5]) && isdigit(filename[6]) && isdigit(filename[7])) {
      FilePos filepos;
      filepos.file_ = std::make_shared<MappedFile>(filename);
      filepos.position = 0;
      filepos.length = filepos.file_->Size();

      int index = std::stoi(filename.substr(4, 4));
      toc_[index] = filepos;
    }
  }
}

void Archive::FindXorKey() {
  if (regname_ == "KEY\\CLANNAD_FV") {
    second_level_xor_key_ = libreallive::clannad_full_voice_xor_mask;
  } else if (regname_ ==
             "\x4b\x45\x59\x5c\x83\x8a\x83\x67\x83\x8b\x83"
             "\x6f\x83\x58\x83\x5e\x81\x5b\x83\x59\x81\x49") {
    second_level_xor_key_ = libreallive::little_busters_xor_mask;
  } else if (regname_ ==
             "\x4b\x45\x59\x5c\x83\x8a\x83\x67\x83\x8b\x83\x6f\x83\x58\x83\x5e"
             "\x81\x5b\x83\x59\x81\x49\x82\x64\x82\x77") {
    // "KEY\<little busters in katakana>!EX", with all fullwidth latin
    // characters.
    second_level_xor_key_ = libreallive::little_busters_ex_xor_mask;
  } else if (regname_ == "StudioMebius\\SNOWSE") {
    second_level_xor_key_ = libreallive::snow_standard_edition_xor_mask;
  } else if (regname_ ==
             "\x4b\x45\x59\x5c\x83\x4e\x83\x68\x82\xed\x82\xd3\x82"
             "\xbd\x81\x5b") {
    // "KEY\<Kud Wafter in hiragana>"
    second_level_xor_key_ = libreallive::kud_wafter_xor_mask;
  } else if (regname_ ==
             "\x4b\x45\x59\x5c\x83\x4e\x83\x68\x82\xed\x82\xd3\x82"
             "\xbd\x81\x5b\x81\x79\x91\x53\x94\x4e\x97\xee\x91\xce"
             "\x8f\xdb\x94\xc5\x81\x7a") {
    second_level_xor_key_ = libreallive::kud_wafter_all_ages_xor_mask;
  }
}

}  // namespace libreallive
