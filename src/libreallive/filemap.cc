// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of libreallive, a dependency of RLVM.
//
// -----------------------------------------------------------------------
//
// Copyright (c) 2006 Peter Jolly
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

#include <sstream>

#include "libreallive/alldefs.h"
#include "libreallive/filemap.h"

namespace libreallive {

MappedFile::MappedFile(const std::string& filename, std::size_t size) {
  const bool is_readonly = size == 0;
  if (is_readonly) {
    file_.open(filename, boost::iostreams::mapped_file::mapmode::readonly);
  } else {
    file_.open(filename, boost::iostreams::mapped_file::mapmode::readwrite,
               size);
  }

  if (!file_.is_open()) {
    std::stringstream ss;
    ss << "Failed to open file:" << filename << std::endl;
    throw Error(ss.str());
  }
}

MappedFile::~MappedFile() {
  if (file_.is_open())
    file_.close();
}

MappedFile::MappedFile(const fs::path& filepath, std::size_t size)
    : MappedFile(filepath.generic_string(), size) {}

std::string_view MappedFile::Read(std::size_t position, std::size_t length) {
  if (!file_.is_open())
    throw Error("File not open");
  if (position + length > file_.size())
    throw Error("Read operation out of bounds");

  return std::string_view(file_.const_data() + position, length);
}

bool MappedFile::Write(std::size_t position, const std::string& data) {
  if (!file_.is_open()) {
    throw Error("File not open");
    return false;
  }
  if (position + data.size() > file_.size()) {
    throw Error("Write operation out of bounds");
    return false;
  }

  char* dst = file_.data();
  if (dst == nullptr) {
    throw Error("No write permission to file");
    return false;
  }

  std::copy(data.cbegin(), data.cend(), dst + position);
  return true;
}

}  // namespace libreallive
