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

#ifndef SRC_LIBREALLIVE_FILEMAP_H_
#define SRC_LIBREALLIVE_FILEMAP_H_

#include "libreallive/alldefs.h"

#include <string_view>
#include <filesystem>
#include "boost/iostreams/device/mapped_file.hpp"

namespace libreallive {

namespace fs = std::filesystem;

class MappedFile {
 public:
  MappedFile(const std::string& filename, std::size_t size = 0);
  MappedFile(const fs::path& filepath, std::size_t size = 0);
  ~MappedFile();

  std::string_view Read(std::size_t position, std::size_t length);

  bool Write(std::size_t position, const std::string& data);

  std::size_t size() const { return file_.size(); }
  std::size_t Size() const { return file_.size(); }

  // Direct access to internal memory, should be removed
  const char* get() {
    if (!file_.is_open())
      throw Error("File not open");
    return file_.const_data();
  }

 private:
  boost::iostreams::mapped_file file_;
};

struct FilePos {
  std::shared_ptr<MappedFile> file_;
  int position, length;

  std::string_view Read() {
    if (!file_)
      throw Error("File is nullptr");
    return file_->Read(position, length);
  }
};

}  // namespace libreallive

#endif  // SRC_LIBREALLIVE_FILEMAP_H_
