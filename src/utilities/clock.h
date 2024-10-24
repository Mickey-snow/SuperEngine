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

#ifndef SRC_UTILITIES_CLOCK_H_
#define SRC_UTILITIES_CLOCK_H_

#include <chrono>

/**
 * @class Clock
 * @brief A class for tracking the elapsed time since the program started.
 *
 * The Clock class provides functionality to retrieve the current time point
 * and the number of milliseconds elapsed since the program began. It is designed
 * to allow subclassing for testing purposes and is not strictly a singleton.
 */
class Clock {
public:
    /**
     * @typedef duration_t
     * @brief Represents the duration type in milliseconds.
     */
    using duration_t = std::chrono::milliseconds;

    /**
     * @typedef timepoint_t
     * @brief Represents a time point using a steady clock.
     */
    using timepoint_t = std::chrono::time_point<std::chrono::steady_clock>;

public:
    Clock() = default;
    virtual ~Clock() = default;

    /**
     * @brief Retrieves the current time point since the program started.
     *
     * @return A timepoint_t object representing the current time.
     */
    virtual timepoint_t GetTime() const;

    /**
     * @brief Retrieves the number of milliseconds elapsed since the program started.
     *
     * @return A duration_t object representing the elapsed time in milliseconds.
     */
    virtual duration_t GetTicks() const;

private:
    /**
     * @brief The epoch time point marking the start of the program.
     *
     * This static member holds the initial time point from which elapsed time
     * is calculated.
     */
    static timepoint_t epoch;
};

#endif
