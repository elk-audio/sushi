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

#include "osc_frontend.h"

#include "logging.h"

#include "osc_utils.h"

#include "lo/lo.h"
#include "lo/lo_types.h"

#include "liblo_osc_messenger.h"

namespace sushi {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("osc frontend");

namespace {

static void osc_error([[maybe_unused]]int num, const char* msg, const char* path)
{
    if (msg && path) // Sometimes liblo passes a null pointer for msg
    {
        SUSHI_LOG_ERROR("liblo server error {} in path {}: {}", num, path, msg);
    }
}

static int osc_send_parameter_change_event(const char* /*path*/,
                                           const char* /*types*/,
                                           lo_arg** argv,
                                           int /*argc*/,
                                           lo_message /*data*/,
                                           void* user_data)
{
    float value = argv[0]->f;

    auto connection = static_cast<control_frontend::OscConnection*>(user_data);
    auto controller = connection->controller->parameter_controller();
    controller->set_parameter_value(connection->processor, connection->parameter, value);

    SUSHI_LOG_DEBUG("Sending parameter {} on processor {} change to {}.", connection->parameter, connection->processor, value);
    return 0;
}

static int osc_send_property_change_event(const char* /*path*/,
                                          const char* /*types*/,
                                          lo_arg** argv,
                                          int /*argc*/,
                                          lo_message /*data*/,
                                          void* user_data)
{
    std::string value(&argv[0]->s);

    auto connection = static_cast<control_frontend::OscConnection*>(user_data);
    auto controller = connection->controller->parameter_controller();
    controller->set_property_value(connection->processor, connection->parameter, value);

    SUSHI_LOG_DEBUG("Sending property {} on processor {} change to {}.", connection->parameter, connection->processor, value);
    return 0;
}

static int osc_send_bypass_state_event(const char* /*path*/,
                                       const char* /*types*/,
                                       lo_arg** argv,
                                       int /*argc*/,
                                       lo_message /*data*/,
                                       void* user_data)
{
    bool isBypassed = (argv[0]->i == 0) ? false : true;

    auto connection = static_cast<control_frontend::OscConnection*>(user_data);
    auto controller = connection->controller->audio_graph_controller();
    controller->set_processor_bypass_state(connection->processor, isBypassed);

    SUSHI_LOG_DEBUG("Setting processor {} bypass to {}", connection->processor, isBypassed);
    return 0;
}

static int osc_send_keyboard_note_event(const char* /*path*/,
                                        const char* /*types*/,
                                        lo_arg** argv,
                                        int /*argc*/,
                                        lo_message /*data*/,
                                        void* user_data)
{
    std::string_view event(&argv[0]->s);
    int channel = argv[1]->i;
    int note = argv[2]->i;
    float value = argv[3]->f;

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
        return 0;
    }
    SUSHI_LOG_DEBUG("Sending {} on processor {}.", event, connection->processor);
    return 0;
}

static int osc_send_keyboard_modulation_event(const char* /*path*/,
                                              const char* /*types*/,
                                              lo_arg** argv,
                                              int /*argc*/,
                                              lo_message /*data*/,
                                              void* user_data)
{
    std::string_view event(&argv[0]->s);
    int channel = argv[1]->i;
    float value = argv[2]->f;

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
        return 0;
    }
    SUSHI_LOG_DEBUG("Sending {} on processor {}.", event, connection->processor);
    return 0;
}

static int osc_send_program_change_event(const char* /*path*/,
                                         const char* /*types*/,
                                         lo_arg** argv,
                                         int /*argc*/,
                                         lo_message /*data*/,
                                         void* user_data)
{
    int program_id = argv[0]->i;

    auto connection = static_cast<control_frontend::OscConnection*>(user_data);
    auto controller = connection->controller->program_controller();
    controller->set_processor_program(connection->processor, program_id);

    SUSHI_LOG_DEBUG("Sending change to program {}, on processor {}", program_id, connection->processor);
    return 0;
}

static int osc_set_timing_statistics_enabled(const char* /*path*/,
                                             const char* /*types*/,
                                             lo_arg** argv,
                                             int /*argc*/,
                                             lo_message /*data*/,
                                             void* user_data)
{
    bool is_enabled = (argv[0]->i == 0) ? false : true;

    auto controller = static_cast<ext::SushiControl*>(user_data)->timing_controller();
    controller->set_timing_statistics_enabled(is_enabled);

    SUSHI_LOG_DEBUG("Got request to set timing statistics enabled to {}", is_enabled);
    return 0;
}

static int osc_reset_timing_statistics(const char* /*path*/,
                                       const char* /*types*/,
                                       lo_arg** argv,
                                       int /*argc*/,
                                       lo_message /*data*/,
                                       void* user_data)
{
    std::string output_text = std::string(&argv[0]->s);
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
            return 0;
        }
    }
    else if (type == "track")
    {
        std::string track_name = std::string(&argv[1]->s);

        auto [track_status, track_id] = processor_ctrl->get_track_id(track_name);
        if (track_status == ext::ControlStatus::OK)
        {
            output_text += " " + track_name;
            timing_ctrl->reset_track_timings(track_id);
        }
        else
        {
            SUSHI_LOG_WARNING("No track with name {} available", track_name);
            return 0;
        }
    }
    else if (type == "processor")
    {
        std::string processor_name = std::string(&argv[1]->s);
        auto [processor_status, processor_id] = processor_ctrl->get_processor_id(processor_name);
        if (processor_status == ext::ControlStatus::OK)
        {
            output_text += " " + processor_name;
            timing_ctrl->reset_processor_timings(processor_id);
        }
        else
        {
            SUSHI_LOG_WARNING("No processor with name {} available", processor_name);
            return 0;
        }
    }
    else
    {
        SUSHI_LOG_WARNING("Unrecognized reset target");
        return 0;
    }
    SUSHI_LOG_DEBUG("Resetting {} timing statistics", output_text);
    return 0;
}

static int osc_set_tempo(const char* /*path*/,
                         const char* /*types*/,
                         lo_arg** argv,
                         int /*argc*/,
                         lo_message /*data*/,
                         void* user_data)
{
    float tempo = argv[0]->f;

    auto controller = static_cast<ext::SushiControl*>(user_data)->transport_controller();
    controller->set_tempo(tempo);

    SUSHI_LOG_DEBUG("Got a set tempo request to {} bpm", tempo);
    return 0;
}

static int osc_set_time_signature(const char* /*path*/,
                                  const char* /*types*/,
                                  lo_arg** argv,
                                  int /*argc*/,
                                  lo_message /*data*/,
                                  void* user_data)
{
    int numerator = argv[0]->i;
    int denominator = argv[1]->i;

    auto controller = static_cast<ext::SushiControl*>(user_data)->transport_controller();
    controller->set_time_signature({numerator, denominator});

    SUSHI_LOG_DEBUG("Got a set time signature to {}/{} request", numerator, denominator);
    return 0;
}

static int osc_set_playing_mode(const char* /*path*/,
                                const char* /*types*/,
                                lo_arg** argv,
                                int /*argc*/,
                                lo_message /*data*/,
                                void* user_data)
{
    std::string_view mode_str(&argv[0]->s);
    ext::PlayingMode mode;
    if (mode_str == "playing")
    {
        mode = ext::PlayingMode::PLAYING;
    }
    else if (mode_str == "stopped")
    {
        mode = ext::PlayingMode::STOPPED;
    }
    else
    {
        SUSHI_LOG_INFO("Unrecognised playing mode \"{}\" received", mode_str);
        return 0;
    }

    auto controller = static_cast<ext::SushiControl*>(user_data)->transport_controller();
    controller->set_playing_mode(mode);

    SUSHI_LOG_DEBUG("Got a set playing mode {} request", mode_str);
    return 0;
}

static int osc_set_tempo_sync_mode(const char* /*path*/,
                                   const char* /*types*/,
                                   lo_arg** argv,
                                   int /*argc*/,
                                   lo_message /*data*/,
                                   void* user_data)
{
    std::string_view mode_str(&argv[0]->s);
    ext::SyncMode mode;
    if (mode_str == "internal")
    {
        mode = ext::SyncMode::INTERNAL;
    }
    else if (mode_str == "ableton_link")
    {
        mode = ext::SyncMode::LINK;
    }
    else if (mode_str == "midi")
    {
        mode = ext::SyncMode::MIDI;
    }
    else
    {
        SUSHI_LOG_INFO("Unrecognised sync mode \"{}\" received", mode_str);
        return 0;
    }

    auto controller = static_cast<ext::SushiControl*>(user_data)->transport_controller();
    controller->set_sync_mode(mode);

    SUSHI_LOG_DEBUG("Got a set sync mode to {} request", mode_str);
    return 0;
}

}; // anonymous namespace

namespace osc {

LibloOscMessenger::LibloOscMessenger(int receive_port, int send_port) : BaseOscMessenger(receive_port, send_port) {}

LibloOscMessenger::~LibloOscMessenger()
{
    if (_osc_initialized)
    {
        lo_server_thread_free(_osc_server);
        lo_address_free(_osc_out_address);
        _osc_initialized = false;
    }
}

bool LibloOscMessenger::init()
{
    std::stringstream port_stream;
    port_stream << _receive_port;
    _osc_server = lo_server_thread_new(port_stream.str().c_str(), osc_error);
    if (_osc_server == nullptr)
    {
        SUSHI_LOG_ERROR("Failed to set up OSC server, Port likely in use ({})", _receive_port);
        return false;
    }

    std::stringstream send_port_stream;
    send_port_stream << _send_port;
    _osc_out_address = lo_address_new(nullptr, send_port_stream.str().c_str());

    return true;
}

int LibloOscMessenger::run()
{
    return lo_server_thread_start(_osc_server);
}

int LibloOscMessenger::stop()
{
    return lo_server_thread_stop(_osc_server);
}

void* LibloOscMessenger::add_method(const char* address_pattern,
                           const char* type_tag_string,
                           OscMethodType type,
                           const void* connection)
{
    switch (type)
    {
    case OscMethodType::SEND_PARAMETER_CHANGE_EVENT:
    {
        return lo_server_thread_add_method(_osc_server,
                                           address_pattern,
                                           type_tag_string,
                                           osc_send_parameter_change_event,
                                           connection);
    }
    case OscMethodType::SEND_PROPERTY_CHANGE_EVENT:
    {
        return lo_server_thread_add_method(_osc_server,
                                           address_pattern,
                                           type_tag_string,
                                           osc_send_property_change_event,
                                           connection);
    }
    case OscMethodType::SEND_BYPASS_STATE_EVENT:
    {
        return lo_server_thread_add_method(_osc_server,
                                           address_pattern,
                                           type_tag_string,
                                           osc_send_bypass_state_event,
                                           connection);
    }
    case OscMethodType::SEND_KEYBOARD_NOTE_EVENT:
    {
        return lo_server_thread_add_method(_osc_server,
                                           address_pattern,
                                           type_tag_string,
                                           osc_send_keyboard_note_event,
                                           connection);
    }
    case OscMethodType::SEND_KEYBOARD_MODULATION_EVENT:
    {
        return lo_server_thread_add_method(_osc_server,
                                           address_pattern,
                                           type_tag_string,
                                           osc_send_keyboard_modulation_event,
                                           connection);
    }
    case OscMethodType::SEND_PROGRAM_CHANGE_EVENT:
    {
        return lo_server_thread_add_method(_osc_server,
                                           address_pattern,
                                           type_tag_string,
                                           osc_send_program_change_event,
                                           connection);
    }
    case OscMethodType::SET_TEMPO:
    {
        return lo_server_thread_add_method(_osc_server,
                                           address_pattern,
                                           type_tag_string,
                                           osc_set_tempo,
                                           connection);
    }
    case OscMethodType::SET_TIME_SIGNATURE:
    {
        return lo_server_thread_add_method(_osc_server,
                                           address_pattern,
                                           type_tag_string,
                                           osc_set_time_signature,
                                           connection);
    }
    case OscMethodType::SET_PLAYING_MODE:
    {
        return lo_server_thread_add_method(_osc_server,
                                           address_pattern,
                                           type_tag_string,
                                           osc_set_playing_mode,
                                           connection);
    }
    case OscMethodType::SET_TEMPO_SYNC_MODE:
    {
        return lo_server_thread_add_method(_osc_server,
                                           address_pattern,
                                           type_tag_string,
                                           osc_set_tempo_sync_mode,
                                           connection);
    }
    case OscMethodType::SET_TIMING_STATISTICS_ENABLED:
    {
        return lo_server_thread_add_method(_osc_server,
                                           address_pattern,
                                           type_tag_string,
                                           osc_set_timing_statistics_enabled,
                                           connection);
    }
    case OscMethodType::RESET_TIMING_STATISTICS:
    {
        return lo_server_thread_add_method(_osc_server,
                                           address_pattern,
                                           type_tag_string,
                                           osc_reset_timing_statistics,
                                           connection);
    }
    default:
    {
        SUSHI_LOG_WARNING("No Liblo OSC method registered - the specified OscMethodType is not supported.");
        assert(false);
        return nullptr;
    }
    }
}

void LibloOscMessenger::delete_method(void* method)
{
    lo_server_thread_del_lo_method(_osc_server, static_cast<lo_method>(method));
}

void LibloOscMessenger::send(const char* address_pattern, float payload)
{
    lo_send(_osc_out_address, address_pattern, "f", payload);
}

void LibloOscMessenger::send(const char* address_pattern, int payload)
{
    lo_send(_osc_out_address, address_pattern, "i", payload);
}

} // namespace osc
} // namespace sushi

