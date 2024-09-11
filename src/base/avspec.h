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

#ifndef SRC_BASE_AVSPEC_H_
#define SRC_BASE_AVSPEC_H_

#include <cstdint>
#include <string>
#include <type_traits>

enum class AV_SAMPLE_FMT {
  NONE,
  U8,   ///< unsigned 8 bits
  S8,   ///< signed 8 bits
  S16,  ///< signed 16 bits
  S32,  ///< signed 32 bits
  S64,  ///< signed 64 bits
  FLT,  ///< float
  DBL,  ///< double
};
using avsample_u8_t = uint8_t;
using avsample_s8_t = int8_t;
using avsample_s16_t = int16_t;
using avsample_s32_t = int32_t;
using avsample_s64_t = int64_t;
using avsample_flt_t = float;
using avsample_dbl_t = double;

template <typename T>
struct av_sample_fmt {
  static constexpr AV_SAMPLE_FMT value = AV_SAMPLE_FMT::NONE;
};
template <>
struct av_sample_fmt<avsample_u8_t> {
  static constexpr AV_SAMPLE_FMT value = AV_SAMPLE_FMT::U8;
};
template <>
struct av_sample_fmt<avsample_s8_t> {
  static constexpr AV_SAMPLE_FMT value = AV_SAMPLE_FMT::S8;
};
template <>
struct av_sample_fmt<avsample_s16_t> {
  static constexpr AV_SAMPLE_FMT value = AV_SAMPLE_FMT::S16;
};
template <>
struct av_sample_fmt<avsample_s32_t> {
  static constexpr AV_SAMPLE_FMT value = AV_SAMPLE_FMT::S32;
};
template <>
struct av_sample_fmt<avsample_s64_t> {
  static constexpr AV_SAMPLE_FMT value = AV_SAMPLE_FMT::S64;
};
template <>
struct av_sample_fmt<avsample_flt_t> {
  static constexpr AV_SAMPLE_FMT value = AV_SAMPLE_FMT::FLT;
};
template <>
struct av_sample_fmt<avsample_dbl_t> {
  static constexpr AV_SAMPLE_FMT value = AV_SAMPLE_FMT::DBL;
};
template <typename T>
constexpr AV_SAMPLE_FMT av_sample_fmt_v = av_sample_fmt<T>::value;

template <AV_SAMPLE_FMT fmt>
struct av_sample_fmt_type {
  using type = void;
};
template <>
struct av_sample_fmt_type<AV_SAMPLE_FMT::U8> {
  using type = avsample_u8_t;
};
template <>
struct av_sample_fmt_type<AV_SAMPLE_FMT::S8> {
  using type = avsample_s8_t;
};
template <>
struct av_sample_fmt_type<AV_SAMPLE_FMT::S16> {
  using type = avsample_s16_t;
};
template <>
struct av_sample_fmt_type<AV_SAMPLE_FMT::S32> {
  using type = avsample_s32_t;
};
template <>
struct av_sample_fmt_type<AV_SAMPLE_FMT::S64> {
  using type = avsample_s64_t;
};
template <>
struct av_sample_fmt_type<AV_SAMPLE_FMT::FLT> {
  using type = avsample_flt_t;
};
template <>
struct av_sample_fmt_type<AV_SAMPLE_FMT::DBL> {
  using type = avsample_dbl_t;
};
template <AV_SAMPLE_FMT fmt>
using av_sample_fmt_t = typename av_sample_fmt_type<fmt>::type;

std::string to_string(AV_SAMPLE_FMT fmt);

size_t Bytecount(AV_SAMPLE_FMT fmt) noexcept;

struct AVSpec {
  int sample_rate;
  AV_SAMPLE_FMT sample_format;
  int channel_count;

  bool operator==(const AVSpec& rhs) const;
  bool operator!=(const AVSpec& rhs) const;
};

#endif
