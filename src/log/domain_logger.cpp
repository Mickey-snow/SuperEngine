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

#include "log/logger.hpp"

#include <format>
#include <iostream>

DomainLogger::DomainLogger(std::string domain_name) : domain_(domain_name) {}

DomainLogger::~DomainLogger() = default;

DomainLogger::LoggingContent DomainLogger::operator()(Severity severity) {
  if (!logging_enabled)
    return LoggingContent(nullptr);

  auto logger = std::make_unique<Logger>();
  if (!domain_.empty())
    logger->AddScope(domain_);
  logger->AddSeverity(severity);
  return LoggingContent(std::move(logger));
}

DomainLogger::LoggingContent::LoggingContent(std::unique_ptr<Logger> logger)
    : logger_(std::move(logger)) {}

DomainLogger::LoggingContent::~LoggingContent() {
  if (logger_)
    logger_->Log(msg_.str());
}
