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
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_CONTROLLER_COMMON_H
#define SUSHI_CONTROLLER_COMMON_H

#include "control_interface.h"
#include "library/base_performance_timer.h"

namespace sushi {
namespace engine {
namespace controller_impl {

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

inline ext::CpuTimings to_external(const sushi::performance::ProcessTimings& timings)
{
    return {.avg = timings.avg_case,
            .min = timings.min_case,
            .max = timings.max_case};
}

inline ext::TimeSignature to_external(sushi::TimeSignature internal)
{
    return {internal.numerator, internal.denominator};
}

inline sushi::TimeSignature to_internal(ext::TimeSignature ext)
{
    return {ext.numerator, ext.denominator};
}

inline ext::MidiChannel to_external_midi_channel(int channel_int)
{
    switch (channel_int)
    {
        case 0:  return sushi::ext::MidiChannel::MIDI_CH_1;
        case 1:  return sushi::ext::MidiChannel::MIDI_CH_2;
        case 2:  return sushi::ext::MidiChannel::MIDI_CH_3;
        case 3:  return sushi::ext::MidiChannel::MIDI_CH_4;
        case 4:  return sushi::ext::MidiChannel::MIDI_CH_5;
        case 5:  return sushi::ext::MidiChannel::MIDI_CH_6;
        case 6:  return sushi::ext::MidiChannel::MIDI_CH_7;
        case 7:  return sushi::ext::MidiChannel::MIDI_CH_8;
        case 8:  return sushi::ext::MidiChannel::MIDI_CH_9;
        case 9:  return sushi::ext::MidiChannel::MIDI_CH_10;
        case 10: return sushi::ext::MidiChannel::MIDI_CH_11;
        case 11: return sushi::ext::MidiChannel::MIDI_CH_12;
        case 12: return sushi::ext::MidiChannel::MIDI_CH_13;
        case 13: return sushi::ext::MidiChannel::MIDI_CH_14;
        case 14: return sushi::ext::MidiChannel::MIDI_CH_15;
        case 15: return sushi::ext::MidiChannel::MIDI_CH_16;
        case 16: return sushi::ext::MidiChannel::MIDI_CH_OMNI;
        default: return sushi::ext::MidiChannel::MIDI_CH_OMNI;
    }
}

inline int int_from_ext_midi_channel(ext::MidiChannel channel)
{
    switch (channel)
    {
        case sushi::ext::MidiChannel::MIDI_CH_1: return 0;
        case sushi::ext::MidiChannel::MIDI_CH_2: return 1;
        case sushi::ext::MidiChannel::MIDI_CH_3: return 2;
        case sushi::ext::MidiChannel::MIDI_CH_4: return 3;
        case sushi::ext::MidiChannel::MIDI_CH_5: return 4;
        case sushi::ext::MidiChannel::MIDI_CH_6: return 5;
        case sushi::ext::MidiChannel::MIDI_CH_7: return 6;
        case sushi::ext::MidiChannel::MIDI_CH_8: return 7;
        case sushi::ext::MidiChannel::MIDI_CH_9: return 8;
        case sushi::ext::MidiChannel::MIDI_CH_10: return 9;
        case sushi::ext::MidiChannel::MIDI_CH_11: return 10;
        case sushi::ext::MidiChannel::MIDI_CH_12: return 11;
        case sushi::ext::MidiChannel::MIDI_CH_13: return 12;
        case sushi::ext::MidiChannel::MIDI_CH_14: return 13;
        case sushi::ext::MidiChannel::MIDI_CH_15: return 14;
        case sushi::ext::MidiChannel::MIDI_CH_16: return 15;
        case sushi::ext::MidiChannel::MIDI_CH_OMNI: return 16;
        default: return 16;
    }
}

inline PluginType to_internal(ext::PluginType type)
{
    switch (type)
    {
        case ext::PluginType::INTERNAL:   return PluginType::INTERNAL;
        case ext::PluginType::VST2X:      return PluginType::VST2X;
        case ext::PluginType::VST3X:      return PluginType::VST3X;
        case ext::PluginType::LV2:        return PluginType::LV2;
        default:                          return PluginType::INTERNAL;
    }
}

inline ext::PluginType to_external(PluginType type)
{
    switch (type)
    {
        case PluginType::INTERNAL:   return ext::PluginType::INTERNAL;
        case PluginType::VST2X:      return ext::PluginType::VST2X;
        case PluginType::VST3X:      return ext::PluginType::VST3X;
        case PluginType::LV2:        return ext::PluginType::LV2;
        default:                     return ext::PluginType::INTERNAL;
    }
}

inline ext::TrackType to_external(TrackType type)
{
    switch (type)
    {
        case TrackType::REGULAR:    return ext::TrackType::REGULAR;
        case TrackType::PRE:        return ext::TrackType::PRE;
        case TrackType::POST:       return ext::TrackType::POST;
        default:                    return ext::TrackType::REGULAR;
    }
}

inline void to_internal(sushi::ProcessorState* dest, const ext::ProcessorState* src)
{
    if (src->program.has_value()) dest->set_program(src->program.value());
    if (src->bypassed.has_value()) dest->set_bypass(src->bypassed.value());
    for (const auto& p : src->parameters)
    {
        dest->add_parameter_change(p.first, p.second);
    }
    for (const auto& p : src->properties)
    {
        dest->add_property_change(p.first, p.second);
    }
    // Note, binary data is be moved instead of copied, don't access src->binary_data() afterwards
    dest->set_binary_data(std::move(src->binary_data));
}

inline void to_external(ext::ProcessorState* dest, sushi::ProcessorState* src)
{
    dest->program = src->program();
    dest->bypassed = src->bypassed();
    dest->parameters.reserve(src->parameters().size());
    for (const auto& param : src->parameters())
    {
        dest->parameters.push_back({param.first, param.second});
    }
    dest->properties.reserve(src->properties().size());
    for (const auto& property : src->properties())
    {
        dest->properties.push_back({property.first, property.second});
    }
    // Note, binary data is be moved instead of copied, don't access src->binary_data() afterwards
    dest->binary_data = std::move(src->binary_data());
}

} // controller_impl
} // namespace engine
} // namespace sushi

#endif //SUSHI_CONTROLLER_COMMON_H
