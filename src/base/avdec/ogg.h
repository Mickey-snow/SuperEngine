// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2009 Elliot Glaysher
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

#ifndef SRC_BASE_AVDEC_OGG_H_
#define SRC_BASE_AVDEC_OGG_H_

#include "base/avdec/iadec.h"
#include "base/avspec.h"

#include <memory>

class ov_adapter;

class OggDecoder : public IAudioDecoder {
 public:
  explicit OggDecoder(std::string_view sv);
  ~OggDecoder();

  OggDecoder(const OggDecoder&) = delete;
  OggDecoder& operator=(const OggDecoder&) = delete;

  std::string DecoderName() const override;

  AVSpec GetSpec() override;

  AudioData DecodeAll() override;

  AudioData DecodeNext() override;

  bool HasNext() override { return false; }

 private:
  std::unique_ptr<ov_adapter> impl_;
};

#endif
