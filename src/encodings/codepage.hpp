/*
  rlBabel: abstract base class for codepage definitions

  Copyright (c) 2006 Peter Jolly.

  This library is free software; you can redistribute it and/or modify it under
  the terms of the GNU Lesser General Public License as published by the Free
  Software Foundation; either version 2.1 of the License, or (at your option)
  any later version.

  This library is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
  FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
  details.

  You should have received a copy of the GNU Lesser General Public License
  along with this library; if not, write to the Free Software Foundation, Inc.,
  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA

  As a special exception to the GNU Lesser General Public License (LGPL), you
  may include a publicly distributed version of the library alongside a "work
  that uses the Library" to produce a composite work that includes the library,
  and distribute that work under terms of your choice, without any of the
  additional requirements listed in clause 6 of the LGPL.

  A "publicly distributed version of the library" means either an unmodified
  binary as distributed by Haeleth, or a modified version of the library that is
  distributed under the conditions defined in clause 2 of the LGPL, and a
  "composite work that includes the library" means a RealLive program which
  links to the library, either through the LoadDLL() interface or a #DLL
  directive, and/or includes code from the library's Kepago header.

  Note that this exception does not invalidate any other reasons why any part of
  the work might be covered by the LGPL.
*/

#pragma once

#include <memory>
#include <string>

enum class Encoding : int {
  Unknown = -1,
  cp932 = 0,
  cp936 = 1,
  cp1252 = 2,
  cp949 = 3,
  utf8 = 10,
  utf16 = 11,
  utf32 = 12
};

class Codepage {
 public:
  virtual ~Codepage();
  virtual unsigned short JisDecode(unsigned short ch) const;
  virtual void JisDecodeString(const char* s, char* buf, size_t buflen) const;
  virtual void JisEncodeString(const char* s, char* buf, size_t buflen) const;
  virtual unsigned short Convert(unsigned short ch) const = 0;
  virtual std::wstring ConvertString(const std::string& s) const = 0;
  virtual bool DbcsDelim(char* str) const;
  virtual bool IsItalic(unsigned short ch) const;

  virtual std::string ConvertTo_utf8(const std::string& input) const;

  // Factory method to create a codepage converter, supported encodings are:
  // cp932, cp936, cp949, cp1252.
  static std::shared_ptr<Codepage> Create(Encoding encoding);
};

class Cp {
 public:
  // Singleton constructor
  static Codepage& instance(int desired);

 private:
  static std::unique_ptr<Codepage> instance_;
  static int codepage;
  static int scenario;
};
