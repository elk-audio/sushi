/*
* Copyright 2017-2026 Elk Audio AB
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
 * @brief Conversion function between sushi internals and generated protobuf objects
 * @Copyright 2017-2026 Elk Audio AB, Stockholm
 */

#ifndef SUSHI_LIBRARY_CONVERSIONS_H
#define SUSHI_LIBRARY_CONVERSIONS_H

ELK_PUSH_WARNING
ELK_DISABLE_UNUSED_PARAMETER
ELK_DISABLE_UNREACHABLE_CODE
#include "sushi_rpc.pb.h"
ELK_POP_WARNING

namespace sushi_rpc {
/* Convenience conversion functions between sushi enums and their respective grpc implementations */
inline sushi_rpc::ParameterType::Type to_grpc(const sushi::control::ParameterType type)
{
    switch (type)
    {
        case sushi::control::ParameterType::FLOAT:        return sushi_rpc::ParameterType::FLOAT;
        case sushi::control::ParameterType::INT:          return sushi_rpc::ParameterType::INT;
        case sushi::control::ParameterType::BOOL:         return sushi_rpc::ParameterType::BOOL;
        default:                                          return sushi_rpc::ParameterType::FLOAT;
    }
}

inline sushi_rpc::PlayingMode::Mode to_grpc(const sushi::control::PlayingMode mode)
{
    switch (mode)
    {
        case sushi::control::PlayingMode::STOPPED:      return sushi_rpc::PlayingMode::STOPPED;
        case sushi::control::PlayingMode::PLAYING:      return sushi_rpc::PlayingMode::PLAYING;
        case sushi::control::PlayingMode::RECORDING:    return sushi_rpc::PlayingMode::RECORDING;
        default:                                        return sushi_rpc::PlayingMode::PLAYING;
    }
}

inline MidiChannel_Channel to_grpc(const sushi::control::MidiChannel channel)
{
    switch (channel)
    {
        case sushi::control::MidiChannel::MIDI_CH_1:    return sushi_rpc::MidiChannel::MIDI_CH_1;
        case sushi::control::MidiChannel::MIDI_CH_2:    return sushi_rpc::MidiChannel::MIDI_CH_2;
        case sushi::control::MidiChannel::MIDI_CH_3:    return sushi_rpc::MidiChannel::MIDI_CH_3;
        case sushi::control::MidiChannel::MIDI_CH_4:    return sushi_rpc::MidiChannel::MIDI_CH_4;
        case sushi::control::MidiChannel::MIDI_CH_5:    return sushi_rpc::MidiChannel::MIDI_CH_5;
        case sushi::control::MidiChannel::MIDI_CH_6:    return sushi_rpc::MidiChannel::MIDI_CH_6;
        case sushi::control::MidiChannel::MIDI_CH_7:    return sushi_rpc::MidiChannel::MIDI_CH_7;
        case sushi::control::MidiChannel::MIDI_CH_8:    return sushi_rpc::MidiChannel::MIDI_CH_8;
        case sushi::control::MidiChannel::MIDI_CH_9:    return sushi_rpc::MidiChannel::MIDI_CH_9;
        case sushi::control::MidiChannel::MIDI_CH_10:   return sushi_rpc::MidiChannel::MIDI_CH_10;
        case sushi::control::MidiChannel::MIDI_CH_11:   return sushi_rpc::MidiChannel::MIDI_CH_11;
        case sushi::control::MidiChannel::MIDI_CH_12:   return sushi_rpc::MidiChannel::MIDI_CH_12;
        case sushi::control::MidiChannel::MIDI_CH_13:   return sushi_rpc::MidiChannel::MIDI_CH_13;
        case sushi::control::MidiChannel::MIDI_CH_14:   return sushi_rpc::MidiChannel::MIDI_CH_14;
        case sushi::control::MidiChannel::MIDI_CH_15:   return sushi_rpc::MidiChannel::MIDI_CH_15;
        case sushi::control::MidiChannel::MIDI_CH_16:   return sushi_rpc::MidiChannel::MIDI_CH_16;
        case sushi::control::MidiChannel::MIDI_CH_OMNI: return sushi_rpc::MidiChannel::MIDI_CH_OMNI;
        default:                                        return sushi_rpc::MidiChannel::MIDI_CH_OMNI;
    }
}

inline sushi::control::MidiChannel to_sushi_ext(const MidiChannel_Channel channel)
{
    switch (channel)
    {
        case sushi_rpc::MidiChannel::MIDI_CH_1:    return sushi::control::MidiChannel::MIDI_CH_1;
        case sushi_rpc::MidiChannel::MIDI_CH_2:    return sushi::control::MidiChannel::MIDI_CH_2;
        case sushi_rpc::MidiChannel::MIDI_CH_3:    return sushi::control::MidiChannel::MIDI_CH_3;
        case sushi_rpc::MidiChannel::MIDI_CH_4:    return sushi::control::MidiChannel::MIDI_CH_4;
        case sushi_rpc::MidiChannel::MIDI_CH_5:    return sushi::control::MidiChannel::MIDI_CH_5;
        case sushi_rpc::MidiChannel::MIDI_CH_6:    return sushi::control::MidiChannel::MIDI_CH_6;
        case sushi_rpc::MidiChannel::MIDI_CH_7:    return sushi::control::MidiChannel::MIDI_CH_7;
        case sushi_rpc::MidiChannel::MIDI_CH_8:    return sushi::control::MidiChannel::MIDI_CH_8;
        case sushi_rpc::MidiChannel::MIDI_CH_9:    return sushi::control::MidiChannel::MIDI_CH_9;
        case sushi_rpc::MidiChannel::MIDI_CH_10:   return sushi::control::MidiChannel::MIDI_CH_10;
        case sushi_rpc::MidiChannel::MIDI_CH_11:   return sushi::control::MidiChannel::MIDI_CH_11;
        case sushi_rpc::MidiChannel::MIDI_CH_12:   return sushi::control::MidiChannel::MIDI_CH_12;
        case sushi_rpc::MidiChannel::MIDI_CH_13:   return sushi::control::MidiChannel::MIDI_CH_13;
        case sushi_rpc::MidiChannel::MIDI_CH_14:   return sushi::control::MidiChannel::MIDI_CH_14;
        case sushi_rpc::MidiChannel::MIDI_CH_15:   return sushi::control::MidiChannel::MIDI_CH_15;
        case sushi_rpc::MidiChannel::MIDI_CH_16:   return sushi::control::MidiChannel::MIDI_CH_16;
        case sushi_rpc::MidiChannel::MIDI_CH_OMNI: return sushi::control::MidiChannel::MIDI_CH_OMNI;
        default:                                   return sushi::control::MidiChannel::MIDI_CH_OMNI;
    }
}

inline sushi::control::PlayingMode to_sushi_ext(const sushi_rpc::PlayingMode::Mode mode)
{
    switch (mode)
    {
        case sushi_rpc::PlayingMode::STOPPED:   return sushi::control::PlayingMode::STOPPED;
        case sushi_rpc::PlayingMode::PLAYING:   return sushi::control::PlayingMode::PLAYING;
        case sushi_rpc::PlayingMode::RECORDING: return sushi::control::PlayingMode::RECORDING;
        default:                                return sushi::control::PlayingMode::PLAYING;
    }
}

inline sushi_rpc::SyncMode::Mode to_grpc(const sushi::control::SyncMode mode)
{
    switch (mode)
    {
        case sushi::control::SyncMode::INTERNAL: return sushi_rpc::SyncMode::INTERNAL;
        case sushi::control::SyncMode::MIDI:     return sushi_rpc::SyncMode::MIDI;
        case sushi::control::SyncMode::LINK:     return sushi_rpc::SyncMode::LINK;
        default:                                 return sushi_rpc::SyncMode::INTERNAL;
    }
}

inline sushi::control::SyncMode to_sushi_ext(const sushi_rpc::SyncMode::Mode mode)
{
    switch (mode)
    {
        case sushi_rpc::SyncMode::INTERNAL: return sushi::control::SyncMode::INTERNAL;
        case sushi_rpc::SyncMode::MIDI:     return sushi::control::SyncMode::MIDI;
        case sushi_rpc::SyncMode::LINK:     return sushi::control::SyncMode::LINK;
        default:                            return sushi::control::SyncMode::INTERNAL;
    }
}

inline sushi_rpc::TrackType::Type to_grpc(const sushi::control::TrackType type)
{
    switch (type)
    {
        case sushi::control::TrackType::REGULAR:  return sushi_rpc::TrackType::REGULAR;
        case sushi::control::TrackType::PRE:      return sushi_rpc::TrackType::PRE;
        case sushi::control::TrackType::POST:     return sushi_rpc::TrackType::POST;
        default:                                  return sushi_rpc::TrackType::REGULAR;
    }
}

inline sushi::control::TrackType to_sushi_ext(const sushi_rpc::TrackType::Type type)
{
    switch (type)
    {
        case sushi_rpc::TrackType::REGULAR: return sushi::control::TrackType::REGULAR;
        case sushi_rpc::TrackType::PRE:     return sushi::control::TrackType::PRE;
        case sushi_rpc::TrackType::POST:    return sushi::control::TrackType::POST;
        default:                            return sushi::control::TrackType::REGULAR;
    }
}

inline const char* to_string(const sushi::control::ControlStatus status)
{
   switch (status)
    {
        case sushi::control::ControlStatus::OK:                    return "OK";
        case sushi::control::ControlStatus::ERROR:                 return "ERROR";
        case sushi::control::ControlStatus::UNSUPPORTED_OPERATION: return "UNSUPPORTED OPERATION";
        case sushi::control::ControlStatus::NOT_FOUND:             return "NOT FOUND";
        case sushi::control::ControlStatus::OUT_OF_RANGE:          return "OUT OF RANGE";
        case sushi::control::ControlStatus::INVALID_ARGUMENTS:     return "INVALID ARGUMENTS";
        default:                                                   return "INTERNAL";
    }
}

inline sushi_rpc::CommandStatus::Status to_grpc(sushi::control::ControlStatus status)
{
    switch (status)
    {
        case sushi::control::ControlStatus::OK:                    return sushi_rpc::CommandStatus::SUCCESS;
        case sushi::control::ControlStatus::ASYNC_RESPONSE:        return sushi_rpc::CommandStatus::ASYNC_RESPONSE;
        case sushi::control::ControlStatus::ERROR:                 return sushi_rpc::CommandStatus::ERROR;
        case sushi::control::ControlStatus::UNSUPPORTED_OPERATION: return sushi_rpc::CommandStatus::UNSUPPORTED_OPERATION;
        case sushi::control::ControlStatus::NOT_FOUND:             return sushi_rpc::CommandStatus::NOT_FOUND;
        case sushi::control::ControlStatus::OUT_OF_RANGE:          return sushi_rpc::CommandStatus::OUT_OF_RANGE;
        case sushi::control::ControlStatus::INVALID_ARGUMENTS:     return sushi_rpc::CommandStatus::INVALID_ARGUMENTS;
        default:                                                   return sushi_rpc::CommandStatus::DUMMY;
    }
}

inline void to_grpc(CommandResponse& dest, const sushi::control::ControlResponse src)
{
    dest.mutable_status()->set_status(to_grpc(src.status));
    dest.set_id(src.id);
}

inline void to_grpc(CommandResponse& dest, const sushi::control::ControlStatus src)
{
    dest.mutable_status()->set_status(to_grpc(src));
    dest.set_id(0);
}

inline void to_grpc(ParameterInfo& dest, const sushi::control::ParameterInfo& src)
{
    dest.set_id(src.id);
    dest.mutable_type()->set_type(to_grpc(src.type));
    dest.set_label(src.label);
    dest.set_name(src.name);
    dest.set_unit(src.unit);
    dest.set_automatable(src.automatable);
    dest.set_min_domain_value(src.min_domain_value);
    dest.set_max_domain_value(src.max_domain_value);
}

//inline void to_grpc(ParameterIdentifier& dest, const sushi::control::ParameterChangeNotifica

inline void to_grpc(PropertyInfo& dest, const sushi::control::PropertyInfo& src)
{
    dest.set_id(src.id);
    dest.set_name(src.name);
    dest.set_label(src.label);
}

inline void to_grpc(sushi_rpc::ProcessorInfo& dest, const sushi::control::ProcessorInfo& src)
{
    dest.set_id(src.id);
    dest.set_label(src.label);
    dest.set_name(src.name);
    dest.set_parameter_count(src.parameter_count);
    dest.set_program_count(src.program_count);
}

inline void to_grpc(sushi_rpc::MidiKbdConnection& dest, const sushi::control::MidiKbdConnection& src)
{
    dest.mutable_track()->set_id(src.track_id);
    dest.mutable_channel()->set_channel(to_grpc(src.channel));
    dest.set_port(src.port);
    dest.set_raw_midi(src.raw_midi);
}

inline void to_grpc(sushi_rpc::MidiCCConnection& dest, const sushi::control::MidiCCConnection& src)
{
    dest.mutable_parameter()->set_processor_id(src.processor_id);
    dest.mutable_parameter()->set_parameter_id(src.parameter_id);
    dest.mutable_parameter()->set_processor_id(src.processor_id);
    dest.mutable_channel()->set_channel(to_grpc(src.channel));
    dest.set_port(src.port);
    dest.set_cc_number(src.cc_number);
    dest.set_min_range(static_cast<float>(src.min_range));
    dest.set_max_range(static_cast<float>(src.max_range));
    dest.set_relative_mode(src.relative_mode);
}

inline void to_grpc(sushi_rpc::MidiPCConnection& dest, const sushi::control::MidiPCConnection& src)
{
    dest.mutable_processor()->set_id(src.processor_id);
    dest.mutable_channel()->set_channel(to_grpc(src.channel));
    dest.set_port(src.port);
}

inline void to_grpc(sushi_rpc::TrackInfo& dest, const sushi::control::TrackInfo& src)
{
    dest.set_id(src.id);
    dest.set_label(src.label);
    dest.set_name(src.name);
    dest.set_channels(src.channels);
    dest.set_buses(src.buses);
    dest.set_thread(src.thread);
    dest.mutable_type()->set_type(to_grpc(src.type));
    for (auto i : src.processors)
    {
        dest.mutable_processors()->Add()->set_id(i);
    }
}

inline void to_grpc(sushi_rpc::Timings& dest, const sushi::control::Timings& src)
{
    dest.set_average(src.avg);
    dest.set_min(src.min);
    dest.set_max(src.max);
}

inline void to_grpc(sushi_rpc::CpuTimings& dest, const sushi::control::CpuTimings& src)
{
    to_grpc(*dest.mutable_main(), src.main);
    for (const auto& thread : src.threads)
    {
        to_grpc(*dest.mutable_threads()->Add(), thread);
    }
}

inline void to_grpc(sushi_rpc::AudioConnection& dest, const sushi::control::AudioConnection& src)
{
    dest.mutable_track()->set_id(src.track_id);
    dest.set_track_channel(src.track_channel);
    dest.set_engine_channel(src.engine_channel);
}

inline sushi_rpc::PluginType::Type to_grpc(const sushi::control::PluginType type)
{
    switch (type)
    {
        case sushi::control::PluginType::INTERNAL:       return sushi_rpc::PluginType::INTERNAL;
        case sushi::control::PluginType::VST2X:          return sushi_rpc::PluginType::VST2X;
        case sushi::control::PluginType::VST3X:          return sushi_rpc::PluginType::VST3X;
        case sushi::control::PluginType::LV2:            return sushi_rpc::PluginType::LV2;
        default:                                         return sushi_rpc::PluginType::INTERNAL;
    }
}

inline sushi::control::PluginType to_sushi_ext(const sushi_rpc::PluginType::Type type)
{
    switch (type)
    {
        case sushi_rpc::PluginType::INTERNAL:       return sushi::control::PluginType::INTERNAL;
        case sushi_rpc::PluginType::VST2X:          return sushi::control::PluginType::VST2X;
        case sushi_rpc::PluginType::VST3X:          return sushi::control::PluginType::VST3X;
        case sushi_rpc::PluginType::LV2:            return sushi::control::PluginType::LV2;
        default:                                    return sushi::control::PluginType::INTERNAL;
    }
}

inline void to_grpc(sushi_rpc::ProcessorState& dest, sushi::control::ProcessorState& src)
{
    if (src.program.has_value())
    {
        dest.mutable_program_id()->set_value(src.program.value());
        dest.mutable_program_id()->set_has_value(true);
    }
    if (src.bypassed.has_value())
    {
        dest.mutable_bypassed()->set_value(src.bypassed.value());
        dest.mutable_bypassed()->set_has_value(true);
    }

    dest.mutable_properties()->Reserve(static_cast<int>(src.properties.size()));
    for (auto& p : src.properties)
    {
        auto target = dest.mutable_properties()->Add();
        target->mutable_property()->set_property_id(p.first);
        target->set_value(std::move(p.second));
    }

    dest.mutable_parameters()->Reserve(static_cast<int>(src.parameters.size()));
    for (const auto& p : src.parameters)
    {
        auto target = dest.mutable_parameters()->Add();
        target->mutable_parameter()->set_parameter_id(p.first);
        target->set_value(p.second);
    }

    // Todo: investigate if this can be moved for efficiency
    dest.mutable_binary_data()->append(reinterpret_cast<const char*>(src.binary_data.data()),
                                       reinterpret_cast<const char*>(src.binary_data.data()) + src.binary_data.size());
}

inline void to_sushi_ext(sushi::control::ProcessorState& dest, const sushi_rpc::ProcessorState& src)
{
    if (src.program_id().has_value())
    {
        dest.program = src.program_id().value();
    }
    if (src.bypassed().has_value())
    {
        dest.bypassed = src.bypassed().value();
    }

    dest.properties.reserve(src.properties_size());
    for (const auto& p : src.properties())
    {
        dest.properties.push_back({p.property().property_id(), p.value()});
    }

    dest.parameters.reserve(src.parameters_size());
    for (const auto& p : src.parameters())
    {
        dest.parameters.push_back({p.parameter().parameter_id(), p.value()});
    }

    dest.binary_data.reserve(src.binary_data().size());
    dest.binary_data.insert(dest.binary_data.begin(),
                            reinterpret_cast<const std::byte*>(src.binary_data().data()),
                            reinterpret_cast<const std::byte*>(src.binary_data().data()) + src.binary_data().size());
}

inline void to_grpc(sushi_rpc::SushiBuildInfo& dest, sushi::control::SushiBuildInfo& src)
{
    dest.set_version(std::move(src.version));

    for (auto& option : src.build_options)
    {
        dest.add_build_options(std::move(option));
    }

    dest.set_audio_buffer_size(src.audio_buffer_size);
    dest.set_commit_hash(std::move(src.commit_hash));
    dest.set_build_date(std::move(src.build_date));
}

inline void to_sushi_ext(sushi::control::SushiBuildInfo& dest, const sushi_rpc::SushiBuildInfo& src)
{
    dest.version = src.version();

    for (auto& option : src.build_options())
    {
        dest.build_options.push_back(option);
    }

    dest.audio_buffer_size = src.audio_buffer_size();
    dest.commit_hash = src.commit_hash();
    dest.build_date = src.build_date();
}

inline void to_grpc(sushi_rpc::OscParameterState& dest, sushi::control::OscParameterState& src)
{
    dest.set_processor(std::move(src.processor));
    dest.mutable_parameter_ids()->Reserve(static_cast<int>(src.parameter_ids.size()));
    for (const auto& id : src.parameter_ids)
    {
        dest.mutable_parameter_ids()->Add(id);
    }
}

inline sushi::control::OscParameterState to_sushi_ext(const sushi_rpc::OscParameterState& src)
{
    sushi::control::OscParameterState dest;
    dest.processor = src.processor();
    dest.parameter_ids.insert(dest.parameter_ids.begin(), src.parameter_ids().begin(), src.parameter_ids().end());
    return dest;
}

inline void to_grpc(sushi_rpc::OscState& dest, sushi::control::OscState& src)
{
    dest.set_enable_all_processor_outputs(src.enable_all_processor_outputs);
    dest.mutable_enabled_processor_outputs()->Reserve(static_cast<int>(src.enabled_processor_outputs.size()));
    for (auto& state : src.enabled_processor_outputs)
    {
        auto grpc_state = dest.mutable_enabled_processor_outputs()->Add();
        to_grpc(*grpc_state, state);
    }
}

inline void to_sushi_ext(sushi::control::OscState& dest, const sushi_rpc::OscState& src)
{
    dest.enable_all_processor_outputs = src.enable_all_processor_outputs();
    dest.enabled_processor_outputs.reserve(src.enabled_processor_outputs_size());
    for (auto& state : src.enabled_processor_outputs())
    {
        dest.enabled_processor_outputs.push_back(to_sushi_ext(state));
    }
}

inline void to_grpc(sushi_rpc::MidiKbdConnectionState& dest, sushi::control::MidiKbdConnectionState& src)
{
    dest.set_track(std::move(src.track));
    dest.mutable_channel()->set_channel(to_grpc(src.channel));
    dest.set_port(src.port);
    dest.set_raw_midi(src.raw_midi);
}

inline sushi::control::MidiKbdConnectionState to_sushi_ext(const sushi_rpc::MidiKbdConnectionState& src)
{
    sushi::control::MidiKbdConnectionState dest;
    dest.track = src.track();
    dest.channel = to_sushi_ext(src.channel().channel());
    dest.port = src.port();
    dest.raw_midi = src.raw_midi();
    return dest;
}

inline void to_grpc(sushi_rpc::MidiCCConnectionState& dest, sushi::control::MidiCCConnectionState& src)
{
    dest.set_processor(std::move(src.processor));
    dest.mutable_parameter()->set_parameter_id(src.parameter_id);
    dest.mutable_channel()->set_channel(to_grpc(src.channel));
    dest.set_port(src.port);
    dest.set_cc_number(src.cc_number);
    dest.set_min_range(src.min_range);
    dest.set_max_range(src.max_range);
    dest.set_relative_mode(src.relative_mode);
}

inline sushi::control::MidiCCConnectionState to_sushi_ext(const sushi_rpc::MidiCCConnectionState& src)
{
    sushi::control::MidiCCConnectionState dest;
    dest.processor = src.processor();
    dest.channel = to_sushi_ext(src.channel().channel());
    dest.port = src.port();
    dest.cc_number = src.cc_number();
    dest.min_range = src.min_range();
    dest.max_range = src.max_range();
    dest.relative_mode = src.relative_mode();
    return dest;
}

inline void to_grpc(sushi_rpc::MidiPCConnectionState& dest, sushi::control::MidiPCConnectionState& src)
{
    dest.set_processor(std::move(src.processor));
    dest.mutable_channel()->set_channel(to_grpc(src.channel));
    dest.set_port(src.port);
}

inline sushi::control::MidiPCConnectionState to_sushi_ext(const sushi_rpc::MidiPCConnectionState& src)
{
    sushi::control::MidiPCConnectionState dest;
    dest.processor = src.processor();
    dest.channel = to_sushi_ext(src.channel().channel());
    dest.port = src.port();
    return dest;
}

inline void to_grpc(sushi_rpc::MidiState& dest, sushi::control::MidiState& src)
{
    dest.set_inputs(src.inputs);
    dest.set_outputs(src.outputs);

    dest.mutable_kbd_input_connections()->Reserve(static_cast<int>(src.kbd_input_connections.size()));
    for (auto& con : src.kbd_input_connections)
    {
        auto grpc_con = dest.mutable_kbd_input_connections()->Add();
        to_grpc(*grpc_con, con);
    }

    dest.mutable_kbd_output_connections()->Reserve(static_cast<int>(src.kbd_output_connections.size()));
    for (auto& con : src.kbd_output_connections)
    {
        auto grpc_con = dest.mutable_kbd_output_connections()->Add();
        to_grpc(*grpc_con, con);
    }

    dest.mutable_cc_connections()->Reserve(static_cast<int>(src.cc_connections.size()));
    for (auto& con : src.cc_connections)
    {
        auto grpc_con = dest.mutable_cc_connections()->Add();
        to_grpc(*grpc_con, con);
    }

    dest.mutable_pc_connections()->Reserve(static_cast<int>(src.pc_connections.size()));
    for (auto& con : src.pc_connections)
    {
        auto grpc_con = dest.mutable_pc_connections()->Add();
        to_grpc(*grpc_con, con);
    }

    dest.mutable_enabled_clock_outputs()->Reserve(static_cast<int>(src.enabled_clock_outputs.size()));
    for (auto port : src.enabled_clock_outputs)
    {
        dest.mutable_enabled_clock_outputs()->Add(port);
    }
}

inline void to_sushi_ext(sushi::control::MidiState& dest, const sushi_rpc::MidiState& src)
{
    dest.inputs = src.inputs();
    dest.outputs = src.outputs();

    dest.kbd_input_connections.reserve(src.kbd_input_connections_size());
    for (auto& con : src.kbd_input_connections())
    {
        dest.kbd_input_connections.push_back(to_sushi_ext(con));
    }

    dest.kbd_output_connections.reserve(src.kbd_output_connections_size());
    for (auto& con : src.kbd_output_connections())
    {
        dest.kbd_output_connections.push_back(to_sushi_ext(con));
    }

    dest.cc_connections.reserve(src.cc_connections_size());
    for (auto& con : src.cc_connections())
    {
        dest.cc_connections.push_back(to_sushi_ext(con));
    }
    dest.pc_connections.reserve(src.pc_connections_size());
    for (auto& con : src.pc_connections())
    {
        dest.pc_connections.push_back(to_sushi_ext(con));
    }
    dest.enabled_clock_outputs = std::vector<int>(src.enabled_clock_outputs().begin(), src.enabled_clock_outputs().end());
}

inline void to_grpc(sushi_rpc::TrackAudioConnectionState& dest, sushi::control::TrackAudioConnectionState& src)
{
    dest.set_track(std::move(src.track));
    dest.set_track_channel(src.track_channel);
    dest.set_engine_channel(src.engine_channel);
}

inline sushi::control::TrackAudioConnectionState to_sushi_ext(const sushi_rpc::TrackAudioConnectionState& src)
{
    sushi::control::TrackAudioConnectionState dest;
    dest.track = src.track();
    dest.track_channel = src.track_channel();
    dest.engine_channel = src.engine_channel();
    return dest;
}

inline void to_grpc(sushi_rpc::EngineState& dest, sushi::control::EngineState& src)
{
    dest.set_sample_rate(src.sample_rate);
    dest.set_tempo(src.tempo);
    dest.mutable_playing_mode()->set_mode(to_grpc(src.playing_mode));
    dest.mutable_sync_mode()->set_mode(to_grpc(src.sync_mode));
    dest.mutable_time_signature()->set_denominator(src.time_signature.denominator);
    dest.mutable_time_signature()->set_numerator(src.time_signature.numerator);
    dest.set_clip_detection_input(src.input_clip_detection);
    dest.set_clip_detection_output(src.output_clip_detection);
    dest.set_master_limiter(src.master_limiter);
    dest.set_used_audio_inputs(src.used_audio_inputs);
    dest.set_used_audio_outputs(src.used_audio_outputs);

    dest.mutable_input_connections()->Reserve(static_cast<int>(src.input_connections.size()));
    for (auto& con : src.input_connections)
    {
        auto grpc_con = dest.mutable_input_connections()->Add();
        to_grpc(*grpc_con, con);
    }

    dest.mutable_output_connections()->Reserve(static_cast<int>(src.output_connections.size()));
    for (auto& con : src.output_connections)
    {
        auto grpc_con = dest.mutable_output_connections()->Add();
        to_grpc(*grpc_con, con);
    }
}

inline void to_sushi_ext(sushi::control::EngineState& dest, const sushi_rpc::EngineState& src)
{
    dest.sample_rate = src.sample_rate();
    dest.tempo = src.tempo();
    dest.playing_mode = to_sushi_ext(src.playing_mode().mode());
    dest.sync_mode = to_sushi_ext(src.sync_mode().mode());
    dest.time_signature = {src.time_signature().numerator(), src.time_signature().denominator()};
    dest.input_clip_detection = src.clip_detection_input();
    dest.output_clip_detection = src.clip_detection_output();
    dest.master_limiter = src.master_limiter();
    dest.used_audio_inputs = src.used_audio_inputs();
    dest.used_audio_outputs = src.used_audio_outputs();

    dest.input_connections.reserve(src.input_connections_size());
    for (auto& con : src.input_connections())
    {
        dest.input_connections.push_back(to_sushi_ext(con));
    }

    dest.output_connections.reserve(src.output_connections_size());
    for (auto& con : src.output_connections())
    {
        dest.output_connections.push_back(to_sushi_ext(con));
    }
}

inline void to_grpc(sushi_rpc::PluginClass& dest, sushi::control::PluginClass& src)
{
    dest.set_name(std::move(src.name));
    dest.set_label(std::move(src.label));
    dest.set_uid(std::move(src.uid));
    dest.set_path(std::move(src.path));
    dest.mutable_type()->set_type(to_grpc(src.type));
    to_grpc(*dest.mutable_state(), src.state);
}

inline sushi::control::PluginClass to_sushi_ext(const sushi_rpc::PluginClass& src)
{
    sushi::control::PluginClass dest;
    dest.name = src.name();
    dest.label = src.label();
    dest.uid = src.uid();
    dest.path = src.path();
    dest.type = to_sushi_ext(src.type().type());
    to_sushi_ext(dest.state, src.state());
    return dest;
}

inline void to_grpc(sushi_rpc::TrackState& dest, sushi::control::TrackState& src)
{
    dest.set_name(std::move(src.name));
    dest.set_label(std::move(src.label));
    dest.set_channels(src.channels);
    dest.set_buses(src.buses);
    dest.set_thread(src.thread);
    dest.mutable_type()->set_type(to_grpc(src.type));
    to_grpc(*dest.mutable_track_state(), src.track_state);

    dest.mutable_processors()->Reserve(static_cast<int>(src.processors.size()));
    for (auto& proc : src.processors)
    {
        auto grpc_proc = dest.mutable_processors()->Add();
        to_grpc(*grpc_proc, proc);
    }
}

inline sushi::control::TrackState to_sushi_ext(const sushi_rpc::TrackState& src)
{
    sushi::control::TrackState dest;
    dest.name = src.name();
    dest.label = src.label();
    dest.channels = src.channels();
    dest.buses = src.buses();
    dest.thread = src.thread();
    dest.type = to_sushi_ext(src.type().type());
    to_sushi_ext(dest.track_state, src.track_state());

    dest.processors.reserve(src.processors_size());
    for (const auto& processor : src.processors())
    {
        dest.processors.push_back(to_sushi_ext(processor));
    }
    return dest;
}

inline void to_grpc(sushi_rpc::SessionState& dest, sushi::control::SessionState& src)
{
    to_grpc(*dest.mutable_sushi_info(), src.sushi_info);
    dest.set_save_date(std::move(src.save_date));
    to_grpc(*dest.mutable_osc_state(), src.osc_state);
    to_grpc(*dest.mutable_midi_state(), src.midi_state);
    to_grpc(*dest.mutable_engine_state(), src.engine_state);

    dest.mutable_tracks()->Reserve(static_cast<int>(src.tracks.size()));
    for (auto& track : src.tracks)
    {
        auto grpc_track = dest.mutable_tracks()->Add();
        to_grpc(*grpc_track, track);
    }
}

inline void to_sushi_ext(sushi::control::SessionState& dest, const sushi_rpc::SessionState& src)
{
    to_sushi_ext(dest.sushi_info, src.sushi_info());
    dest.save_date = src.save_date();
    to_sushi_ext(dest.osc_state, src.osc_state());
    to_sushi_ext(dest.midi_state, src.midi_state());
    to_sushi_ext(dest.engine_state, src.engine_state());

    dest.tracks.reserve(src.tracks_size());
    for (auto& track : src.tracks())
    {
        dest.tracks.push_back(to_sushi_ext(track));
    }
}
}

#endif //SUSHI_LIBRARY_CONVERSIONS_H
