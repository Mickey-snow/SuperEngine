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

#include "log/core.hpp"

#include <boost/log/attributes/constant.hpp>
#include <boost/log/attributes/mutable_constant.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include <iostream>
#include <string>

std::ostream& operator<<(std::ostream& os, Severity severity) {
  switch (severity) {
    case Severity::None:
      os << "NONE";
      break;
    case Severity::Info:
      os << "INFO";
      break;
    case Severity::Warn:
      os << "WARNING";
      break;
    case Severity::Error:
      os << "ERROR";
      break;
    default:
      os << "UNKNOWN";
      break;
  }
  return os;
}

bool logging_enabled = false;

void SetupLogging(Severity minSeverity) {
  if (logging_enabled)
    return;
  logging_enabled = true;

  namespace logging = boost::log;
  namespace expr = boost::log::expressions;
  namespace sinks = boost::log::sinks;
  namespace attrs = boost::log::attributes;

  using text_sink =
      typename sinks::synchronous_sink<sinks::text_ostream_backend>;
  boost::shared_ptr<text_sink> sink = boost::make_shared<text_sink>();

  sink->locked_backend()->add_stream(
      boost::shared_ptr<std::ostream>(&std::clog, boost::null_deleter()));

  sink->set_formatter([](logging::record_view const& rec,
                         logging::formatting_ostream& strm) {
    const auto severity =
        logging::extract_or_default<Severity>("Severity", rec, Severity::None);
    const auto scope =
        logging::extract_or_default<std::string>("Scope", rec, "");
    bool should_insert_spc = false;

    if (severity != Severity::None) {
      strm << '[' << severity << ']';
      should_insert_spc = true;
    }

    if (scope != "") {
      strm << '[' << scope << ']';
      should_insert_spc = true;
    }

    strm << (should_insert_spc ? " " : "") << rec[expr::smessage];
  });

  sink->set_filter(expr::attr<Severity>("Severity") >= minSeverity);

  logging::core::get()->add_sink(sink);
  logging::add_common_attributes();
}
