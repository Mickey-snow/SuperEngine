// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2024 Serina Sakurai
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

#include "log/tracer.hpp"

#include "libreallive/elements/command.hpp"
#include "libreallive/elements/expression.hpp"
#include "libreallive/visitors.hpp"
#include "machine/module_manager.hpp"
#include "machine/rloperation.hpp"
#include "memory/location.hpp"

#include <boost/format.hpp>
#include <boost/log/attributes/scoped_attribute.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/console.hpp>
#include <boost/log/utility/setup/file.hpp>

struct Tracer::Tracer_ctx {
  boost::log::sources::logger logger_;
};

Tracer::Tracer() : ctx_(std::make_unique<Tracer_ctx>()) {
  namespace logging = boost::log;
  namespace keywords = boost::log::keywords;
  namespace expr = boost::log::expressions;

  logging::add_console_log(
      std::clog, keywords::format =
                     (expr::stream << "[" << expr::attr<std::string>("Scene")
                                   << ':' << expr::attr<std::string>("Line")
                                   << "]: " << expr::smessage));

  logging::add_file_log(
      keywords::file_name = "log.txt",
      keywords::format =
          (expr::stream << "[" << expr::attr<std::string>("Scene") << ':'
                        << expr::attr<std::string>("Line")
                        << "]: " << expr::smessage));

  // logging::add_common_attributes();
}

Tracer::~Tracer() = default;

void Tracer::Log(int scene,
                 int line,
                 RLOperation* op,
                 const libreallive::CommandElement& f) {
  BOOST_LOG_SCOPED_THREAD_TAG("Scene", (boost::format("%04d") % scene).str());
  BOOST_LOG_SCOPED_THREAD_TAG("Line", (boost::format("%04d") % line).str());

  IModuleManager& manager = ModuleManager::GetInstance();
  BOOST_LOG(ctx_->logger_) << std::visit(
      libreallive::DebugStringVisitor(&manager), f.DownCast());
}

void Tracer::Log(int scene, int line, const libreallive::ExpressionElement& f) {
  BOOST_LOG_SCOPED_THREAD_TAG("Scene", (boost::format("%04d") % scene).str());
  BOOST_LOG_SCOPED_THREAD_TAG("Line", (boost::format("%04d") % line).str());

  BOOST_LOG(ctx_->logger_) << f.GetSourceRepresentation();
}
