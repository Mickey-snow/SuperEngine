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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "test_utils.h"

#include "base/avspec.h"
#include "systems/sdl/sdl_sound_system.h"
#include "systems/sdl/sound_implementor.h"

#include <filesystem>
#include <memory>
#include <tuple>

class FakeAudioImpl : public SoundSystemImpl {
 public:
  using super = SoundSystemImpl;

  FakeAudioImpl() = default;
  ~FakeAudioImpl() = default;

  mutable bool activate_ = false;
  void InitSystem() const override { activate_ = true; }
  void CheckSystemActivate() const {
    if (!activate_)
      FAIL() << "Using the sound system implementor without init it";
  }
  void QuitSystem() const override { activate_ = false; }

  mutable int frequency_ = -1, channels_ = -1, buffers_ = -1;
  mutable AV_SAMPLE_FMT format_;
  int OpenAudio(int rate,
                AV_SAMPLE_FMT format,
                int channels,
                int buffers) const override {
    CheckSystemActivate();
    frequency_ = rate;
    format_ = format;
    channels_ = channels;
    buffers_ = buffers;
    return 0;
  }
  AVSpec QuerySpec() const override {
    CheckSystemActivate();
    return AVSpec{.sample_rate = frequency_,
                  .sample_format = format_,
                  .channel_count = channels_};
  }

  void CloseAudio() const override { frequency_ = channels_ = buffers_ = -1; }

  mutable std::function<void(int)> channel_finished_callback_ = [](int) {};
  void ChannelFinished(void (*callback)(int)) const override {
    CheckSystemActivate();
    channel_finished_callback_ = callback;
  }

  mutable std::function<void(uint8_t*, int)> music_callback_ = [](auto, auto) {
  };
  void HookMusic(void (*callback)(void*, uint8_t*, int),
                 void* data) const override {
    CheckSystemActivate();
    music_callback_ =
        std::bind(callback, data, std::placeholders::_1, std::placeholders::_2);
  }

  using chunk_handle_t = uint64_t;
  mutable std::map<chunk_handle_t, std::string> chunk_handle_;

  chunk_handle_t MakeHandle() const {
    static chunk_handle_t idn = 1;
    return idn++;
  }
  Chunk_t* LoadWAV(const char* path) const override {
    CheckSystemActivate();

    std::string chunk = path;
    auto id = MakeHandle();
    Chunk_t* chunk_ptr = reinterpret_cast<Chunk_t*>(id);
    chunk_handle_[id] = chunk;
    return chunk_ptr;
  }
  Chunk_t* LoadWAV_RW(char* data, int length) const override {
    CheckSystemActivate();

    std::string chunk(data, length);
    auto id = MakeHandle();
    Chunk_t* chunk_ptr = reinterpret_cast<Chunk_t*>(id);
    chunk_handle_[id] = chunk;
    return chunk_ptr;
  }
  void FreeChunk(Chunk_t* chunk) const override {
    auto id = reinterpret_cast<chunk_handle_t>(chunk);
    if (chunk_handle_.count(id))
      chunk_handle_.erase(id);
    else
      FAIL() << "Cannot free chunk " << id;
  }
  std::string GetChunk(chunk_handle_t id) const {
    if (!chunk_handle_.count(id)) {
      ADD_FAILURE() << "Chunk id " << id << " invalid";
      return {};
    }
    return chunk_handle_.at(id);
  }

  struct Channel_info {
    int volume = 0;
    Chunk_t* playing = nullptr;
    int loops;
  };
  mutable std::vector<Channel_info> channel_;
  void AllocateChannels(const int& num) const override {
    CheckSystemActivate();
    channel_.resize(num);
  }
  void CheckChannel(int channel) const {
    if (channel < 0)
      FAIL() << "Channel " << channel << " invalid";
    if (channel >= channel_.size())
      FAIL() << "Using channel " << channel << " without allocating it";
  }
  void SetVolume(int channel, int vol) const override {
    CheckSystemActivate();
    CheckChannel(channel);
    channel_[channel].volume = vol;
  }
  int Playing(int channel) const override {
    CheckSystemActivate();
    CheckChannel(channel);
    return channel_[channel].playing == nullptr ? 0 : 1;
  }
  int PlayChannel(int channel, Chunk_t* chunk, int loops) const override {
    CheckSystemActivate();
    CheckChannel(channel);
    channel_[channel].loops = loops;
    channel_[channel].playing = chunk;
    return 0;
  }
  int FadeInChannel(int channel,
                    Chunk_t* chunk,
                    int loops,
                    int ms) const override {
    return PlayChannel(channel, chunk, loops);
  }
  void HaltChannel(int channel) const override {
    CheckSystemActivate();
    CheckChannel(channel);
    channel_[channel].playing = nullptr;
  }
  int FadeOutChannel(int channel, int fadetime) const override {
    HaltChannel(channel);
    return 0;
  }

  MOCK_METHOD(void,
              MixAudio,
              (uint8_t * dst, uint8_t* src, int len, int volume),
              (const, override));
  MOCK_METHOD(int, MaxVolumn, (), (const, override));

  const char* GetError() const override { return nullptr; }

  using super::FromSDLSoundFormat;
  using super::ToSDLSoundFormat;
};
using ::testing::EndsWith;
using ::testing::Return;

namespace fs = std::filesystem;

TEST(SDLSound, SoundFormat) {
  // sdl1.2 audio format flags
  constexpr auto AUDIO_S8 = 0x8008;
  constexpr auto AUDIO_U8 = 0x0008;
  constexpr auto AUDIO_S16SYS = 0x8010;

  auto aimpl = std::make_shared<FakeAudioImpl>();
  EXPECT_EQ(aimpl->ToSDLSoundFormat(AV_SAMPLE_FMT::U8), AUDIO_U8);
  EXPECT_EQ(aimpl->ToSDLSoundFormat(AV_SAMPLE_FMT::S8), AUDIO_S8);
  EXPECT_EQ(aimpl->ToSDLSoundFormat(AV_SAMPLE_FMT::S16), AUDIO_S16SYS);
  EXPECT_THROW(aimpl->ToSDLSoundFormat(AV_SAMPLE_FMT::S64),
               std::invalid_argument);
  EXPECT_THROW(aimpl->ToSDLSoundFormat(AV_SAMPLE_FMT::DBL),
               std::invalid_argument);
  EXPECT_THROW(aimpl->ToSDLSoundFormat(AV_SAMPLE_FMT::NONE),
               std::invalid_argument);

  EXPECT_EQ(aimpl->FromSDLSoundFormat(AUDIO_U8), AV_SAMPLE_FMT::U8);
  EXPECT_EQ(aimpl->FromSDLSoundFormat(AUDIO_S8), AV_SAMPLE_FMT::S8);
  EXPECT_EQ(aimpl->FromSDLSoundFormat(AUDIO_S16SYS), AV_SAMPLE_FMT::S16);

  EXPECT_THROW(aimpl->FromSDLSoundFormat(0), std::invalid_argument);
  EXPECT_THROW(aimpl->FromSDLSoundFormat(12345), std::invalid_argument);
}
