/*
* Copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk
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
* @brief Main entry point to Sushi
* @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
*/

#ifndef SUSHI_SUSHI_INTERFACE_H
#define SUSHI_SUSHI_INTERFACE_H

#include <cassert>
#include <memory>
#include <chrono>
#include <optional>

#include "compile_time_settings.h"

namespace sushi {

namespace engine {
class AudioEngine;
class Controller;
}

namespace audio_frontend {
class BaseAudioFrontendConfiguration;
class BaseAudioFrontend;
class PassiveFrontend;
}

namespace midi_frontend {
class BaseMidiFrontend;
class PassiveMidiFrontend;
}

enum class FrontendType
{
    OFFLINE,
    DUMMY,
    JACK,
    PORTAUDIO,
    XENOMAI_RASPA,
    PASSIVE,
    NONE
};

enum class InitStatus
{
    OK,

    FAILED_INVALID_FILE_PATH,
    FAILED_INVALID_CONFIGURATION_FILE,

    FAILED_LOAD_HOST_CONFIG,
    FAILED_LOAD_TRACKS,
    FAILED_LOAD_MIDI_MAPPING,
    FAILED_LOAD_CV_GATE,
    FAILED_LOAD_PROCESSOR_STATES,
    FAILED_LOAD_EVENT_LIST,
    FAILED_LOAD_EVENTS,
    FAILED_LOAD_OSC,

    FAILED_XENOMAI_INITIALIZATION,
    FAILED_OSC_FRONTEND_INITIALIZATION,
    FAILED_AUDIO_FRONTEND_MISSING,
    FAILED_AUDIO_FRONTEND_INITIALIZATION,
    FAILED_MIDI_FRONTEND_INITIALIZATION
};

std::string to_string(InitStatus init_status);

/**
 * Collects all options for instantiating Sushi in one place.
 */
struct SushiOptions
{
    std::string input_filename;
    std::string output_filename;

    std::string log_level = std::string(SUSHI_LOG_LEVEL_DEFAULT);
    std::string log_filename = std::string(SUSHI_LOG_FILENAME_DEFAULT);

    bool use_input_config_file = true;
    std::string config_filename = std::string(SUSHI_JSON_FILENAME_DEFAULT);
    std::string jack_client_name = std::string(SUSHI_JACK_CLIENT_NAME_DEFAULT);
    std::string jack_server_name = std::string("");
    int osc_server_port = SUSHI_OSC_SERVER_PORT_DEFAULT;
    int osc_send_port = SUSHI_OSC_SEND_PORT_DEFAULT;
    std::string osc_send_ip = SUSHI_OSC_SEND_IP_DEFAULT;
    std::optional<int> portaudio_input_device_id = std::nullopt;
    std::optional<int> portaudio_output_device_id = std::nullopt;
    std::string grpc_listening_address = SUSHI_GRPC_LISTENING_PORT_DEFAULT;
    FrontendType frontend_type = FrontendType::NONE;
    bool connect_ports = false;
    bool debug_mode_switches = false;
    int  rt_cpu_cores = 1;
    bool enable_timings = false;
    bool enable_flush_interval = false;
    bool enable_parameter_dump = false;
    std::chrono::seconds log_flush_interval = std::chrono::seconds(0);
};

/**
 * This should be called only once in the lifetime of the embedding binary - or it will fail.
 * @param options
 */
void init_logger(const SushiOptions& options);

class AbstractSushi
{
public:
    AbstractSushi() = default;
    virtual ~AbstractSushi() = default;

    virtual InitStatus init(const SushiOptions& options) = 0;

    virtual void start() = 0;

    virtual void exit() = 0;

    virtual engine::Controller* controller() = 0;

    virtual audio_frontend::PassiveFrontend* audio_frontend() = 0;

    virtual void set_sample_rate(float sample_rate) = 0;

    virtual midi_frontend::PassiveMidiFrontend* midi_frontend() = 0;

    virtual engine::AudioEngine* audio_engine() = 0;
};

} // namespace Sushi

#endif // SUSHI_SUSHI_INTERFACE_H
