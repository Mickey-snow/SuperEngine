// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2013 Elliot Glaysher
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
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
// -----------------------------------------------------------------------

#pragma once

#include <memory>
#include <string_view>

class glslProgram {
 public:
  glslProgram(std::string_view vertex_src, std::string_view frag_src);
  ~glslProgram();

  auto GetID() const { return id_; }
  unsigned int UniformLocation(std::string_view name);
  void SetUniform(std::string_view name, int value);
  void SetUniform(std::string_view name, float value);
  void SetUniform(std::string_view name, float x, float y, float z, float w);
  void SetUniform(std::string_view name, float x, float y, float z);

 private:
  unsigned int id_;
};

std::shared_ptr<glslProgram> GetOpShader();
std::shared_ptr<glslProgram> GetColorMaskShader();
std::shared_ptr<glslProgram> GetObjectShader();
