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

#ifndef __LongOperation_hpp__
#define __LongOperation_hpp__

class RLMachine;

/** 
 * A LongOperation is a non-trivial command that requires multiple
 * passes through the game loop to complete.
 *
 * For example, pause(). The pause() RLOperation sets the pause()
 * LongOperation, which is executed instead of normal
 * interpretation. The pause() LongOperation checks for any input from
 * the user (ctrl or mouse click), returning true when it detects it,
 * telling the RLMachine to delete the current LongOperation and
 * resume normal operations.
 */
class LongOperation 
{
public:
  virtual ~LongOperation();

  /** 
   * Executes the current LongOperation.
   * 
   * @return Returns true if the command has completed, and normal
   * interpretation should be resumed. Returns false if the command is
   * still running.
   */
  virtual bool operator()(RLMachine& machine) = 0;
};

#endif