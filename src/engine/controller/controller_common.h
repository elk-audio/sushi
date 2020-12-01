/*
 * Copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI.  If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Container for types and functions common to several sub-controllers
 * @copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_CONTROLLER_COMMON_H
#define SUSHI_CONTROLLER_COMMON_H

#include "control_interface.h"

namespace sushi {
namespace engine {

/* Convenience conversion functions between external and internal
 * enums and data structs */
inline ext::PlayingMode to_external(const sushi::PlayingMode mode)
{
    switch (mode)
    {
        case PlayingMode::STOPPED:      return ext::PlayingMode::STOPPED;
        case PlayingMode::PLAYING:      return ext::PlayingMode::PLAYING;
        case PlayingMode::RECORDING:    return ext::PlayingMode::RECORDING;
        default:                        return ext::PlayingMode::PLAYING;
    }
}

inline sushi::PlayingMode to_internal(const ext::PlayingMode mode)
{
    switch (mode)
    {
        case ext::PlayingMode::STOPPED:   return sushi::PlayingMode::STOPPED;
        case ext::PlayingMode::PLAYING:   return sushi::PlayingMode::PLAYING;
        case ext::PlayingMode::RECORDING: return sushi::PlayingMode::RECORDING;
        default:                          return sushi::PlayingMode::PLAYING;
    }
}

inline ext::SyncMode to_external(const sushi::SyncMode mode)
{
    switch (mode)
    {
        case SyncMode::INTERNAL:     return ext::SyncMode::INTERNAL;
        case SyncMode::MIDI:         return ext::SyncMode::MIDI;
        case SyncMode::GATE_INPUT:   return ext::SyncMode::GATE;
        case SyncMode::ABLETON_LINK: return ext::SyncMode::LINK;
        default:                     return ext::SyncMode::INTERNAL;
    }
}

inline sushi::SyncMode to_internal(const ext::SyncMode mode)
{
    switch (mode)
    {
        case ext::SyncMode::INTERNAL: return sushi::SyncMode::INTERNAL;
        case ext::SyncMode::MIDI:     return sushi::SyncMode::MIDI;
        case ext::SyncMode::GATE:     return sushi::SyncMode::GATE_INPUT;
        case ext::SyncMode::LINK:     return sushi::SyncMode::ABLETON_LINK;
        default:                      return sushi::SyncMode::INTERNAL;
    }
}

inline ext::TimeSignature to_external(sushi::TimeSignature internal)
{
    return {internal.numerator, internal.denominator};
}

inline sushi::TimeSignature to_internal(ext::TimeSignature ext)
{
    return {ext.numerator, ext.denominator};
}

} // namespace engine
} // namespace sushi

#endif //SUSHI_CONTROLLER_COMMON_H
