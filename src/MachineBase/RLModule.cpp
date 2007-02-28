// This file is part of RLVM, a RealLive virutal machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2006 El Riot
//  
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//  
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//  
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//  
// -----------------------------------------------------------------------

/**
 * @file   RLModule.cpp
 * @author Elliot Glaysher
 * @date   Sat Oct  7 11:14:14 2006
 * 
 * @brief  Definition of RLModule
 */


#include "MachineBase/RLModule.hpp"
#include "MachineBase/RLOperation.hpp"

#include <sstream>

using namespace std;
using namespace libReallive;

RLModule::RLModule(const std::string& inModuleName, int inModuleType, 
                   int inModuleNumber)
  : m_moduleType(inModuleType), m_moduleNumber(inModuleNumber), 
    m_moduleName(inModuleName) 
{}

// -----------------------------------------------------------------------

RLModule::~RLModule()
{}

// -----------------------------------------------------------------------

int RLModule::packOpcodeNumber(int opcode, unsigned char overload)
{
  return ((int)opcode << 8) | overload;
}

// -----------------------------------------------------------------------

void RLModule::unpackOpcodeNumber(int packedOpcode, int& opcode, unsigned char& overload)
{
  opcode = (packedOpcode >> 8);
  overload = packedOpcode & 0xFF;
}

// -----------------------------------------------------------------------

void RLModule::addOpcode(int opcode, unsigned char overload, RLOperation* op) 
{
  int packedOpcode = packOpcodeNumber(opcode, overload);
  storedOperations.insert(packedOpcode, op);
}

// -----------------------------------------------------------------------

void RLModule::dispatchFunction(RLMachine& machine, const CommandElement& f) 
{
  OpcodeMap::iterator it = storedOperations.find(packOpcodeNumber(f.opcode(), f.overload()));
  if(it != storedOperations.end()) {
    it->dispatchFunction(machine, f);
  } else {
    ostringstream ss;
    ss << "Undefined opcode<" << f.modtype() << ":" << f.module() << ":" 
       << f.opcode() << ", " << f.overload() << ">";
    throw Error(ss.str());
  }
}

// -----------------------------------------------------------------------

std::ostream& operator<<(std::ostream& os, const RLModule& module)
{
  os << "mod<" << module.moduleName() << "," << module.moduleType() 
     << ":" << module.moduleNumber() << ">";
  return os;
}