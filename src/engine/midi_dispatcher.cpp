/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Handles translation of midi to internal events and midi routing
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <algorithm>

#include "base_event_dispatcher.h"
#include "engine/midi_dispatcher.h"
#include "base_engine.h"
#include "library/midi_encoder.h"
#include "logging.h"

namespace sushi {
namespace midi_dispatcher {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("midi dispatcher");

inline Event* make_note_on_event(const InputConnection& c,
                                 const midi::NoteOnMessage& msg,
                                 Time timestamp)
{
    if (msg.velocity == 0)
    {
        return new KeyboardEvent(KeyboardEvent::Subtype::NOTE_OFF, c.target, msg.channel, msg.note, 0.5f, timestamp);
    }

    float velocity = msg.velocity / static_cast<float>(midi::MAX_VALUE);
    return new KeyboardEvent(KeyboardEvent::Subtype::NOTE_ON, c.target, msg.channel, msg.note, velocity, timestamp);
}

inline Event* make_note_off_event(const InputConnection& c,
                                  const midi::NoteOffMessage& msg,
                                  Time timestamp)
{
    float velocity = msg.velocity / static_cast<float>(midi::MAX_VALUE);
    return new KeyboardEvent(KeyboardEvent::Subtype::NOTE_OFF, c.target, msg.channel, msg.note, velocity, timestamp);
}

inline Event* make_note_aftertouch_event(const InputConnection& c,
                                         const midi::PolyKeyPressureMessage& msg,
                                         Time timestamp)
{
    float pressure = msg.pressure / static_cast<float>(midi::MAX_VALUE);
    return new KeyboardEvent(KeyboardEvent::Subtype::NOTE_AFTERTOUCH, c.target, msg.channel, msg.note, pressure, timestamp);
}

inline Event* make_aftertouch_event(const InputConnection& c,
                                    const midi::ChannelPressureMessage& msg,
                                    Time timestamp)
{
    float pressure = msg.pressure / static_cast<float>(midi::MAX_VALUE);
    return new KeyboardEvent(KeyboardEvent::Subtype::AFTERTOUCH, c.target, msg.channel, pressure, timestamp);
}

inline Event* make_modulation_event(const InputConnection& c,
                                    const midi::ControlChangeMessage& msg,
                                    Time timestamp)
{
    float value = msg.value / static_cast<float>(midi::MAX_VALUE);
    return new KeyboardEvent(KeyboardEvent::Subtype::MODULATION, c.target, msg.channel, value, timestamp);
}

inline Event* make_pitch_bend_event(const InputConnection& c,
                                    const midi::PitchBendMessage& msg,
                                    Time timestamp)
{
    float value = (msg.value / static_cast<float>(midi::PITCH_BEND_MIDDLE)) - 1.0f;
    return new KeyboardEvent(KeyboardEvent::Subtype::PITCH_BEND, c.target, msg.channel, value, timestamp);
}

inline Event* make_wrapped_midi_event(const InputConnection& c,
                                      const uint8_t* data,
                                      size_t size,
                                      Time timestamp)
{
    MidiDataByte midi_data{0};
    std::copy(data, data + size, midi_data.data());
    return new KeyboardEvent(KeyboardEvent::Subtype::WRAPPED_MIDI, c.target, midi_data, timestamp);
}

inline Event* make_param_change_event(InputConnection& c,
                                      const midi::ControlChangeMessage& msg,
                                      Time timestamp)
{
    uint8_t abs_value = msg.value;
    // Maybe TODO: currently this is based on a virtual controller absolute value which is
    // initialized at 64. An alternative would be to read the parameter value from the plugin
    // and compute a change from that. We should investigate what other DAWs are doing.
    if (c.relative)
    {
        abs_value = c.virtual_abs_value;
        if (msg.value < 64u)
        {
            auto clipped_increment = std::min<uint8_t>(msg.value, 127u - abs_value);
            abs_value += clipped_increment;
        }
        else
        {
            // Two-complement encoding for negative relative changes
            auto clipped_decrease = std::min<uint8_t>(128u - msg.value, abs_value);
            abs_value -= clipped_decrease;
        }
        c.virtual_abs_value = abs_value;
    }
    float value = static_cast<float>(abs_value) / midi::MAX_VALUE * (c.max_range - c.min_range) + c.min_range;
    return new ParameterChangeEvent(ParameterChangeEvent::Subtype::FLOAT_PARAMETER_CHANGE, c.target, c.parameter, value, timestamp);
}

inline Event* make_program_change_event(const InputConnection& c,
                                        const midi::ProgramChangeMessage& msg,
                                        Time timestamp)
{
    return new ProgramChangeEvent(c.target, msg.program, timestamp);
}

MidiDispatcher::MidiDispatcher(dispatcher::BaseEventDispatcher* event_dispatcher) : _frontend(nullptr),
                                                                                    _event_dispatcher(event_dispatcher)
{
    _event_dispatcher->register_poster(this);
    _event_dispatcher->subscribe_to_keyboard_events(this);
    _event_dispatcher->subscribe_to_engine_notifications(this);
}

MidiDispatcher::~MidiDispatcher()
{
    _event_dispatcher->unsubscribe_from_keyboard_events(this);
    _event_dispatcher->unsubscribe_from_engine_notifications(this);
    _event_dispatcher->deregister_poster(this);
}


void MidiDispatcher::set_midi_outputs(int no_outputs)
{
    _midi_outputs = no_outputs;
    _enabled_clock_out = std::vector<int>(no_outputs, 0);
}

MidiDispatcherStatus MidiDispatcher::connect_cc_to_parameter(int midi_input,
                                                             ObjectId processor_id,
                                                             ObjectId parameter_id,
                                                             int cc_no,
                                                             float min_range,
                                                             float max_range,
                                                             bool use_relative_mode,
                                                             int channel)
{
    if (midi_input >= _midi_inputs || midi_input < 0 || channel > midi::MidiChannel::OMNI)
    {
        return MidiDispatcherStatus ::INVALID_MIDI_INPUT;
    }

    InputConnection connection;
    connection.target = processor_id;
    connection.parameter = parameter_id;
    connection.min_range = min_range;
    connection.max_range = max_range;
    connection.relative = use_relative_mode;
    connection.virtual_abs_value = 64;

    std::scoped_lock lock(_cc_routes_lock);

    _cc_routes[midi_input][cc_no][channel].push_back(connection);
    SUSHI_LOG_INFO("Connected parameter ID \"{}\" "
                           "(cc number \"{}\") to processor ID \"{}\"", parameter_id, cc_no, processor_id);
    return MidiDispatcherStatus::OK;
}

MidiDispatcherStatus MidiDispatcher::disconnect_cc_from_parameter(int midi_input,
                                                                  ObjectId processor_id,
                                                                  int cc_no,
                                                                  int channel)
{
    if (midi_input >= _midi_inputs || midi_input < 0 || channel > midi::MidiChannel::OMNI)
    {
        return MidiDispatcherStatus::INVALID_MIDI_INPUT;
    }

    std::scoped_lock lock(_cc_routes_lock);

    auto connections = _cc_routes.find(midi_input);
    if (connections != _cc_routes.end())
    {
        auto& connection_vector = connections->second[cc_no][channel];
        auto erase_iterator = std::remove_if(connection_vector.begin(),
                                             connection_vector.end(),
                                             [&](const auto& c)
                                             {
                                                 return c.target == processor_id;
                                             });

        connection_vector.erase(erase_iterator, connection_vector.end());
    }

    SUSHI_LOG_INFO("Disconnected "
                   "(cc number \"{}\") from processor ID \"{}\"", cc_no, processor_id);
    return MidiDispatcherStatus::OK;
}

MidiDispatcherStatus MidiDispatcher::disconnect_all_cc_from_processor(ObjectId processor_id)
{
    std::scoped_lock lock(_cc_routes_lock);

    for(auto input_i = _cc_routes.begin(); input_i != _cc_routes.end(); ++input_i)
    {
        auto& cc_channel_matrix = input_i->second;
        for(size_t cc_i = 0 ; cc_i < cc_channel_matrix.size(); ++cc_i)
        {
            auto& channels = cc_channel_matrix[cc_i];
            for(size_t channel_i = 0 ; channel_i < channels.size(); ++channel_i)
            {
                auto& connection_vector = cc_channel_matrix[cc_i][channel_i];
                auto erase_iterator = std::remove_if(connection_vector.begin(),
                                                     connection_vector.end(),
                                                     [&](const auto& c)
                                                     {
                                                         return c.target == processor_id;
                                                     });

                connection_vector.erase(erase_iterator, connection_vector.end());

                SUSHI_LOG_DEBUG("Disconnected all CC's from processor ID \"{}\"", processor_id);
            }
        }
    }

    return MidiDispatcherStatus::OK;
}

std::vector<CCInputConnection> MidiDispatcher::get_all_cc_input_connections()
{
    return _get_cc_input_connections(std::nullopt);
}

std::vector<CCInputConnection> MidiDispatcher::get_cc_input_connections_for_processor(int processor_id)
{
    return _get_cc_input_connections(processor_id);
}

MidiDispatcherStatus MidiDispatcher::connect_pc_to_processor(int midi_input,
                                                             ObjectId processor_id,
                                                             int channel)
{
    if (midi_input >= _midi_inputs || midi_input < 0 || channel > midi::MidiChannel::OMNI)
    {
        return MidiDispatcherStatus::INVALID_MIDI_INPUT;
    }

    InputConnection connection;
    connection.target = processor_id;
    connection.parameter = 0;
    connection.min_range = 0;
    connection.max_range = 0;

    std::scoped_lock lock(_pc_routes_lock);

    _pc_routes[midi_input][channel].push_back(connection);
    SUSHI_LOG_INFO("Connected program changes from MIDI port \"{}\" to processor id\"{}\"", midi_input, processor_id);
    return MidiDispatcherStatus::OK;
}

MidiDispatcherStatus MidiDispatcher::disconnect_pc_from_processor(int midi_input,
                                                                  ObjectId processor_id,
                                                                  int channel)
{
    if (midi_input >= _midi_inputs || midi_input < 0 || channel > midi::MidiChannel::OMNI)
    {
        return MidiDispatcherStatus::INVALID_MIDI_INPUT;
    }

    std::scoped_lock lock(_pc_routes_lock);

    auto connections = _pc_routes.find(midi_input);
    if (connections != _pc_routes.end())
    {
        auto& connection_vector = connections->second[channel];
        auto erase_iterator = std::remove_if(connection_vector.begin(),
                                             connection_vector.end(),
                                             [&](const auto& c)
                                             {
                                                 return c.target == processor_id;
                                             });

        connection_vector.erase(erase_iterator, connection_vector.end());
    }

    SUSHI_LOG_INFO("Disconnected program changes from MIDI port \"{}\" to processor ID \"{}\"", midi_input, processor_id);
    return MidiDispatcherStatus::OK;
}

MidiDispatcherStatus MidiDispatcher::disconnect_all_pc_from_processor(ObjectId processor_id)
{
    std::scoped_lock lock(_pc_routes_lock);

    for(auto inputs_i = _pc_routes.begin(); inputs_i != _pc_routes.end(); ++inputs_i)
    {
        auto& channels = inputs_i->second;
        for(size_t channel_i = 0 ; channel_i < channels.size(); ++channel_i)
        {
            auto& connection_vector = channels[channel_i];
            auto erase_iterator = std::remove_if(connection_vector.begin(),
                                                 connection_vector.end(),
                                                 [&](const auto& c)
                                                 {
                                                     return c.target == processor_id;
                                                 });

            connection_vector.erase(erase_iterator, connection_vector.end());
        }
    }
    SUSHI_LOG_DEBUG("Disconnected all PC's from processor ID \"{}\"", processor_id);

    return MidiDispatcherStatus::OK;
}

std::vector<PCInputConnection> MidiDispatcher::get_all_pc_input_connections()
{
    return _get_pc_input_connections(std::nullopt);
}

std::vector<PCInputConnection> MidiDispatcher::get_pc_input_connections_for_processor(int processor_id)
{
    return _get_pc_input_connections(processor_id);
}

MidiDispatcherStatus MidiDispatcher::connect_kb_to_track(int midi_input,
                                                         ObjectId track_id,
                                                         int channel)
{
    if (midi_input >= _midi_inputs || midi_input < 0 || channel > midi::MidiChannel::OMNI)
    {
        return MidiDispatcherStatus::INVALID_MIDI_INPUT;
    }

    InputConnection connection;
    connection.target = track_id;
    connection.parameter = 0;
    connection.min_range = 0;
    connection.max_range = 0;

    std::scoped_lock lock(_kb_routes_in_lock);

    _kb_routes_in[midi_input][channel].push_back(connection);
    SUSHI_LOG_INFO("Connected MIDI port \"{}\" to track ID \"{}\"", midi_input, track_id);
    return MidiDispatcherStatus::OK;
}

MidiDispatcherStatus MidiDispatcher::disconnect_kb_from_track(int midi_input,
                                                              ObjectId track_id,
                                                              int channel)
{
    // TODO: These midi_input checks here and elsewhere should be made redundant eventually - when _midi_inputs isn't used.
    if (midi_input >= _midi_inputs || midi_input < 0 || channel > midi::MidiChannel::OMNI)
    {
        return MidiDispatcherStatus::INVALID_MIDI_INPUT;
    }

    std::scoped_lock lock(_kb_routes_in_lock);

    auto connections = _kb_routes_in.find(midi_input); // All connections for the midi_input
    if (connections != _kb_routes_in.end())
    {
        auto& connection_vector = connections->second[channel];
        auto erase_iterator = std::remove_if(connection_vector.begin(),
                                             connection_vector.end(),
                                             [&](const auto& c)
                                             {
                                                 return c.target == track_id;
                                             });

        connection_vector.erase(erase_iterator, connection_vector.end());
    }

    SUSHI_LOG_INFO("Disconnected MIDI port \"{}\" from track ID \"{}\"", midi_input, track_id);
    return MidiDispatcherStatus::OK;
}

std::vector<KbdInputConnection> MidiDispatcher::get_all_kb_input_connections()
{
    std::vector<KbdInputConnection> returns;

    std::scoped_lock lock_in(_kb_routes_in_lock);

    // Adding kbd connections:
    for(auto inputs_i = _kb_routes_in.begin(); inputs_i != _kb_routes_in.end(); ++inputs_i)
    {
        auto& channels = inputs_i->second;
        for(size_t channel_i = 0 ; channel_i < channels.size(); ++channel_i)
        {
            for(auto connection = channels[channel_i].begin(); connection != channels[channel_i].end(); ++connection)
            {
                KbdInputConnection conn;
                conn.input_connection = *connection.base();
                conn.port = inputs_i->first;
                conn.channel = channel_i;
                conn.raw_midi = false;
                returns.emplace_back(conn);
            }
        }
    }

    std::scoped_lock lock_raw(_raw_routes_in_lock);

    // Adding Raw midi connections:
    for(auto inputs_i = _raw_routes_in.begin(); inputs_i != _raw_routes_in.end(); ++inputs_i)
    {
        auto channels = inputs_i->second;
        for(size_t channel_i = 0 ; channel_i < channels.size(); ++channel_i)
        {
            for(auto connection = channels[channel_i].begin(); connection != channels[channel_i].end(); ++connection)
            {
                KbdInputConnection conn;
                conn.input_connection = *connection.base();
                conn.port = inputs_i->first;
                conn.channel = channel_i;
                conn.raw_midi = true;
                returns.emplace_back(conn);
            }
        }
    }

    return returns;
}

MidiDispatcherStatus MidiDispatcher::connect_raw_midi_to_track(int midi_input,
                                                               ObjectId track_id,
                                                               int channel)
{
    if (midi_input >= _midi_inputs || midi_input < 0 || channel > midi::MidiChannel::OMNI)
    {
        return MidiDispatcherStatus::INVALID_MIDI_INPUT;
    }

    InputConnection connection;
    connection.target = track_id;
    connection.parameter = 0;
    connection.min_range = 0;
    connection.max_range = 0;

    std::scoped_lock lock(_raw_routes_in_lock);

    _raw_routes_in[midi_input][channel].push_back(connection);
    SUSHI_LOG_INFO("Connected MIDI port \"{}\" to track ID \"{}\"", midi_input, track_id);
    return MidiDispatcherStatus::OK;
}

MidiDispatcherStatus MidiDispatcher::disconnect_raw_midi_from_track(int midi_input,
                                                                    ObjectId track_id,
                                                                    int channel)
{
    if (midi_input >= _midi_inputs || midi_input < 0 || channel > midi::MidiChannel::OMNI)
    {
        return MidiDispatcherStatus::INVALID_MIDI_INPUT;
    }

    std::scoped_lock lock(_raw_routes_in_lock);

    auto connections = _raw_routes_in.find(midi_input); // All connections for the midi_input
    if (connections != _raw_routes_in.end())
    {
        auto& connection_vector = connections->second[channel];
        auto erase_iterator = std::remove_if(connection_vector.begin(),
                                             connection_vector.end(),
                                             [&](const auto& c)
                                             {
                                                 return c.target == track_id;
                                             });

        connection_vector.erase(erase_iterator, connection_vector.end());
    }

    SUSHI_LOG_INFO("Disconnected MIDI port \"{}\" from track ID \"{}\"", midi_input, track_id);
    return MidiDispatcherStatus::OK;
}

MidiDispatcherStatus MidiDispatcher::connect_track_to_output(int midi_output, ObjectId track_id, int channel)
{
    if (channel >= midi::MidiChannel::OMNI)
    {
        return MidiDispatcherStatus::INVAlID_CHANNEL;
    }
    if (midi_output >= _midi_outputs || midi_output < 0)
    {
        return MidiDispatcherStatus::INVALID_MIDI_OUTPUT;
    }

    OutputConnection connection;
    connection.channel = channel;
    connection.output = midi_output;

    // TODO: Why are these here? Why not remove from OutputConnection struct?
    connection.min_range = 1.234f;
    connection.max_range = 4.5678f;

    connection.cc_number = 123;

    std::scoped_lock lock(_kb_routes_out_lock);

    _kb_routes_out[track_id].push_back(connection);
    SUSHI_LOG_INFO("Connected MIDI from track ID \"{}\" to port \"{}\" with channel {}", track_id, midi_output, channel);
    return MidiDispatcherStatus::OK;
}

MidiDispatcherStatus MidiDispatcher::disconnect_track_from_output(int midi_output,
                                                                  ObjectId track_id,
                                                                  int channel)
{
    if (channel >= midi::MidiChannel::OMNI)
    {
        return MidiDispatcherStatus::INVAlID_CHANNEL;
    }
    if (midi_output >= _midi_outputs || midi_output < 0)
    {
        return MidiDispatcherStatus::INVALID_MIDI_OUTPUT;
    }

    std::scoped_lock lock(_kb_routes_out_lock);

    auto connections = _kb_routes_out.find(track_id);
    if (connections != _kb_routes_out.end())
    {
        auto& connection_vector = connections->second;
        auto erase_iterator = std::remove_if(connection_vector.begin(),
                                             connection_vector.end(),
                                             [&](const auto& c)
                                             {
                                                 return (c.channel == channel) && (c.output == midi_output);
                                             });

        connection_vector.erase(erase_iterator, connection_vector.end());
    }

    SUSHI_LOG_INFO("Disconnected MIDI from track ID \"{}\" to port \"{}\" with channel {}", track_id, midi_output, channel);
    return MidiDispatcherStatus::OK;
}

std::vector<KbdOutputConnection> MidiDispatcher::get_all_kb_output_connections()
{
    std::vector<KbdOutputConnection> returns;

    std::scoped_lock lock(_kb_routes_out_lock);

    for(auto outputs_i = _kb_routes_out.begin(); outputs_i != _kb_routes_out.end(); ++outputs_i)
    {
        for (auto connection = outputs_i->second.begin(); connection != outputs_i->second.end(); ++connection)
        {
            KbdOutputConnection conn;
            conn.track_id = outputs_i->first;
            conn.port = connection->output;
            conn.channel = connection->channel;
            returns.emplace_back(conn);
        }
    }

    return returns;
}

MidiDispatcherStatus MidiDispatcher::enable_midi_clock(bool enabled, int midi_output)
{
    if (static_cast<size_t>(midi_output) < _enabled_clock_out.size())
    {
        _enabled_clock_out[midi_output] = enabled;
        return MidiDispatcherStatus::OK;
    }
    SUSHI_LOG_ERROR("Failed to {} midi clock for port {}, no such port", enabled? "enable" : "disable", midi_output);
    return MidiDispatcherStatus::INVALID_MIDI_OUTPUT;
}

bool MidiDispatcher::midi_clock_enabled(int midi_output)
{
    if (static_cast<size_t>(midi_output) < _enabled_clock_out.size())
    {
        return _enabled_clock_out[midi_output];
    }
    return false;
}

void MidiDispatcher::send_midi(int port, MidiDataByte data, Time timestamp)
{
    const int channel = midi::decode_channel(data);
    const int size = data.size();
    /* Dispatch raw midi messages */
    {
        std::scoped_lock lock(_raw_routes_in_lock);

        const auto& cons = _raw_routes_in.find(port);
        if (cons != _raw_routes_in.end())
        {
            for (auto c : cons->second[midi::MidiChannel::OMNI])
            {
                _event_dispatcher->post_event(make_wrapped_midi_event(c, data.data(), size, timestamp));
            }
            for (auto c : cons->second[channel])
            {
                _event_dispatcher->post_event(make_wrapped_midi_event(c, data.data(), size, timestamp));
            }
        }
    }

    std::scoped_lock lock(_kb_routes_in_lock);

    /* Dispatch decoded midi messages */
    midi::MessageType type = midi::decode_message_type(data);
    switch (type)
    {
        case midi::MessageType::CONTROL_CHANGE:
        {
            midi::ControlChangeMessage decoded_msg = midi::decode_control_change(data);

            std::scoped_lock lock(_cc_routes_lock);

            const auto& cons = _cc_routes.find(port);
            if (cons != _cc_routes.end())
            {
                for (auto& c : cons->second[decoded_msg.controller][midi::MidiChannel::OMNI])
                {
                    _event_dispatcher->post_event(make_param_change_event(c, decoded_msg, timestamp));
                }
                for (auto& c : cons->second[decoded_msg.controller][decoded_msg.channel])
                {
                    _event_dispatcher->post_event(make_param_change_event(c, decoded_msg, timestamp));
                }
            }
            if (decoded_msg.controller == midi::MOD_WHEEL_CONTROLLER_NO)
            {
                const auto& cons = _kb_routes_in.find(port);
                if (cons != _kb_routes_in.end())
                {
                    for (auto c : cons->second[midi::MidiChannel::OMNI])
                    {
                        _event_dispatcher->post_event(make_modulation_event(c, decoded_msg, timestamp));
                    }
                    for (auto c : cons->second[decoded_msg.channel])
                    {
                        _event_dispatcher->post_event(make_modulation_event(c, decoded_msg, timestamp));
                    }
                }
            }
            break;
        }

        case midi::MessageType::NOTE_ON:
        {
            midi::NoteOnMessage decoded_msg = midi::decode_note_on(data);
            const auto& cons = _kb_routes_in.find(port);
            if (cons != _kb_routes_in.end())
            {
                for (auto c : cons->second[midi::MidiChannel::OMNI])
                {
                    _event_dispatcher->post_event(make_note_on_event(c, decoded_msg, timestamp));
                }
                for (auto c : cons->second[decoded_msg.channel])
                {
                    _event_dispatcher->post_event(make_note_on_event(c, decoded_msg, timestamp));
                }
            }
            break;
        }

        case midi::MessageType::NOTE_OFF:
        {
            midi::NoteOffMessage decoded_msg = midi::decode_note_off(data);
            const auto& cons = _kb_routes_in.find(port);
            if (cons != _kb_routes_in.end())
            {
                for (auto c : cons->second[midi::MidiChannel::OMNI])
                {
                    _event_dispatcher->post_event(make_note_off_event(c, decoded_msg, timestamp));
                }
                for (auto c : cons->second[decoded_msg.channel])
                {
                    _event_dispatcher->post_event(make_note_off_event(c, decoded_msg, timestamp));
                }
            }
            break;
        }

        case midi::MessageType::PITCH_BEND:
        {
            midi::PitchBendMessage decoded_msg = midi::decode_pitch_bend(data);
            const auto& cons = _kb_routes_in.find(port);
            if (cons != _kb_routes_in.end())
            {
                for (auto c : cons->second[midi::MidiChannel::OMNI])
                {
                    _event_dispatcher->post_event(make_pitch_bend_event(c, decoded_msg, timestamp));
                }
                for (auto c : cons->second[decoded_msg.channel])
                {
                    _event_dispatcher->post_event(make_pitch_bend_event(c, decoded_msg, timestamp));
                }
            }
            break;
        }

        case midi::MessageType::POLY_KEY_PRESSURE:
        {
            midi::PolyKeyPressureMessage decoded_msg = midi::decode_poly_key_pressure(data);
            const auto& cons = _kb_routes_in.find(port);
            if (cons != _kb_routes_in.end())
            {
                for (auto c : cons->second[midi::MidiChannel::OMNI])
                {
                    _event_dispatcher->post_event(make_note_aftertouch_event(c, decoded_msg, timestamp));
                }
                for (auto c : cons->second[decoded_msg.channel])
                {
                    _event_dispatcher->post_event(make_note_aftertouch_event(c, decoded_msg, timestamp));
                }
            }
            break;
        }

        case midi::MessageType::CHANNEL_PRESSURE:
        {
            midi::ChannelPressureMessage decoded_msg = midi::decode_channel_pressure(data);
            const auto& cons = _kb_routes_in.find(port);
            if (cons != _kb_routes_in.end())
            {
                for (auto c : cons->second[midi::MidiChannel::OMNI])
                {
                    _event_dispatcher->post_event(make_aftertouch_event(c, decoded_msg, timestamp));
                }
                for (auto c : cons->second[decoded_msg.channel])
                {
                    _event_dispatcher->post_event( make_aftertouch_event(c, decoded_msg, timestamp));
                }
            }
            break;
        }

        case midi::MessageType::PROGRAM_CHANGE:
        {
            midi::ProgramChangeMessage decoded_msg = midi::decode_program_change(data);

            std::scoped_lock lock(_pc_routes_lock);
            const auto& cons = _pc_routes.find(port);
            if (cons != _pc_routes.end())
            {
                for (auto c : cons->second[midi::MidiChannel::OMNI])
                {
                    _event_dispatcher->post_event(make_program_change_event(c, decoded_msg, timestamp));
                }
                for (auto c : cons->second[decoded_msg.channel])
                {
                    _event_dispatcher->post_event(make_program_change_event(c, decoded_msg, timestamp));
                }
            }
            break;
        }

        default:
            break;
    }
}

int MidiDispatcher::process(Event* event)
{
    if (event->is_keyboard_event())
    {
        std::scoped_lock lock(_kb_routes_out_lock);
        auto typed_event = static_cast<KeyboardEvent*>(event);
        const auto& cons = _kb_routes_out.find(typed_event->processor_id());
        if (cons != _kb_routes_out.end())
        {
            for (const OutputConnection& c : cons->second)
            {
                MidiDataByte midi_data;
                switch (typed_event->subtype())
                {
                    case KeyboardEvent::Subtype::NOTE_ON:
                        midi_data = midi::encode_note_on(c.channel, typed_event->note(), typed_event->velocity());
                        break;
                    case KeyboardEvent::Subtype::NOTE_OFF:
                        midi_data = midi::encode_note_off(c.channel, typed_event->note(), typed_event->velocity());
                        break;
                    case KeyboardEvent::Subtype::NOTE_AFTERTOUCH:
                        midi_data = midi::encode_poly_key_pressure(c.channel, typed_event->note(), typed_event->velocity());
                        break;
                    case KeyboardEvent::Subtype::AFTERTOUCH:
                        midi_data = midi::encode_channel_pressure(c.channel, typed_event->value());
                        break;
                    case KeyboardEvent::Subtype::PITCH_BEND:
                        midi_data = midi::encode_pitch_bend(c.channel, typed_event->value());
                        break;
                    case KeyboardEvent::Subtype::MODULATION:
                        midi_data = midi::encode_control_change(c.channel, midi::MOD_WHEEL_CONTROLLER_NO, typed_event->value());
                        break;
                    case KeyboardEvent::Subtype::WRAPPED_MIDI:
                        midi_data = typed_event->midi_data();
                }
                SUSHI_LOG_DEBUG("Dispatching midi [{:x} {:x} {:x} {:x}], timestamp: {}",
                                midi_data[0], midi_data[1], midi_data[2], midi_data[3], event->time().count());
                _frontend->send_midi(c.output, midi_data, event->time());
            }
        }
        return EventStatus::HANDLED_OK;
    }
    else if (event->is_engine_notification())
    {
        _handle_engine_notification(static_cast<EngineNotificationEvent*>(event));
    }

    return EventStatus::NOT_HANDLED;
}

std::vector<CCInputConnection> MidiDispatcher::_get_cc_input_connections(std::optional<int> processor_id_filter)
{
    std::vector<CCInputConnection> returns;

    std::scoped_lock lock(_cc_routes_lock);

    for(auto input_i = _cc_routes.begin(); input_i != _cc_routes.end(); ++input_i)
    {
        auto& cc_channel_matrix = input_i->second;
        for(size_t cc_i = 0 ; cc_i < cc_channel_matrix.size(); ++cc_i)
        {
            auto& channels = cc_channel_matrix[cc_i];
            for(size_t channel_i = 0 ; channel_i < channels.size(); ++channel_i)
            {
                auto& connections = cc_channel_matrix[cc_i][channel_i];
                for (auto connection = connections.begin(); connection != connections.end(); ++connection)
                {
                    if (!processor_id_filter.has_value() ||
                        processor_id_filter == connection->target)
                    {
                        CCInputConnection conn;
                        conn.input_connection = *(connection.base());
                        conn.channel = channel_i;
                        conn.port = input_i->first;
                        conn.cc = cc_i;
                        returns.emplace_back(conn);
                    }
                }
            }
        }
    }

    return returns;
}

std::vector<PCInputConnection> MidiDispatcher::_get_pc_input_connections(std::optional<int> processor_id_filter)
{
    std::vector<PCInputConnection> returns;

    std::scoped_lock lock(_pc_routes_lock);

    for(auto inputs_i = _pc_routes.begin(); inputs_i != _pc_routes.end(); ++inputs_i)
    {
        auto& channels = inputs_i->second;
        for(size_t channel_i = 0 ; channel_i < channels.size(); ++channel_i)
        {
            for(auto connection = channels[channel_i].begin(); connection != channels[channel_i].end(); ++connection)
            {
                if (!processor_id_filter.has_value() ||
                    processor_id_filter == connection->target)
                {
                    PCInputConnection conn;
                    conn.processor_id = connection->target;
                    conn.channel = channel_i;
                    conn.port = inputs_i->first;
                    returns.emplace_back(conn);
                }
            }
        }
    }

    return returns;
}

bool MidiDispatcher::_handle_audio_graph_notification(const AudioGraphNotificationEvent* event)
{
    switch (event->action())
    {
        case AudioGraphNotificationEvent::Action::PROCESSOR_DELETED:
        {
            auto processor_id = event->processor();

            disconnect_all_cc_from_processor(processor_id);

            disconnect_all_pc_from_processor(processor_id);

            SUSHI_LOG_DEBUG("MidiController received a PROCESSOR_DELETED notification for processor {}",
                            event->processor());
            break;
        }
        case AudioGraphNotificationEvent::Action::TRACK_DELETED:
        {
            auto track_id = event->track();

            disconnect_all_cc_from_processor(track_id);
            disconnect_all_pc_from_processor(track_id);

            auto input_connections = get_all_kb_input_connections();
            auto inputs_found = std::find_if(input_connections.begin(),
                                             input_connections.end(),
                                             [&](const auto& connection)
                                             {
                                                 return connection.input_connection.target == track_id;
                                             });

            while (inputs_found != input_connections.end())
            {
                disconnect_kb_from_track(inputs_found->port,
                                         track_id,
                                         inputs_found->channel);

                disconnect_raw_midi_from_track(inputs_found->port,
                                               track_id,
                                               inputs_found->channel);

                inputs_found++;
            }

            auto output_connections = get_all_kb_output_connections();
            auto outputs_found = std::find_if(output_connections.begin(),
                                              output_connections.end(),
                                              [&](const auto& connection)
                                              {
                                                  return connection.track_id == track_id;
                                              });

            while (outputs_found != output_connections.end())
            {
                disconnect_track_from_output(outputs_found->port,
                                             track_id,
                                             outputs_found->channel);
                outputs_found++;
            }

            SUSHI_LOG_DEBUG("MidiController received a TRACK_DELETED notification for track {}", event->track());
            break;
        }
        default:
            break;
    }

    return EventStatus::HANDLED_OK;
}

bool MidiDispatcher::_handle_engine_notification(const EngineNotificationEvent* event)
{
    if (event->is_audio_graph_notification())
    {
        return _handle_audio_graph_notification(static_cast<const AudioGraphNotificationEvent*>(event));
    }
    else if (event->is_playing_mode_notification())
    {
        return _handle_transport_notification(static_cast<const PlayingModeNotificationEvent*>(event));
    }
    else if (event->is_timing_tick_notification())
    {
        return _handle_tick_notification(static_cast<const EngineTimingTickNotificationEvent*>(event));
    }
    return false;
}

bool MidiDispatcher::_handle_transport_notification(const PlayingModeNotificationEvent* event)
{
    switch (event->mode())
    {
        case PlayingMode::PLAYING:
            for (int i = 0; i < _midi_outputs; ++i)
            {
                if (_enabled_clock_out[i])
                {
                    SUSHI_LOG_DEBUG("Sending midi start message");
                    _frontend->send_midi(i, midi::encode_start_message(), event->time());
                }
            }
            break;

        case PlayingMode::STOPPED:
            for (int i = 0; i < _midi_outputs; ++i)
            {
                if (_enabled_clock_out[i])
                {
                    SUSHI_LOG_DEBUG("Sending midi stop message");
                    _frontend->send_midi(i, midi::encode_stop_message(), event->time());
                }
            }
            break;

        default:
            break;
    }
    return EventStatus::HANDLED_OK;
}

bool MidiDispatcher::_handle_tick_notification(const EngineTimingTickNotificationEvent* event)
{
    for (int i = 0; i < _midi_outputs; ++i)
    {
        if (_enabled_clock_out[i])
        {
            _frontend->send_midi(i, midi::encode_timing_clock(), event->time());
        }
    }
    return EventStatus::HANDLED_OK;
}

} // end namespace midi_dispatcher
} // end namespace sushi
