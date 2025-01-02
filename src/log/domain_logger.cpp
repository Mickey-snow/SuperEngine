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

#include "log/domain_logger.hpp"

#include <format>
#include <iostream>

std::string ToString(Severity severity) {
  switch (severity) {
    case Severity::None:
      return "NONE";
    case Severity::Info:
      return "INFO";
    case Severity::Warn:
      return "WARNING";
    case Severity::Error:
      return "ERROR";
    default:
      return "UNKNOWN";
  }
}

DomainLogger::DomainLogger(std::string domain_name) : domain_(domain_name) {}

DomainLogger::~DomainLogger() = default;

DomainLogger::LoggingContent DomainLogger::operator()(Severity severity) {
  std::map<std::string, std::string> attr;
  attr["severity"] = ToString(severity);
  attr["domain"] = domain_;
  return LoggingContent(std::move(attr));
}

DomainLogger::LoggingContent::LoggingContent(
    std::map<std::string, std::string> attr)
    : attr_(std::move(attr)) {}

DomainLogger::LoggingContent::~LoggingContent() {
  std::clog << std::format("[{}] {}: {}", attr_["severity"], attr_["domain"],
                           msg_.str())
            << std::endl;
}
