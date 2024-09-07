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

#ifndef BASE_AVDEC_AUDIO_DECODER_H_
#define BASE_AVDEC_AUDIO_DECODER_H_

#include "base/avdec/iadec.h"
#include "base/avdec/nwa.h"
#include "base/avdec/ogg.h"
#include "base/avdec/wav.h"
#include "base/avspec.h"
#include "utilities/mapped_file.h"

#include <algorithm>
#include <any>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

using decoder_t = std::shared_ptr<IAudioDecoder>;

class ADecoderFactory {
 public:
  using decoder_constructor_t = std::function<decoder_t(std::string_view)>;

  ADecoderFactory() : decoder_map_(&default_decoder_map_) {}

  decoder_t Create(std::string_view data,
                   std::optional<std::string> format_hint = {}) {
    using std::string_literals::operator""s;
    std::string format = format_hint.value_or("unknown"s);

    try {
      const auto& fn = decoder_map_->at(format);
      return fn(data);
    } catch (...) {
      // format wrong or unknown, fallback to a chain of responsibility factory
    }

    for (const auto& [key, fn] : *decoder_map_) {
      try {
        return fn(data);
      } catch (...) {
      }
    }

    throw std::runtime_error("No Decoder found for format: " +
                             std::string(format));
  }

 protected:
  const std::unordered_map<std::string, decoder_constructor_t>* decoder_map_;

 private:
  static std::unordered_map<std::string, decoder_constructor_t>
      default_decoder_map_;
};

class AudioDecoder {
 public:
  AudioDecoder(FilePos fp, const std::string& format)
      : dataholder_(fp), decoderimpl_(factory_.Create(fp.Read(), format)) {}

  AudioDecoder(std::shared_ptr<IAudioDecoder> dec)
      : dataholder_(), decoderimpl_(dec) {}

  AudioDecoder(std::filesystem::path filepath, const std::string& format = "") {
    auto file = std::make_shared<MappedFile>(filepath);
    dataholder_ = file;
    decoderimpl_ = factory_.Create(file->Read(), format);
  }

  AudioDecoder(std::string filestr, const std::string& format = "")
      : AudioDecoder(std::filesystem::path(filestr), format) {}

  AudioData DecodeAll() { return decoderimpl_->DecodeAll(); }

  AudioData DecodeNext() { return decoderimpl_->DecodeNext(); }

  bool HasNext() const { return decoderimpl_->HasNext(); }

  void Rewind() const { decoderimpl_->Seek(0, SEEKDIR::BEG); }

  AVSpec GetSpec() const { return decoderimpl_->GetSpec(); }

 private:
  std::any dataholder_; /* if we are responsible for memory management, the
                           container class is being held here */
  decoder_t decoderimpl_;

  static ADecoderFactory factory_;
};

#endif
