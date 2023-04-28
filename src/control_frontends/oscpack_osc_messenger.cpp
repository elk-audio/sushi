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

#include <sstream>

#include "logging.h"
#include "osc_utils.h"
#include "oscpack_osc_messenger.h"

namespace sushi {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("osc frontend");

namespace osc
{

// The below are informed by this:
// "According to the C++0x standard, section 20.3.3.26, std::pair has an operator< defined such that for two pairs x and y, it returns:"
// x.first < y.first || (!(y.first < x.first) && x.second < y.second)

// These two suffice - it doesn't require ==, or >, =>, etc.
// If I didn't use LightKey, but std::pair, my overrides would not be hit.
// Stepping through, I've ensured the below engage the string .compare(char*) overload.
bool operator<(const std::pair<std::string, std::string>& fat, const LightKey& light)
{
    return fat.first < light.first || (!(light.first < fat.first) && fat.second < light.second);
}

bool operator<(const LightKey& light, const std::pair<std::string, std::string>& fat)
{
    return light.first < fat.first || (!(fat.first < light.first) && light.second < fat.second);
}

OscpackOscMessenger::OscpackOscMessenger(int receive_port,
                                         int send_port,
                                         const std::string& send_ip) : BaseOscMessenger(receive_port,
                                                                                        send_port,
                                                                                        send_ip)
{}

OscpackOscMessenger::~OscpackOscMessenger()
{
    if (_osc_initialized)
    {
        _osc_initialized = false;
    }
}

bool OscpackOscMessenger::init()
{
    bool status = true;

    try
    {
        _transmit_socket = std::make_unique<UdpTransmitSocket>(IpEndpointName(_send_ip.c_str(), _send_port));
    }
    catch (oscpack::Exception& e)
    {
        status = false;
        SUSHI_LOG_ERROR("OSC transmitter failed instantiating for IP {} and port {}, with message: ",
                        _send_ip.c_str(),
                        _send_port,
                        e.what());
    }

    try
    {
        _receive_socket = std::make_unique<UdpListeningReceiveSocket>(IpEndpointName(IpEndpointName::ANY_ADDRESS, _receive_port),
                                                                      this);
    }
    catch (oscpack::Exception& e)
    {
        status = false;
        SUSHI_LOG_ERROR("OSC receiver failed instantiating for port {}, with message: ",
                        _receive_port,
                        e.what());
    }

    return status;
}

void OscpackOscMessenger::run()
{
    _osc_receive_worker = std::thread(&OscpackOscMessenger::_osc_receiving_worker, this);
}

void OscpackOscMessenger::stop()
{
    if (_receive_socket.get() != nullptr)
    {
        _receive_socket->AsynchronousBreak();
    }

    if (_osc_receive_worker.joinable())
    {
        _osc_receive_worker.join();
    }
}

void* OscpackOscMessenger::add_method(const char* address_pattern,
                                      const char* type_tag_string,
                                      OscMethodType type,
                                      const void* callback_data)
{
    LightKey key(address_pattern,  type_tag_string);
    auto iterator = _registered_messages.find(key);

    if (iterator != _registered_messages.end())
    {
        return reinterpret_cast<void*>(-1);
    }

    MessageRegistration reg;

    // Casting away const to address OSC library API incompatibilities.
    // It is later used unchanged.
    reg.callback_data = const_cast<void*>(callback_data);

    reg.type = type;
    reg.handle = _last_generated_handle;

    _registered_messages[{address_pattern, type_tag_string}] = reg;

    _last_generated_handle++;

    auto to_return = reinterpret_cast<void*>(reg.handle);
    assert(sizeof(void*) == sizeof(OSC_CALLBACK_HANDLE));
    return to_return;
}

void OscpackOscMessenger::delete_method(void* handle)
{
    auto itr = _registered_messages.begin();
    while (itr != _registered_messages.end())
    {
        auto id = itr->second.handle;
        auto method_int = reinterpret_cast<OSC_CALLBACK_HANDLE>(handle);
        if (id == method_int)
        {
            _registered_messages.erase(itr);
            break;
        }
        else
        {
            ++itr;
        }
    }
}

void OscpackOscMessenger::send(const char* address_pattern, float payload)
{
    oscpack::OutboundPacketStream p(_output_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p << oscpack::BeginMessage(address_pattern) << payload  << oscpack::EndMessage;
    _transmit_socket->Send(p.Data(), p.Size());
}

void OscpackOscMessenger::send(const char* address_pattern, int payload)
{
    oscpack::OutboundPacketStream p(_output_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p << oscpack::BeginMessage(address_pattern) << payload  << oscpack::EndMessage;
    _transmit_socket->Send(p.Data(), p.Size());
}

void OscpackOscMessenger::send(const char* address_pattern, const std::string& payload)
{
    oscpack::OutboundPacketStream p(_output_buffer, OSC_OUTPUT_BUFFER_SIZE);
    p << oscpack::BeginMessage(address_pattern) << payload.c_str()  << oscpack::EndMessage;
    _transmit_socket->Send(p.Data(), p.Size());
}

void OscpackOscMessenger::ProcessMessage(const oscpack::ReceivedMessage& m, const IpEndpointName& /*remoteEndpoint*/)
{
    try
    {
        LightKey key(m.AddressPattern(),  m.TypeTags());
        auto iterator = _registered_messages.find(key);

        if (iterator != _registered_messages.end())
        {
            auto reg = iterator->second;

            switch (reg.type)
            {
                case OscMethodType::SEND_PARAMETER_CHANGE_EVENT:
                {
                    _send_parameter_change_event(m, reg.callback_data);
                    break;
                }
                case OscMethodType::SEND_PROPERTY_CHANGE_EVENT:
                {
                    _send_property_change_event(m, reg.callback_data);
                    break;
                }
                case OscMethodType::SEND_BYPASS_STATE_EVENT:
                {
                    _send_bypass_state_event(m, reg.callback_data);
                    break;
                }
                case OscMethodType::SEND_KEYBOARD_NOTE_EVENT:
                {
                    _send_keyboard_note_event(m, reg.callback_data);
                    break;
                }
                case OscMethodType::SEND_KEYBOARD_MODULATION_EVENT:
                {
                    _send_keyboard_modulation_event(m, reg.callback_data);
                    break;
                }
                case OscMethodType::SEND_PROGRAM_CHANGE_EVENT:
                {
                    _send_program_change_event(m, reg.callback_data);
                    break;
                }
                case OscMethodType::SET_TEMPO:
                {
                    _set_tempo(m, reg.callback_data);
                    break;
                }
                case OscMethodType::SET_TIME_SIGNATURE:
                {
                    _set_time_signature(m, reg.callback_data);
                    break;
                }
                case OscMethodType::SET_PLAYING_MODE:
                {
                    _set_playing_mode(m, reg.callback_data);
                    break;
                }
                case OscMethodType::SET_TEMPO_SYNC_MODE:
                {
                    _set_tempo_sync_mode(m, reg.callback_data);
                    break;
                }
                case OscMethodType::SET_TIMING_STATISTICS_ENABLED:
                {
                    _set_timing_statistics_enabled(m, reg.callback_data);
                    break;
                }
                case OscMethodType::RESET_TIMING_STATISTICS:
                {
                    _reset_timing_statistics(m, reg.callback_data);
                    break;
                }
                default:
                {
                    SUSHI_LOG_INFO("Unrecognised OSC message received: {}", m.AddressPattern());
                }
            }
        }
    }
    catch (oscpack::Exception& e)
    {
        // Any parsing errors such as unexpected argument types, or missing arguments get thrown as exceptions.
        SUSHI_LOG_ERROR("Exception while parsing message: {}: {}", m.AddressPattern(), e.what());
    }
}

void OscpackOscMessenger::_osc_receiving_worker()
{
    _receive_socket->Run();
}

void OscpackOscMessenger::_send_parameter_change_event(const oscpack::ReceivedMessage& m, void* user_data) const
{
    oscpack::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
    float value = (arg++)->AsFloat();
    auto connection = static_cast<control_frontend::OscConnection*>(user_data);
    auto controller = connection->controller->parameter_controller();
    controller->set_parameter_value(connection->processor, connection->parameter, value);

    SUSHI_LOG_DEBUG("Sending parameter {} on processor {} change to {}.", connection->parameter, connection->processor, value);
}

void OscpackOscMessenger::_send_property_change_event(const oscpack::ReceivedMessage& m, void* user_data) const
{
    oscpack::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
    auto value = (arg++)->AsString();
    auto connection = static_cast<control_frontend::OscConnection*>(user_data);
    auto controller = connection->controller->parameter_controller();
    controller->set_property_value(connection->processor, connection->parameter, value);

    SUSHI_LOG_DEBUG("Sending property {} on processor {} change to {}.", connection->parameter, connection->processor, value);
}

void OscpackOscMessenger::_send_bypass_state_event(const oscpack::ReceivedMessage& m, void* user_data) const
{
    oscpack::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
    int value = (arg++)->AsInt32();
    bool isBypassed = (value) ? true : false;

    auto connection = static_cast<control_frontend::OscConnection*>(user_data);
    auto controller = connection->controller->audio_graph_controller();
    controller->set_processor_bypass_state(connection->processor, isBypassed);

    SUSHI_LOG_DEBUG("Setting processor {} bypass to {}", connection->processor, isBypassed);
}

void OscpackOscMessenger::_send_keyboard_note_event(const oscpack::ReceivedMessage& m, void* user_data) const
{
    oscpack::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
    std::string_view event = (arg++)->AsString();
    int channel = (arg++)->AsInt32();
    int note = (arg++)->AsInt32();
    float value = (arg++)->AsFloat();

    auto connection = static_cast<control_frontend::OscConnection*>(user_data);
    auto controller = connection->controller->keyboard_controller();

    if (event == "note_on")
    {
        controller->send_note_on(connection->processor, channel, note, value);
    }
    else if (event == "note_off")
    {
        controller->send_note_off(connection->processor, channel, note, value);
    }
    else if (event == "note_aftertouch")
    {
        controller->send_note_aftertouch(connection->processor, channel, note, value);
    }
    else
    {
        SUSHI_LOG_WARNING("Unrecognized event: {}.", event);
    }
    SUSHI_LOG_DEBUG("Sending {} on processor {}.", event, connection->processor);
}

void OscpackOscMessenger::_send_keyboard_modulation_event(const oscpack::ReceivedMessage& m, void* user_data) const
{
    oscpack::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
    std::string_view event = (arg++)->AsString();
    int channel = (arg++)->AsInt32();
    float value = (arg++)->AsFloat();

    auto connection = static_cast<control_frontend::OscConnection*>(user_data);
    auto controller = connection->controller->keyboard_controller();

    if (event == "modulation")
    {
        controller->send_modulation(connection->processor, channel, value);
    }
    else if (event == "pitch_bend")
    {
        controller->send_pitch_bend(connection->processor, channel, value);
    }
    else if (event == "aftertouch")
    {
        controller->send_aftertouch(connection->processor, channel, value);
    }
    else
    {
        SUSHI_LOG_WARNING("Unrecognized event: {}.", event);
    }
    SUSHI_LOG_DEBUG("Sending {} on processor {}.", event, connection->processor);
}

void OscpackOscMessenger::_send_program_change_event(const oscpack::ReceivedMessage& m, void* user_data) const
{
    oscpack::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
    int program_id = (arg++)->AsInt32();

    auto connection = static_cast<control_frontend::OscConnection*>(user_data);
    auto controller = connection->controller->program_controller();
    controller->set_processor_program(connection->processor, program_id);

    SUSHI_LOG_DEBUG("Sending change to program {}, on processor {}", program_id, connection->processor);
}

void OscpackOscMessenger::_set_timing_statistics_enabled(const oscpack::ReceivedMessage& m, void* user_data) const
{
    oscpack::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
    int value = (arg++)->AsInt32();
    bool is_enabled = (value) ? true : false;

    auto controller = static_cast<ext::SushiControl*>(user_data)->timing_controller();
    controller->set_timing_statistics_enabled(is_enabled);

    SUSHI_LOG_DEBUG("Got request to set timing statistics enabled to {}", is_enabled);
}

void OscpackOscMessenger::_reset_timing_statistics(const oscpack::ReceivedMessage& m, void* user_data) const
{
    oscpack::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
    std::string output_text = (arg++)->AsString();
    std::string_view type = output_text;

    auto controller = static_cast<ext::SushiControl*>(user_data);
    auto timing_ctrl = controller->timing_controller();
    auto processor_ctrl = controller->audio_graph_controller();

    if (type == "all")
    {
        auto status = timing_ctrl->reset_all_timings();
        if (status != ext::ControlStatus::OK)
        {
            SUSHI_LOG_WARNING("Failed to reset track timings of all tracks and processors");
        }
    }
    else if (type == "track")
    {
        std::string track_name = (arg++)->AsString();

        auto [track_status, track_id] = processor_ctrl->get_track_id(track_name);
        if (track_status == ext::ControlStatus::OK)
        {
            output_text += " " + track_name;
            timing_ctrl->reset_track_timings(track_id);
        }
        else
        {
            SUSHI_LOG_WARNING("No track with name {} available", track_name);
        }
    }
    else if (type == "processor")
    {
        std::string processor_name = (arg++)->AsString();

        auto [processor_status, processor_id] = processor_ctrl->get_processor_id(processor_name);
        if (processor_status == ext::ControlStatus::OK)
        {
            output_text += " " + processor_name;
            timing_ctrl->reset_processor_timings(processor_id);
        }
        else
        {
            SUSHI_LOG_WARNING("No processor with name {} available", processor_name);
        }
    }
    else
    {
        SUSHI_LOG_WARNING("Unrecognized reset target");
    }
    SUSHI_LOG_DEBUG("Resetting {} timing statistics", output_text);
}

void OscpackOscMessenger::_set_tempo(const oscpack::ReceivedMessage& m, void* user_data) const
{
    oscpack::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
    float tempo = (arg++)->AsFloat();

    auto controller = static_cast<ext::SushiControl*>(user_data)->transport_controller();
    controller->set_tempo(tempo);

    SUSHI_LOG_DEBUG("Got a set tempo request to {} bpm", tempo);
}

void OscpackOscMessenger::_set_time_signature(const oscpack::ReceivedMessage& m, void* user_data) const
{
    oscpack::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
    int numerator = (arg++)->AsInt32();
    int denominator = (arg++)->AsInt32();

    auto controller = static_cast<ext::SushiControl*>(user_data)->transport_controller();
    controller->set_time_signature({numerator, denominator});

    SUSHI_LOG_DEBUG("Got a set time signature to {}/{} request", numerator, denominator);
}

void OscpackOscMessenger::_set_playing_mode(const oscpack::ReceivedMessage& m, void* user_data) const
{
    oscpack::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
    std::string mode_str = (arg++)->AsString();

    auto controller = static_cast<ext::SushiControl*>(user_data)->transport_controller();

    if (mode_str == "playing")
    {
        controller->set_playing_mode(ext::PlayingMode::PLAYING);
    }
    else if (mode_str == "stopped")
    {
        controller->set_playing_mode(ext::PlayingMode::STOPPED);
    }
    else
    {
        SUSHI_LOG_INFO("Unrecognised playing mode \"{}\" received", mode_str);
    }

    SUSHI_LOG_DEBUG("Got a set playing mode {} request", mode_str);
}

void OscpackOscMessenger::_set_tempo_sync_mode(const oscpack::ReceivedMessage& m, void* user_data) const
{
    oscpack::ReceivedMessage::const_iterator arg = m.ArgumentsBegin();
    std::string mode_str = (arg++)->AsString();

    auto controller = static_cast<ext::SushiControl*>(user_data)->transport_controller();

    if (mode_str == "internal")
    {
        controller->set_sync_mode(ext::SyncMode::INTERNAL);
    }
    else if (mode_str == "ableton_link")
    {
        controller->set_sync_mode(ext::SyncMode::LINK);
    }
    else if (mode_str == "midi")
    {
        controller->set_sync_mode(ext::SyncMode::MIDI);
    }
    else
    {
        SUSHI_LOG_INFO("Unrecognised sync mode \"{}\" received", mode_str);
    }

    SUSHI_LOG_DEBUG("Got a set sync mode to {} request", mode_str);
}

} // namespace osc
} // namespace sushi
