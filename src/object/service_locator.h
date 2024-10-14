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
// -----------------------------------------------------------------------

#ifndef SRC_OBJECT_SERVICE_LOCATOR_H_
#define SRC_OBJECT_SERVICE_LOCATOR_H_

class IMutatorService {
 public:
  virtual ~IMutatorService() = default;

  virtual unsigned int GetTicks() const = 0;
  virtual void MarkObjStateDirty() const = 0;
};

class RLMachine;
class MutatorService : public IMutatorService {
 public:
  MutatorService(RLMachine&);
  virtual ~MutatorService() = default;

  virtual unsigned int GetTicks() const override;
  virtual void MarkObjStateDirty() const override;

 private:
  RLMachine& machine_;
};

#endif
