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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using ::testing::_;
using ::testing::Return;

#include "base/audio_player.h"
#include "base/avdec/audio_decoder.h"
#include "utilities/numbers.h"

constexpr auto sample_rate = 44100;
constexpr auto frequency = 440;
constexpr auto duration = 0.2;
constexpr auto channel_count = 2;

class TuningFork : public IAudioDecoder {
 public:
  TuningFork()
      : position_(0),
        sample_rate_(sample_rate),
        frequency_(440.0),
        total_samples_(sample_rate_ * duration * channel_count) {
    double two_pi_f = 2.0 * pi * frequency_;

    for (int i = 0; buffer_.size() < total_samples_; ++i) {
      double sample_value =
          std::sin(two_pi_f * (static_cast<double>(i) / sample_rate_));
      for (int j = 0; j < channel_count; ++j)
        buffer_.push_back(static_cast<avsample_flt_t>(sample_value));
    }
  }

  std::string DecoderName() const override { return "#A4 Tuning Fork"; }

  AVSpec GetSpec() override {
    return {.sample_rate = sample_rate_,
            .sample_format = AV_SAMPLE_FMT::FLT,
            .channel_count = channel_count};
  }

  AudioData DecodeAll() override { return AudioData{GetSpec(), buffer_}; }

  AudioData DecodeNext() override {
    static constexpr int chunk_size = 1024;
    int end_position = std::min(position_ + chunk_size, total_samples_);

    std::vector<avsample_flt_t> chunk(buffer_.begin() + position_,
                                      buffer_.begin() + end_position);
    position_ = end_position;

    return AudioData{GetSpec(), std::move(chunk)};
  }

  bool HasNext() override { return position_ < total_samples_; }

  SEEK_RESULT Seek(pcm_count_t offset, SEEKDIR whence = SEEKDIR::CUR) override {
    if (whence == SEEKDIR::BEG) {
      if (offset >= 0 && offset < total_samples_) {
        position_ = offset;
        return SEEK_RESULT::PRECISE_SEEK;
      }
    } else if (whence == SEEKDIR::CUR) {
      pcm_count_t new_pos = position_ + offset;
      if (new_pos >= 0 && new_pos < total_samples_) {
        position_ = new_pos;
        return SEEK_RESULT::PRECISE_SEEK;
      }
    } else if (whence == SEEKDIR::END) {
      pcm_count_t new_pos = total_samples_ + offset;
      if (new_pos >= 0 && new_pos < total_samples_) {
        position_ = new_pos;
        return SEEK_RESULT::PRECISE_SEEK;
      }
    }
    return SEEK_RESULT::ERROR;
  }

  pcm_count_t Tell() override { return position_ / channel_count; }

  std::vector<avsample_flt_t> buffer_;
  pcm_count_t position_;
  const int sample_rate_;
  const double frequency_;
  const pcm_count_t total_samples_;
};

class AudioPlayerTest : public ::testing::Test {
 protected:
  void SetUp() override {
    decoder = std::make_shared<TuningFork>();
    player = std::make_unique<AudioPlayer>(AudioDecoder(decoder));
  }

  std::shared_ptr<TuningFork> decoder;
  std::unique_ptr<AudioPlayer> player;

  static double Deviation(const std::vector<float>& a,
                          const std::vector<float>& b) {
    double var = 0;
    auto sqr = [](double x) { return x * x; };
    auto n = std::min(a.size(), b.size());
    for (size_t i = 0; i < n; ++i)
      var += sqr(a[i] - b[i]) / n;
    return std::sqrt(var);
  }

  static long long GetTicks(double time_s) {
    return static_cast<long long>(time_s * 1000);
  }
};

TEST_F(AudioPlayerTest, LoadPCM) {
  const int samples_quarter = sample_rate * channel_count * duration / 4;

  for (int i = 0; i < 4; ++i) {
    auto result = player->LoadPCM(samples_quarter);
    ASSERT_EQ(result.SampleCount(), samples_quarter);
    EXPECT_EQ(player->GetCurrentTime(), GetTicks(duration * (i + 1) / 4.0));

    auto expect = std::vector<float>(
        decoder->buffer_.begin() + i * samples_quarter,
        decoder->buffer_.begin() + (i + 1) * samples_quarter);
    auto actual = std::get<std::vector<float>>(result.data);
    EXPECT_LE(Deviation(expect, actual), 1e-4);
  }

  EXPECT_FALSE(player->IsLoopingEnabled());
  EXPECT_FALSE(player->IsPlaybackActive());

  auto result = player->LoadPCM(samples_quarter * 10);
  EXPECT_LE(Deviation(std::get<std::vector<float>>(result.data),
                      std::vector<float>(samples_quarter * 10)),
            1e-4);
}

TEST_F(AudioPlayerTest, LoopPlayback) {
  player->SetLooping(true);
  ASSERT_TRUE(player->IsLoopingEnabled());

  const auto tot_samples = sample_rate * duration * channel_count;
  auto result = player->LoadPCM(tot_samples * 1.5);
  EXPECT_EQ(player->GetCurrentTime(), GetTicks(duration * 0.5));

  EXPECT_TRUE(player->IsPlaybackActive());
  result.Concat(player->LoadPCM(tot_samples));
  EXPECT_EQ(player->GetCurrentTime(), GetTicks(duration * 0.5));

  player->SetLooping(false);
  result.Concat(player->LoadPCM(tot_samples * 0.5));
  EXPECT_FALSE(player->IsPlaybackActive());
  EXPECT_EQ(player->GetCurrentTime(), GetTicks(duration));

  auto expect = decoder->buffer_;
  expect.insert(expect.end(), decoder->buffer_.cbegin(),
                decoder->buffer_.cend());
  expect.insert(expect.end(), decoder->buffer_.cbegin(),
                decoder->buffer_.cend());
  EXPECT_EQ(expect.size(), result.SampleCount());
  EXPECT_LE(Deviation(std::get<std::vector<float>>(result.data), expect), 1e-4);
}
