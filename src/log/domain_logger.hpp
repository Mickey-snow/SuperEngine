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

#pragma once

#include "log/core.hpp"

#include <map>
#include <memory>
#include <sstream>
#include <string>

class Logger;

class DomainLogger {
 public:
  DomainLogger(std::string domain_name = "");
  ~DomainLogger();

  class LoggingContent {
   public:
    LoggingContent(std::unique_ptr<Logger> logger);
    ~LoggingContent();

    template <typename T>
    LoggingContent& operator<<(T&& x) {
      if (logger_)
        msg_ << std::forward<T>(x);
      return *this;
    }

   private:
    std::unique_ptr<Logger> logger_;
    std::stringstream msg_;
  };
  LoggingContent operator()(Severity severity = Severity::None) const;

 private:
  std::string domain_;
};
