/*
 * Copyright 2017-2023 Elk Audio AB
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI. If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Container for types and functions common to several sub-controllers
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef SUSHI_CONTROLLER_COMMON_H
#define SUSHI_CONTROLLER_COMMON_H

#include "sushi/control_interface.h"

#include "library/base_performance_timer.h"

namespace sushi::internal::engine::controller_impl {

/* Convenience conversion functions between external and internal
 * enums and data structs */
inline control::PlayingMode to_external(const sushi::internal::PlayingMode mode)
{
    switch (mode)
    {
        case PlayingMode::STOPPED:      return control::PlayingMode::STOPPED;
        case PlayingMode::PLAYING:      return control::PlayingMode::PLAYING;
        case PlayingMode::RECORDING:    return control::PlayingMode::RECORDING;
        default:                        return control::PlayingMode::PLAYING;
    }
}

inline sushi::internal::PlayingMode to_internal(const control::PlayingMode mode)
{
    switch (mode)
    {
        case control::PlayingMode::STOPPED:   return sushi::internal::PlayingMode::STOPPED;
        case control::PlayingMode::PLAYING:   return sushi::internal::PlayingMode::PLAYING;
        case control::PlayingMode::RECORDING: return sushi::internal::PlayingMode::RECORDING;
        default:                              return sushi::internal::PlayingMode::PLAYING;
    }
}

inline control::SyncMode to_external(const sushi::internal::SyncMode mode)
{
    switch (mode)
    {
        case SyncMode::INTERNAL:     return control::SyncMode::INTERNAL;
        case SyncMode::MIDI:         return control::SyncMode::MIDI;
        case SyncMode::GATE_INPUT:   return control::SyncMode::GATE;
        case SyncMode::ABLETON_LINK: return control::SyncMode::LINK;
        default:                     return control::SyncMode::INTERNAL;
    }
}

inline sushi::internal::SyncMode to_internal(const control::SyncMode mode)
{
    switch (mode)
    {
        case control::SyncMode::INTERNAL: return sushi::internal::SyncMode::INTERNAL;
        case control::SyncMode::MIDI:     return sushi::internal::SyncMode::MIDI;
        case control::SyncMode::GATE:     return sushi::internal::SyncMode::GATE_INPUT;
        case control::SyncMode::LINK:     return sushi::internal::SyncMode::ABLETON_LINK;
        default:                          return sushi::internal::SyncMode::INTERNAL;
    }
}

inline control::CpuTimings to_external(const sushi::internal::performance::ProcessTimings& timings)
{
    return {.avg = timings.avg_case,
            .min = timings.min_case,
            .max = timings.max_case};
}

inline control::TimeSignature to_external(sushi::TimeSignature internal)
{
    return {internal.numerator, internal.denominator};
}

inline sushi::TimeSignature to_internal(control::TimeSignature ext)
{
    return {ext.numerator, ext.denominator};
}

inline control::MidiChannel to_external_midi_channel(int channel_int)
{
    switch (channel_int)
    {
        case 0:  return sushi::control::MidiChannel::MIDI_CH_1;
        case 1:  return sushi::control::MidiChannel::MIDI_CH_2;
        case 2:  return sushi::control::MidiChannel::MIDI_CH_3;
        case 3:  return sushi::control::MidiChannel::MIDI_CH_4;
        case 4:  return sushi::control::MidiChannel::MIDI_CH_5;
        case 5:  return sushi::control::MidiChannel::MIDI_CH_6;
        case 6:  return sushi::control::MidiChannel::MIDI_CH_7;
        case 7:  return sushi::control::MidiChannel::MIDI_CH_8;
        case 8:  return sushi::control::MidiChannel::MIDI_CH_9;
        case 9:  return sushi::control::MidiChannel::MIDI_CH_10;
        case 10: return sushi::control::MidiChannel::MIDI_CH_11;
        case 11: return sushi::control::MidiChannel::MIDI_CH_12;
        case 12: return sushi::control::MidiChannel::MIDI_CH_13;
        case 13: return sushi::control::MidiChannel::MIDI_CH_14;
        case 14: return sushi::control::MidiChannel::MIDI_CH_15;
        case 15: return sushi::control::MidiChannel::MIDI_CH_16;
        case 16: return sushi::control::MidiChannel::MIDI_CH_OMNI;
        default: return sushi::control::MidiChannel::MIDI_CH_OMNI;
    }
}

inline int int_from_ext_midi_channel(control::MidiChannel channel)
{
    switch (channel)
    {
        case sushi::control::MidiChannel::MIDI_CH_1: return 0;
        case sushi::control::MidiChannel::MIDI_CH_2: return 1;
        case sushi::control::MidiChannel::MIDI_CH_3: return 2;
        case sushi::control::MidiChannel::MIDI_CH_4: return 3;
        case sushi::control::MidiChannel::MIDI_CH_5: return 4;
        case sushi::control::MidiChannel::MIDI_CH_6: return 5;
        case sushi::control::MidiChannel::MIDI_CH_7: return 6;
        case sushi::control::MidiChannel::MIDI_CH_8: return 7;
        case sushi::control::MidiChannel::MIDI_CH_9: return 8;
        case sushi::control::MidiChannel::MIDI_CH_10: return 9;
        case sushi::control::MidiChannel::MIDI_CH_11: return 10;
        case sushi::control::MidiChannel::MIDI_CH_12: return 11;
        case sushi::control::MidiChannel::MIDI_CH_13: return 12;
        case sushi::control::MidiChannel::MIDI_CH_14: return 13;
        case sushi::control::MidiChannel::MIDI_CH_15: return 14;
        case sushi::control::MidiChannel::MIDI_CH_16: return 15;
        case sushi::control::MidiChannel::MIDI_CH_OMNI: return 16;
        default: return 16;
    }
}

inline PluginType to_internal(control::PluginType type)
{
    switch (type)
    {
        case control::PluginType::INTERNAL:   return PluginType::INTERNAL;
        case control::PluginType::VST2X:      return PluginType::VST2X;
        case control::PluginType::VST3X:      return PluginType::VST3X;
        case control::PluginType::LV2:        return PluginType::LV2;
        default:                              return PluginType::INTERNAL;
    }
}

inline control::PluginType to_external(PluginType type)
{
    switch (type)
    {
        case PluginType::INTERNAL:   return control::PluginType::INTERNAL;
        case PluginType::VST2X:      return control::PluginType::VST2X;
        case PluginType::VST3X:      return control::PluginType::VST3X;
        case PluginType::LV2:        return control::PluginType::LV2;
        default:                     return control::PluginType::INTERNAL;
    }
}

inline control::TrackType to_external(TrackType type)
{
    switch (type)
    {
        case TrackType::REGULAR:    return control::TrackType::REGULAR;
        case TrackType::PRE:        return control::TrackType::PRE;
        case TrackType::POST:       return control::TrackType::POST;
        default:                    return control::TrackType::REGULAR;
    }
}

inline void to_internal(sushi::internal::ProcessorState* dest, const control::ProcessorState* src)
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

inline void to_external(control::ProcessorState* dest, sushi::internal::ProcessorState* src)
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

} // end namespace sushi::internal::engine::controller_impl

#endif // SUSHI_CONTROLLER_COMMON_H
