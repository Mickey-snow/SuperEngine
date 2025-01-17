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

#pragma once

#include "core/audio_data.hpp"

#include <string>

enum class SEEK_RESULT { ERROR = 1, FAIL, IMPRECISE_SEEK, PRECISE_SEEK };

enum class SEEKDIR { BEG = 1, END, CUR };

class IAudioDecoder {
 public:
  virtual ~IAudioDecoder() = default;

  virtual std::string DecoderName() const = 0;

  virtual AVSpec GetSpec() = 0;

  virtual AudioData DecodeAll() = 0;

  virtual AudioData DecodeNext() = 0;

  virtual bool HasNext() = 0;

  using pcm_count_t = long long;
  virtual SEEK_RESULT Seek(pcm_count_t offset, SEEKDIR whence = SEEKDIR::CUR);

  virtual pcm_count_t Tell();
};
