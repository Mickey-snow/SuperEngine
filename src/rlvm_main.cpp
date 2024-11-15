// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2006, 2007 Elliot Glaysher
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

// We include this here because SDL is retarded and works by #define
// main(int argc, char* agrv[]). Loosers.
#include <SDL/SDL.h>
// TODO: Clean up platform-specific dependencies (SDL, GTK, etc.) once
// abstractions and implementations are separated properly. Consider
// refactoring for better cross-platform support and modularity.

#include <boost/program_options.hpp>
#include <iostream>
#include <string>

#include "platforms/implementor.hpp"
#include "platforms/platform_factory.hpp"
#include "rlvm_instance.hpp"
#include "systems/base/system.hpp"
#include "utilities/file.hpp"

namespace po = boost::program_options;
namespace fs = std::filesystem;

// -----------------------------------------------------------------------

void printVersionInformation() {
  std::cout
      << "rlvm (" << GetRlvmVersionString() << ")\n"
      << "Copyright (C) 2006-2014 Elliot Glaysher, et all.\n\n"
      << "Contains code that is: \n"
      << "  Copyright (C) 2006-2007 Peter \"Haeleth\" Jolly\n"
      << "  Copyright (C) 2004-2006 Kazunori \"jagarl\" Ueno\n\n"

      << "This program is free software: you can redistribute it and/or modify"
      << '\n'
      << "it under the terms of the GNU General Public License as published by"
      << '\n'
      << "the Free Software Foundation, either version 3 of the License, or"
      << '\n'
      << "(at your option) any later version.\n\n"

      << "This program is distributed in the hope that it will be useful,"
      << '\n'
      << "but WITHOUT ANY WARRANTY; without even the implied warranty of"
      << '\n'
      << "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
      << "GNU General Public License for more details.\n\n"

      << "You should have received a copy of the GNU General Public License"
      << '\n'
      << "along with this program.  If not, see <http://www.gnu.org/licenses/>."
      << std::endl
      << std::endl;
}

void printUsage(const std::string& name, po::options_description& opts) {
  std::cout << "Usage: " << name << " [options] <game root>" << std::endl;
  std::cout << opts << std::endl;
}

int main(int argc, char* argv[]) {
  // -----------------------------------------------------------------------
  // Set up argument parser

  // Declare command line options
  po::options_description opts("Options");
  opts.add_options()("help", "Produce help message")(
      "help-debug", "Print help message for people working on rlvm")(
      "version", "Display version and license information")(
      "font", po::value<std::string>(), "Specifies TrueType font to use.")(
      "platform", po::value<std::string>(),
      "Specifies which gui platform to use.")(
      "show-platforms", "Print all avaliable gui platforms.");

  po::options_description debugOpts("Debugging Options");
  debugOpts.add_options()("start-seen", po::value<int>(),
                          "Force start at SEEN#")(
      "dump-seen", po::value<int>(), "Dumps rlvm's internal parsing of SEEN#")(
      "load-save", po::value<int>(), "Load a saved game on start")(
      "memory", "Forces debug mode (Sets #MEMORY=1 in the Gameexe.ini file)")(
      "undefined-opcodes", "Display a message on undefined opcodes")(
      "count-undefined",
      "On exit, present a summary table about how many times each undefined "
      "opcode was called")("trace", "Prints opcodes as they are run)");

  // Declare the final option to be game-root
  po::options_description hidden("Hidden");
  hidden.add_options()("game-root", po::value<std::string>(),
                       "Location of game root");

  po::positional_options_description p;
  p.add("game-root", -1);

  // Use these on the command line
  po::options_description commandLineOpts;
  commandLineOpts.add(opts).add(hidden).add(debugOpts);

  po::variables_map vm;
  try {
    po::store(po::basic_command_line_parser<char>(argc, argv)
                  .options(commandLineOpts)
                  .positional(p)
                  .run(),
              vm);
    po::notify(vm);
  } catch (boost::program_options::multiple_occurrences& e) {
    std::cerr
        << "Couldn't parse command line: multiple_occurances." << std::endl
        << " (Hint: this can happen when your shell doesn't escape properly,"
        << std::endl
        << "  e.g. \"/path/to/Clannad Full Voice/\" without the quotes.)"
        << std::endl;
    return -1;
  } catch (boost::program_options::error& e) {
    std::cerr << "Couldn't parse command line: " << e.what() << std::endl;
    return -1;
  }

  // -----------------------------------------------------------------------

  po::options_description allOpts("Allowed options");
  allOpts.add(opts).add(debugOpts);

  // -----------------------------------------------------------------------
  // Process command line options
  fs::path gamerootPath;

  if (vm.count("help")) {
    printUsage(argv[0], opts);
    return 0;
  }

  if (vm.count("help-debug")) {
    printUsage(argv[0], allOpts);
    return 0;
  }

  if (vm.count("version")) {
    printVersionInformation();
    return 0;
  }

  if (vm.count("show-platforms")) {
    for (auto begin = PlatformFactory::cbegin(), end = PlatformFactory::cend();
         begin != end; ++begin) {
      std::cout << begin->first << std::endl;
    }
    return 0;
  }

  // This is where we need platform implementor to pop up platform-specific
  // dialogue if we need to ask user for providing the path.
  PlatformImpl_t platform_impl = nullptr;
  if (vm.count("platform"))
    platform_impl = PlatformFactory::Create(vm["platform"].as<std::string>());
  else
    platform_impl = PlatformFactory::Create("default");

  if (!platform_impl) {
    // If everything fails
    std::cerr << "[WARNING] No gui implementation found." << std::endl;
  }

  // -----------------------------------------------------------------------
  // Select game root directory.
  if (vm.count("game-root")) {
    gamerootPath = vm["game-root"].as<std::string>();

    if (!fs::exists(gamerootPath)) {
      std::cerr << "ERROR: Path '" << gamerootPath << "' does not exist."
                << std::endl;
      return -1;
    }

    if (!fs::is_directory(gamerootPath)) {
      std::cerr << "ERROR: Path '" << gamerootPath << "' is not a directory."
                << std::endl;
      return -1;
    }

    // Some games hide data in a lower subdirectory.  A little hack to
    // make these behave as expected...
    if (CorrectPathCase(gamerootPath / "Gameexe.ini").empty()) {
      if (!CorrectPathCase(gamerootPath / "KINETICDATA" / "Gameexe.ini")
               .empty()) {
        gamerootPath /= "KINETICDATA/";
      } else if (!CorrectPathCase(gamerootPath / "REALLIVEDATA" / "Gameexe.ini")
                      .empty()) {
        gamerootPath /= "REALLIVEDATA/";
      } else {
        std::cerr << "WARNING: Path '" << gamerootPath << "' may not contain a "
                  << "RealLive game." << std::endl;
      }
    }
  } else {
    gamerootPath = platform_impl->SelectGameDirectory();
    if (gamerootPath.empty())
      return -1;
  }

  // -----------------------------------------------------------------------
  // Create game instance

  RLVMInstance instance;
  instance.SetPlatformImplementor(platform_impl);

  if (vm.count("start-seen"))
    instance.set_seen_start(vm["start-seen"].as<int>());

  if (vm.count("dump-seen"))
    instance.set_dump_seen(vm["dump-seen"].as<int>());

  if (vm.count("memory"))
    instance.set_memory();

  if (vm.count("undefined-opcodes"))
    instance.set_undefined_opcodes();

  if (vm.count("count-undefined"))
    instance.set_count_undefined();

  if (vm.count("trace"))
    instance.set_tracing();

  if (vm.count("load-save"))
    instance.set_load_save(vm["load-save"].as<int>());

  if (vm.count("font"))
    instance.set_custom_font(vm["font"].as<std::string>());

  instance.Run(gamerootPath);

  return 0;
}
