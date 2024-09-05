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

#include <any>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

class AudioDecoder {
 public:
  AudioDecoder(FilePos fp, const std::string& format)
      : dataholder_(fp), decoderimpl_(nullptr) {
    if (format == "nwa")
      decoderimpl_ = std::make_shared<NwaDecoder>(fp.Read());
    else if (format == "ogg")
      decoderimpl_ = std::make_shared<OggDecoder>(fp.Read());

    else
      throw std::logic_error("No valid decoder found.");
  }

  AVSpec GetSpec() { return decoderimpl_->GetSpec(); }

  AudioData DecodeAll() { return decoderimpl_->DecodeAll(); }

  AudioData DecodeNext() { return decoderimpl_->DecodeNext(); }

  /* exposed for testing */
  std::shared_ptr<IAudioDecoder> GetDecoder() const { return decoderimpl_; }

  void SetDecoder(std::shared_ptr<IAudioDecoder> impl) { decoderimpl_ = impl; }

 private:
  std::any dataholder_; /* if we are responsible for memory management, the
                           container class is being held here */
  std::shared_ptr<IAudioDecoder> decoderimpl_;
};

#endif
