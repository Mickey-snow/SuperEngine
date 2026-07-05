// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2026 Serina Sakurai
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

#include "core/avdec/iadec.hpp"
#include "core/rect.hpp"

#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>

class FfmpegVideoDecoder {
 public:
  struct Info {
    Size size;
    int total_time = 0;
    int usec_per_frame = 0;
  };

  explicit FfmpegVideoDecoder(std::filesystem::path path);
  ~FfmpegVideoDecoder();

  const Info& info() const;
  int width() const;
  int height() const;
  int total_time() const;

  bool DecodeTime(int time_ms, std::span<char> bgra, bool force = false);
  bool IsPlaying(int time_ms) const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

class FfmpegAudioDecoder : public IAudioDecoder {
 public:
  explicit FfmpegAudioDecoder(std::string_view data);
  ~FfmpegAudioDecoder() override;

  std::string DecoderName() const override;
  AVSpec GetSpec() override;
  AudioData DecodeAll() override;
  AudioData DecodeNext() override;
  bool HasNext() override;
  SEEK_RESULT Seek(pcm_count_t offset,
                   SEEKDIR whence = SEEKDIR::CUR) override;
  pcm_count_t Tell() override;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};
