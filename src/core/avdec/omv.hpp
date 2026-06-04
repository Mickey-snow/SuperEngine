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
// -----------------------------------------------------------------------

#pragma once

#include "core/rect.hpp"

#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <vector>

class OmvDecoder {
 public:
  static constexpr int kTypeRgb = 0;
  static constexpr int kTypeRgba = 1;
  static constexpr int kTypeYuv = 2;

  struct Info {
    int type = 0;
    Size size;
    Point center;
    int frame_count = 0;
    int total_time = 0;
    int usec_per_frame = 0;
  };

  explicit OmvDecoder(std::filesystem::path path, bool loop);
  ~OmvDecoder();

  inline const std::filesystem::path& path() const { return path_; }
  inline const Info& info() const { return info_; }
  inline int width() const { return info_.size.width(); }
  inline int height() const { return info_.size.height(); }
  inline int frame_count() const { return info_.frame_count; }
  inline int total_time() const { return info_.total_time; }

  int TimeToFrame(int now_time, bool set_end);
  bool CheckNeedUpdate(int now_time, int* frame_no, bool force);
  bool DecodeFrame(int frame_no, std::span<char> bgra, bool force);
  bool DecodeTime(int now_time, std::span<char> bgra, bool force);

  void UpdateTimeOnly(int now_time);
  void EndLoop();

  bool IsPlaying() const;
  bool ended() const { return end_of_theora_; }

 private:
  struct Header;
  struct Page;
  struct Packet;
  struct Impl;

  void LoadFile();
  void ParseHeader();
  void InitializeDecoder();
  int ReadPage(int page_no);
  void EmptyStream();
  void CheckReady() const;
  void DecodeFrameInternal(int frame_no, bool force);
  void WriteVideo(std::span<char> bgra);

  std::filesystem::path path_;
  std::vector<unsigned char> data_;
  std::unique_ptr<Header> header_;
  std::vector<Page> pages_;
  std::vector<Packet> packets_;
  std::size_t data_start_ = 0;
  Info info_;

  bool loop_ = false;
  bool end_of_theora_ = false;
  int now_page_no_ = -1;
  int now_frame_no_ = -1;

  std::unique_ptr<Impl> impl_;
};
