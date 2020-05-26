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

#include "engine/midi_dispatcher.h"
#include "library/midi_encoder.h"
#include "logging.h"

namespace sushi {
namespace midi_dispatcher {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("midi dispatcher");

inline Event* make_note_on_event(const InputConnection &c,
                                 const midi::NoteOnMessage &msg,
                                 Time timestamp)
{
    if (msg.velocity == 0)
    {
        return new KeyboardEvent(KeyboardEvent::Subtype::NOTE_OFF, c.target, msg.channel, msg.note, 0.5f, timestamp);
    }

    float velocity = msg.velocity / static_cast<float>(midi::MAX_VALUE);
    return new KeyboardEvent(KeyboardEvent::Subtype::NOTE_ON, c.target, msg.channel, msg.note, velocity, timestamp);
}

inline Event* make_note_off_event(const InputConnection &c,
                                  const midi::NoteOffMessage &msg,
                                  Time timestamp)
{
    float velocity = msg.velocity / static_cast<float>(midi::MAX_VALUE);
    return new KeyboardEvent(KeyboardEvent::Subtype::NOTE_OFF, c.target, msg.channel, msg.note, velocity, timestamp);
}

inline Event* make_note_aftertouch_event(const InputConnection &c,
                                         const midi::PolyKeyPressureMessage &msg,
                                         Time timestamp)
{
    float pressure = msg.pressure / static_cast<float>(midi::MAX_VALUE);
    return new KeyboardEvent(KeyboardEvent::Subtype::NOTE_AFTERTOUCH, c.target, msg.channel, msg.note, pressure, timestamp);
}

inline Event* make_aftertouch_event(const InputConnection &c,
                                    const midi::ChannelPressureMessage &msg,
                                    Time timestamp)
{
    float pressure = msg.pressure / static_cast<float>(midi::MAX_VALUE);
    return new KeyboardEvent(KeyboardEvent::Subtype::AFTERTOUCH, c.target, msg.channel, pressure, timestamp);
}

inline Event* make_modulation_event(const InputConnection &c,
                                    const midi::ControlChangeMessage &msg,
                                    Time timestamp)
{
    float value = msg.value / static_cast<float>(midi::MAX_VALUE);
    return new KeyboardEvent(KeyboardEvent::Subtype::MODULATION, c.target, msg.channel, value, timestamp);
}

inline Event* make_pitch_bend_event(const InputConnection &c,
                                    const midi::PitchBendMessage &msg,
                                    Time timestamp)
{
    float value = (msg.value / static_cast<float>(midi::PITCH_BEND_MIDDLE)) - 1.0f;
    return new KeyboardEvent(KeyboardEvent::Subtype::PITCH_BEND, c.target, msg.channel, value, timestamp);
}

inline Event* make_wrapped_midi_event(const InputConnection &c,
                                      const uint8_t* data,
                                      size_t size,
                                      Time timestamp)
{
    MidiDataByte midi_data{0};
    std::copy(data, data + size, midi_data.data());
    return new KeyboardEvent(KeyboardEvent::Subtype::WRAPPED_MIDI, c.target, midi_data, timestamp);
}

inline Event* make_param_change_event(InputConnection &c,
                                      const midi::ControlChangeMessage &msg,
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

inline Event* make_program_change_event(const InputConnection &c,
                                        const midi::ProgramChangeMessage &msg,
                                        Time timestamp)
{
    return new ProgramChangeEvent(c.target, msg.program, timestamp);
}

MidiDispatcher::MidiDispatcher(engine::BaseEngine* engine) : _engine(engine),
                                                             _frontend(nullptr)
{
    // TODO: We can pass the event dispatcher directly and avoid the engine dependency
    _event_dispatcher = _engine->event_dispatcher();
    _event_dispatcher->register_poster(this);
    _event_dispatcher->subscribe_to_keyboard_events(this);
}

MidiDispatcher::~MidiDispatcher()
{
    _event_dispatcher->unsubscribe_from_keyboard_events(this);
    _event_dispatcher->deregister_poster(this);
}

MidiDispatcherStatus MidiDispatcher::connect_cc_to_parameter(int midi_input,
                                                             const std::string &processor_name,
                                                             const std::string &parameter_name,
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
    auto processor = _engine->processor_container()->processor(processor_name);
    if (processor == nullptr)
    {
        return MidiDispatcherStatus::INVALID_PROCESSOR;
    }
    auto parameter = processor->parameter_from_name(parameter_name);
    if (parameter == nullptr)
    {
        return MidiDispatcherStatus::INVALID_PARAMETER;
    }

    InputConnection connection;
    connection.target = processor->id();
    connection.parameter = parameter->id();
    connection.min_range = min_range;
    connection.max_range = max_range;
    connection.relative = use_relative_mode;
    connection.virtual_abs_value = 64;

    std::scoped_lock lock(_cc_routes_lock);

    _cc_routes[midi_input][cc_no][channel].push_back(connection);
    SUSHI_LOG_INFO("Connected parameter \"{}\" "
                           "(cc number \"{}\") to processor \"{}\"", parameter_name, cc_no, processor_name);
    return MidiDispatcherStatus::OK;
}

MidiDispatcherStatus MidiDispatcher::disconnect_cc_from_parameter(int midi_input,
                                                                  const std::string& processor_name,
                                                                  int cc_no,
                                                                  int channel)
{
    if (midi_input >= _midi_inputs || midi_input < 0 || channel > midi::MidiChannel::OMNI)
    {
        return MidiDispatcherStatus::INVALID_MIDI_INPUT;
    }
    auto processor = _engine->processor_container()->processor(processor_name);
    if (processor == nullptr)
    {
        return MidiDispatcherStatus::INVALID_PROCESSOR;
    }

    std::scoped_lock lock(_cc_routes_lock);

    auto connections = _cc_routes.find(midi_input);
    if (connections != _cc_routes.end())
    {
        for (auto i = connections->second[cc_no][channel].begin(); i != connections->second[cc_no][channel].end(); ++i)
        {
            if (i->target == processor->id())
            {
                connections->second[cc_no][channel].erase(i);
                // Assuming only a single connection per target.
                break;
            }
        }
    }

    SUSHI_LOG_INFO("Disconnected "
                   "(cc number \"{}\") from processor \"{}\"", cc_no, processor_name);
    return MidiDispatcherStatus::OK;
}

std::vector<CC_InputConnection> MidiDispatcher::get_all_cc_input_connections()
{
    return _get_cc_input_connections(std::nullopt);
}

std::vector<CC_InputConnection> MidiDispatcher::get_cc_input_connections_for_processor(int processor_id)
{
    return _get_cc_input_connections(processor_id);
}

MidiDispatcherStatus MidiDispatcher::connect_pc_to_processor(int midi_input,
                                                             const std::string& processor_name,
                                                             int channel)
{
    if (midi_input >= _midi_inputs || midi_input < 0 || channel > midi::MidiChannel::OMNI)
    {
        return MidiDispatcherStatus::INVALID_MIDI_INPUT;
    }
    auto processor = _engine->processor_container()->processor(processor_name);
    if (processor == nullptr)
    {
        return MidiDispatcherStatus::INVALID_CHAIN_NAME;
    }
    InputConnection connection;
    connection.target = processor->id();
    connection.parameter = 0;
    connection.min_range = 0;
    connection.max_range = 0;

    std::scoped_lock lock(_pc_routes_lock);

    _pc_routes[midi_input][channel].push_back(connection);
    SUSHI_LOG_INFO("Connected program changes from MIDI port \"{}\" to processor \"{}\"", midi_input, processor_name);
    return MidiDispatcherStatus::OK;
}

MidiDispatcherStatus MidiDispatcher::disconnect_pc_from_processor(int midi_input,
                                                                  const std::string& processor_name,
                                                                  int channel)
{
    if (midi_input >= _midi_inputs || midi_input < 0 || channel > midi::MidiChannel::OMNI)
    {
        return MidiDispatcherStatus::INVALID_MIDI_INPUT;
    }
    auto processor = _engine->processor_container()->processor(processor_name);
    if (processor == nullptr)
    {
        return MidiDispatcherStatus::INVALID_CHAIN_NAME;
    }

    std::scoped_lock lock(_pc_routes_lock);

    auto connections = _pc_routes.find(midi_input);
    if (connections != _pc_routes.end())
    {
        for (auto i = connections->second[channel].begin(); i != connections->second[channel].end(); ++i)
        {
            if (i->target == processor->id())
            {
                connections->second[channel].erase(i);
                // Assuming only a single connection per target.
                break;
            }
        }
    }

    SUSHI_LOG_INFO("Disconnected program changes from MIDI port \"{}\" to processor \"{}\"", midi_input, processor_name);
    return MidiDispatcherStatus::OK;
}

std::vector<PC_InputConnection> MidiDispatcher::get_all_pc_input_connections()
{
    return _get_pc_input_connections(std::nullopt);
}

std::vector<PC_InputConnection> MidiDispatcher::get_pc_input_connections_for_processor(int processor_id)
{
    return _get_pc_input_connections(processor_id);
}

MidiDispatcherStatus MidiDispatcher::connect_kb_to_track(int midi_input,
                                                         const std::string &track_name,
                                                         int channel)
{
    if (midi_input >= _midi_inputs || midi_input < 0 || channel > midi::MidiChannel::OMNI)
    {
        return MidiDispatcherStatus::INVALID_MIDI_INPUT;
    }
    auto track = _engine->processor_container()->track(track_name);
    if (track == nullptr)
    {
        return MidiDispatcherStatus::INVALID_CHAIN_NAME;
    }
    InputConnection connection;
    connection.target = track->id();
    connection.parameter = 0;
    connection.min_range = 0;
    connection.max_range = 0;

    std::scoped_lock lock(_kb_routes_in_lock);

    _kb_routes_in[midi_input][channel].push_back(connection);
    SUSHI_LOG_INFO("Connected MIDI port \"{}\" to track \"{}\"", midi_input, track_name);
    return MidiDispatcherStatus::OK;
}

MidiDispatcherStatus MidiDispatcher::disconnect_kb_from_track(int midi_input,
                                                              const std::string& track_name,
                                                              int channel)
{
    // TODO: These midi_input checks should be made redundant eventually - when _midi_inputs isn't used.
    if (midi_input >= _midi_inputs || midi_input < 0 || channel > midi::MidiChannel::OMNI)
    {
        return MidiDispatcherStatus::INVALID_MIDI_INPUT;
    }
    auto track = _engine->processor_container()->track(track_name);
    if (track == nullptr)
    {
        return MidiDispatcherStatus::INVALID_CHAIN_NAME;
    }

    std::scoped_lock lock(_kb_routes_in_lock);

    auto connections = _kb_routes_in.find(midi_input); // All connections for the midi_input
    if (connections != _kb_routes_in.end())
    {
        for (auto i = connections->second[channel].begin(); i != connections->second[channel].end(); ++i)
        {
            if (i->target == track->id())
            {
                connections->second[channel].erase(i);
                // Assuming only a single connection per target.
                break;
            }
        }
    }

    SUSHI_LOG_INFO("Disconnected MIDI port \"{}\" from track \"{}\"", midi_input, track_name);
    return MidiDispatcherStatus::OK;
}

std::vector<Kbd_InputConnection> MidiDispatcher::get_all_kb_input_connections()
{
    std::vector<Kbd_InputConnection> returns;

    std::scoped_lock lock_in(_kb_routes_in_lock);

    // Adding kbd connections:
    for(auto inputs_i = _kb_routes_in.begin(); inputs_i != _kb_routes_in.end(); ++inputs_i)
    {
        auto& channels = inputs_i->second;
        for(size_t channel_i = 0 ; channel_i < channels.size(); ++channel_i)
        {
            for(auto connection = channels[channel_i].begin(); connection != channels[channel_i].end(); ++connection)
            {
                Kbd_InputConnection conn;
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
                Kbd_InputConnection conn;
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
                                                               const std::string &track_name,
                                                               int channel)
{
    if (midi_input >= _midi_inputs || midi_input < 0 || channel > midi::MidiChannel::OMNI)
    {
        return MidiDispatcherStatus::INVALID_MIDI_INPUT;
    }
    auto track = _engine->processor_container()->track(track_name);
    if (track == nullptr)
    {
        return MidiDispatcherStatus::INVALID_CHAIN_NAME;
    }
    InputConnection connection;
    connection.target = track->id();
    connection.parameter = 0;
    connection.min_range = 0;
    connection.max_range = 0;

    std::scoped_lock lock(_raw_routes_in_lock);

    _raw_routes_in[midi_input][channel].push_back(connection);
    SUSHI_LOG_INFO("Connected MIDI port \"{}\" to track \"{}\"", midi_input, track_name);
    return MidiDispatcherStatus::OK;
}

MidiDispatcherStatus MidiDispatcher::disconnect_raw_midi_from_track(int midi_input,
                                                                    const std::string& track_name,
                                                                    int channel)
{
    if (midi_input >= _midi_inputs || midi_input < 0 || channel > midi::MidiChannel::OMNI)
    {
        return MidiDispatcherStatus::INVALID_MIDI_INPUT;
    }
    auto track = _engine->processor_container()->track(track_name);
    if (track == nullptr)
    {
        return MidiDispatcherStatus::INVALID_CHAIN_NAME;
    }

    std::scoped_lock lock(_raw_routes_in_lock);

    auto connections = _raw_routes_in.find(midi_input); // All connections for the midi_input
    if (connections != _raw_routes_in.end())
    {
        for (auto i = connections->second[channel].begin(); i != connections->second[channel].end(); ++i)
        {
            if (i->target == track->id())
            {
                connections->second[channel].erase(i);
                // Assuming only a single connection per target.
                break;
            }
        }
    }

    SUSHI_LOG_INFO("Disconnected MIDI port \"{}\" from track \"{}\"", midi_input, track_name);
    return MidiDispatcherStatus::OK;
}

MidiDispatcherStatus MidiDispatcher::connect_track_to_output(int midi_output, const std::string &track_name, int channel)
{
    if (channel >= midi::MidiChannel::OMNI)
    {
        return MidiDispatcherStatus::INVAlID_CHANNEL;
    }
    if (midi_output >= _midi_outputs || midi_output < 0)
    {
        return MidiDispatcherStatus::INVALID_MIDI_OUTPUT;
    }
    auto track = _engine->processor_container()->track(track_name);
    if (track == nullptr)
    {
        return MidiDispatcherStatus::INVALID_CHAIN_NAME;
    }
    OutputConnection connection;
    connection.channel = channel;
    connection.output = midi_output;
    connection.min_range = 1.234f; // TODO: Why are these here? Why not remove from OutputConnection struct?
    connection.max_range = 4.5678f;
    connection.cc_number = 123;

    std::scoped_lock lock(_kb_routes_out_lock);

    _kb_routes_out[track->id()].push_back(connection);
    SUSHI_LOG_INFO("Connected MIDI from track \"{}\" to port \"{}\" with channel {}", track_name, midi_output, channel);
    return MidiDispatcherStatus::OK;
}

MidiDispatcherStatus MidiDispatcher::disconnect_track_from_output(int midi_output,
                                                                  const std::string& track_name,
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
    auto track = _engine->processor_container()->track(track_name);
    if (track == nullptr)
    {
        return MidiDispatcherStatus::INVALID_CHAIN_NAME;
    }

    std::scoped_lock lock(_kb_routes_out_lock);

    auto connections = _kb_routes_out.find(track->id());
    if (connections != _kb_routes_out.end())
    {
        for (auto connection = connections->second.begin(); connection != connections->second.end(); ++connection)
        {
            if ((connection->channel == channel) && (connection->output == midi_output))
            {
                connections->second.erase(connection);
                // Assuming only a single connection per target.
                break;
            }
        }
    }

    SUSHI_LOG_INFO("Disconnected MIDI from track \"{}\" to port \"{}\" with channel {}", track_name, midi_output, channel);
    return MidiDispatcherStatus::OK;
}

std::vector<Kbd_OutputConnection> MidiDispatcher::get_all_kb_output_connections()
{
    std::vector<Kbd_OutputConnection> returns;

    std::scoped_lock lock(_kb_routes_out_lock);

    for(auto outputs_i = _kb_routes_out.begin(); outputs_i != _kb_routes_out.end(); ++outputs_i)
    {
        for (auto connection = outputs_i->second.begin(); connection != outputs_i->second.end(); ++connection)
        {
            Kbd_OutputConnection conn;
            conn.track_id = outputs_i->first;
            conn.port = connection->output;
            conn.channel = connection->channel;
            returns.emplace_back(conn);
        }
    }

    return returns;
}

void MidiDispatcher::send_midi(int port, MidiDataByte data, Time timestamp)
{
    int channel = midi::decode_channel(data);
    int size = data.size();
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
    std::scoped_lock lock(_kb_routes_out_lock);

    if (event->is_keyboard_event())
    {
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
    return EventStatus::NOT_HANDLED;
}

std::vector<CC_InputConnection> MidiDispatcher::_get_cc_input_connections(std::optional<int> processor_id_filter)
{
    std::vector<CC_InputConnection> returns;

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
                    if(!processor_id_filter.has_value() ||
                       processor_id_filter == connection->target)
                    {
                        CC_InputConnection conn;
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

std::vector<PC_InputConnection> MidiDispatcher::_get_pc_input_connections(std::optional<int> processor_id_filter)
{
    std::vector<PC_InputConnection> returns;

    std::scoped_lock lock(_pc_routes_lock);
    for(auto inputs_i = _pc_routes.begin(); inputs_i != _pc_routes.end(); ++inputs_i)
    {
        auto& channels = inputs_i->second;
        for(size_t channel_i = 0 ; channel_i < channels.size(); ++channel_i)
        {
            for(auto connection = channels[channel_i].begin(); connection != channels[channel_i].end(); ++connection)
            {
                if(!processor_id_filter.has_value() ||
                   processor_id_filter == connection->target)
                {
                    PC_InputConnection conn;
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

} // end namespace midi_dispatcher
} // end namespace sushi
