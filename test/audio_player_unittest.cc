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
  EXPECT_FALSE(player->IsPlaying());

  auto result = player->LoadPCM(samples_quarter * 10);
  EXPECT_LE(Deviation(std::get<std::vector<float>>(result.data),
                      std::vector<float>(samples_quarter * 10)),
            1e-4);
}

TEST_F(AudioPlayerTest, LoadInvalid) {
  EXPECT_THROW(player->LoadPCM(0), std::invalid_argument);
  EXPECT_THROW(player->LoadPCM(-123), std::invalid_argument);
}

TEST_F(AudioPlayerTest, LoadAll) {
  const int samples = sample_rate * channel_count * duration;
  auto result = player->LoadPCM(samples - 3);
  EXPECT_NO_THROW(result.Append(player->LoadRemain()));
  EXPECT_EQ(result.SampleCount(), decoder->buffer_.size());
  EXPECT_LE(
      Deviation(std::get<std::vector<float>>(result.data), decoder->buffer_),
      1e-4);
}

TEST_F(AudioPlayerTest, LoopPlayback) {
  player->SetLooping(true);
  ASSERT_TRUE(player->IsLoopingEnabled());

  const auto tot_samples = sample_rate * duration * channel_count;
  auto result = player->LoadPCM(tot_samples * 1.5);
  EXPECT_EQ(player->GetCurrentTime(), GetTicks(duration * 0.5));

  EXPECT_TRUE(player->IsPlaying());
  result.Append(player->LoadPCM(tot_samples));
  EXPECT_EQ(player->GetCurrentTime(), GetTicks(duration * 0.5));

  player->SetLooping(false);
  result.Append(player->LoadPCM(tot_samples * 0.5));
  EXPECT_FALSE(player->IsPlaying());
  EXPECT_EQ(player->GetCurrentTime(), GetTicks(duration));

  auto expect = decoder->buffer_;
  expect.insert(expect.end(), decoder->buffer_.cbegin(),
                decoder->buffer_.cend());
  expect.insert(expect.end(), decoder->buffer_.cbegin(),
                decoder->buffer_.cend());
  EXPECT_EQ(expect.size(), result.SampleCount());
  EXPECT_LE(Deviation(std::get<std::vector<float>>(result.data), expect), 1e-4);
}

TEST_F(AudioPlayerTest, Fadein) {
  const auto tot_samples = sample_rate * duration * channel_count;
  const float fadein_ms = duration * 1000 / 2.0;
  player->FadeIn(fadein_ms);

  auto result = player->LoadPCM(tot_samples);
  auto first =
      std::get<std::vector<float>>(result.Slice(0, tot_samples / 2).data);
  auto second = std::get<std::vector<float>>(
      result.Slice(tot_samples / 2, tot_samples).data);

  auto actual = std::vector<float>(decoder->buffer_.begin(),
                                   decoder->buffer_.begin() + tot_samples / 2);
  float last_volume = -0.01, current_volume = 0;
  ASSERT_EQ(actual.size(), first.size());
  for (size_t i = 0; i < actual.size(); ++i) {
    if (fabs(actual[i]) < 1e-5) {  // avoid divide by zero
      EXPECT_LT(fabs(first[i]), 1e-5);
      continue;
    }

    current_volume = first[i] / actual[i];
    EXPECT_GT(current_volume, last_volume);
    last_volume = current_volume;
  }

  actual = std::vector<float>(decoder->buffer_.begin() + tot_samples / 2,
                              decoder->buffer_.end());
  EXPECT_LE(Deviation(actual, second), 1e-4);
}

TEST_F(AudioPlayerTest, Fadeout) {
  const float fadeout_ms = duration * 1000 / 2.0;
  player->FadeOut(fadeout_ms);
  EXPECT_TRUE(player->IsPlaying());

  const auto fadeout_samples = fadeout_ms * sample_rate * channel_count / 1000;
  auto result =
      std::get<std::vector<float>>(player->LoadPCM(fadeout_samples).data);
  EXPECT_FALSE(player->IsPlaying());

  float last_volume = 1.01, current_volume = 0;
  for (size_t i = 0; i < result.size(); ++i) {
    const auto& actual = decoder->buffer_;
    if (fabs(actual[i]) < 1e-5) {
      EXPECT_LT(fabs(result[i]), 1e-5);
      continue;
    }

    current_volume = result[i] / actual[i];
    EXPECT_LT(current_volume, last_volume);
    last_volume = current_volume;
  }
}

TEST_F(AudioPlayerTest, LoopingRewind) {
  player->SetLooping(true);
  ASSERT_TRUE(player->IsLoopingEnabled());

  const auto tot_samples = sample_rate * duration * channel_count;

  auto result = player->LoadPCM(tot_samples * 2);

  EXPECT_EQ(player->GetCurrentTime(), GetTicks(0.0));
  EXPECT_TRUE(player->IsPlaying());
}

TEST_F(AudioPlayerTest, TerminateLoop) {
  const auto tot_samples = sample_rate * duration * channel_count;
  player->SetLooping(true);
  auto result = player->LoadPCM(tot_samples * 2 - 5);
  EXPECT_TRUE(player->IsPlaying());

  player->SetLooping(false);
  result.Append(player->LoadRemain());
  EXPECT_EQ(player->LoadRemain().SampleCount(), 0);

  EXPECT_EQ(result.SampleCount(), tot_samples * 2);
  auto expect = decoder->buffer_;
  expect.insert(expect.end(), decoder->buffer_.begin(), decoder->buffer_.end());

  EXPECT_LE(Deviation(std::get<std::vector<float>>(result.data), expect), 1e-4);
  EXPECT_FALSE(player->IsPlaying());
}

TEST_F(AudioPlayerTest, StartTerminated) {
  const auto tot_samples = sample_rate * duration * channel_count;

  EXPECT_TRUE(player->IsPlaying());

  player->Terminate();
  EXPECT_FALSE(player->IsPlaying());

  auto result = std::get<std::vector<float>>(player->LoadPCM(tot_samples).data);
  EXPECT_EQ(result.size(), tot_samples);
  EXPECT_LE(Deviation(result, std::vector<float>(tot_samples)), 1e-4);
  EXPECT_FALSE(player->IsPlaying());
}
