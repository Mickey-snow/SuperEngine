// -*- Mode: C++; tab-width:2; indent-tabs-mode: nil; c-basic-offset: 2 -*-
// vi:tw=80:et:ts=2:sts=2
//
// -----------------------------------------------------------------------
//
// This file is part of RLVM, a RealLive virtual machine clone.
//
// -----------------------------------------------------------------------
//
// Copyright (C) 2024 Serina Sakurai
// Copyright (C) 2008 Elliot Glaysher
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

#ifndef SRC_BASE_TONE_CURVE_H_
#define SRC_BASE_TONE_CURVE_H_

#include <array>
#include <string_view>
#include <vector>

class Gameexe;

/// @file ToneCurve.h
/// @brief Header file for the ToneCurve class and related types.

/// @brief Represents a mapping from original color values to tone-curved values
/// for one color channel.
using ToneCurveColorMap = std::array<unsigned char, 256>;

/// @brief Represents the tone curve mappings for RGB channels.
using ToneCurveRGBMap = std::array<ToneCurveColorMap, 3>;

/**
 * @class ToneCurve
 * @brief Manages tone curve effects for image processing.
 *
 * The `ToneCurve` class handles the loading and application of tone curve
 * effects from TCC (Tone Curve Control) files. A TCC file contains mappings
 * between original R, G, and B color values and their corresponding values
 * after a tone curve effect is applied.
 *
 * This class provides methods to load the TCC data and access individual tone
 * curve effects, which can then be applied to image data to adjust colors
 * according to the specified tone curves.
 *
 * Example usage:
 * @code
 * // Instantiate a ToneCurve object using the game configuration.
 * ToneCurve tone_curve = CreateToneCurve(gameexe);
 *
 * // Get the third tone curve effect (index 2).
 * ToneCurveRGBMap tcc_effect = tone_curve.GetEffect(2);
 *
 * // Apply the tone curve effect to a green value of 200.
 * uint8_t original_green = 200;
 * uint8_t adjusted_green = tcc_effect[1][original_green];
 * @endcode
 */
class ToneCurve {
 public:
  /**
   * @brief Initializes an empty tone curve set.
   *
   * Use this constructor for games that do not use tone curve effects.
   */
  ToneCurve();

  /**
   * @brief Constructs a ToneCurve object from TCC file data.
   * @param data The content of the .tcc file.
   */
  ToneCurve(std::string_view data);

  /**
   * @brief Destructor for the ToneCurve class.
   */
  ~ToneCurve();

  /**
   * @brief Gets the total number of tone curve effects available.
   * @return The number of tone curve effects.
   */
  int GetEffectCount() const;

  /**
   * @brief Retrieves the tone curve effect at the specified index.
   *
   * The effects are indexed sequentially from 0 to `GetEffectCount() - 1`.
   *
   * @param index The zero-based index of the desired tone curve effect.
   * @return The tone curve mapping (RGB) for the specified effect.
   * @throws std::out_of_range If the index is out of bounds.
   */
  ToneCurveRGBMap GetEffect(int index) const;

 private:
  /**
   * @brief Parses a tone curve effect from raw data.
   *
   * This method is used internally to parse individual tone curve effects from
   * the TCC file data.
   *
   * @param data The raw data representing a single tone curve effect.
   * @return A `ToneCurveRGBMap` containing the tone curve mappings for R, G,
   * and B channels.
   * @throws std::runtime_error If the data is invalid or corrupted.
   */
  static ToneCurveRGBMap ParseEffect(std::string_view data);

  /// @brief Collection of tone curve effects loaded from the TCC file.
  std::vector<ToneCurveRGBMap> tcc_info_;
};  // end of class ToneCurve

/**
 * @brief Creates a `ToneCurve` object using the specified game configuration.
 *
 * This helper function initializes a `ToneCurve` object by loading the TCC data
 * file specified in the `#TONECURVE_FILENAME` key of the `gameexe`
 * configuration.
 *
 * @param gameexe Reference to the game configuration object (`Gameexe`).
 * @return A `ToneCurve` object initialized with the TCC data.
 */
ToneCurve CreateToneCurve(Gameexe& gameexe);

#endif  // SRC_SYSTEMS_BASE_TONE_CURVE_H_
