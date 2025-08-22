// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2013 Elliot Glaysher
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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
// -----------------------------------------------------------------------

#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

class FrameCounter;
class ParameterManager;
class GraphicsObject;
class RLMachine;
class RenderingService;

struct Mutator {
  using SetFn = std::function<void(ParameterManager&, int)>;
  SetFn setter_;
  std::shared_ptr<FrameCounter> fc_;

  bool Update(ParameterManager& pm) const;
};

class ObjectMutator {
  std::vector<Mutator> mutators_;
  int repr_;
  std::string name_;

 public:
  ObjectMutator(std::vector<Mutator> mut,
                int repr = 0,
                std::string name = "unknown");

  inline void SetRepr(int in) { repr_ = in; }
  inline void SetName(std::string in) { name_.swap(in); }
  inline int repr() const { return repr_; }
  inline const std::string& name() const { return name_; }
  inline bool OperationMatches(int repr, const std::string& name) const {
    return repr_ == repr && name_ == name;
  }

  void OnComplete(DoneFn fn);

  bool operator()(RLMachine& machine, GraphicsObject& go);
  bool operator()(RenderingService& locator, ParameterManager& pm);

  bool Update(ParameterManager& pm);
  void SetToEnd(ParameterManager& pm);
};
