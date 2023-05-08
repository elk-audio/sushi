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
#include <filesystem>

#include "compile_time_settings.h"

namespace sushi {

namespace control {
class SushiControl;
}

namespace engine {
class AudioEngine;
}

namespace audio_frontend {
struct BaseAudioFrontendConfiguration;
class BaseAudioFrontend;
}

namespace midi_frontend {
class BaseMidiFrontend;
class ReactiveMidiFrontend;
}

enum class FrontendType
{
    OFFLINE,
    DUMMY,
    JACK,
    PORTAUDIO,
    XENOMAI_RASPA,
    REACTIVE,
    NONE
};

enum class ConfigurationSource
{
    FILE,
    JSON_STRING,
    DEFAULT
};

/**
 * The status of why starting Sushi failed.
 * The non-zero values are also returned by the process on exit
 */
enum class Status : int
{
    OK = 0,

    FAILED_INVALID_FILE_PATH = 1,
    FAILED_INVALID_CONFIGURATION_FILE = 2,

    FAILED_LOAD_HOST_CONFIG = 3,
    FAILED_LOAD_TRACKS = 4,
    FAILED_LOAD_MIDI_MAPPING = 5,
    FAILED_LOAD_CV_GATE = 6,
    FAILED_LOAD_PROCESSOR_STATES = 7,
    FAILED_LOAD_EVENT_LIST = 8,
    FAILED_LOAD_EVENTS = 9,
    FAILED_LOAD_OSC = 10,

    FAILED_XENOMAI_INITIALIZATION = 11,
    FAILED_OSC_FRONTEND_INITIALIZATION = 12,
    FAILED_AUDIO_FRONTEND_MISSING = 13,
    FAILED_AUDIO_FRONTEND_INITIALIZATION = 14,
    FAILED_MIDI_FRONTEND_INITIALIZATION = 15,

    FAILED_TO_START_RPC_SERVER = 16,
    FRONTEND_IS_INCOMPATIBLE_WITH_STANDALONE = 17
};

std::string to_string(Status status);

/**
 * Collects all options for instantiating Sushi in one place.
 */
struct SushiOptions
{
    std::string input_filename;
    std::string output_filename;

    std::string log_level = std::string(SUSHI_LOG_LEVEL_DEFAULT);
    std::string log_filename = std::string(SUSHI_LOG_FILENAME_DEFAULT);

    std::string jack_client_name = std::string(SUSHI_JACK_CLIENT_NAME_DEFAULT);
    std::string jack_server_name = std::string("");
    int osc_server_port = SUSHI_OSC_SERVER_PORT_DEFAULT;
    int osc_send_port = SUSHI_OSC_SEND_PORT_DEFAULT;
    std::string osc_send_ip = SUSHI_OSC_SEND_IP_DEFAULT;
    std::optional<int> portaudio_input_device_id = std::nullopt;
    std::optional<int> portaudio_output_device_id = std::nullopt;

    float suggested_input_latency = SUSHI_PORTAUDIO_INPUT_LATENCY_DEFAULT;
    float suggested_output_latency = SUSHI_PORTAUDIO_OUTPUT_LATENCY_DEFAULT;

    bool enable_portaudio_devs_dump = false;

    bool use_osc = true;
    bool use_grpc = true;

    std::string base_plugin_path = std::filesystem::current_path();

    std::string sentry_crash_handler_path = SUSHI_SENTRY_CRASH_HANDLER_PATH_DEFAULT;
    std::string sentry_dsn = SUSHI_SENTRY_DSN_DEFAULT;

    std::string grpc_listening_address = SUSHI_GRPC_LISTENING_PORT_DEFAULT;
    FrontendType frontend_type = FrontendType::NONE;
    bool connect_ports = false;
    bool debug_mode_switches = false;
    int  rt_cpu_cores = 1;
    bool enable_timings = false;
    bool enable_flush_interval = false;
    bool enable_parameter_dump = false;
    std::chrono::seconds log_flush_interval = std::chrono::seconds(0);

    ConfigurationSource config_source = ConfigurationSource::FILE;
    std::string config_filename = std::string(SUSHI_JSON_FILENAME_DEFAULT);
    std::string json_string = std::string(SUSHI_JSON_STRING_DEFAULT);

    /**
     * Extracts the address string and port number from the grpc_listening_address string.
     * @return A pair with address and port on success - nullopt on failure.
     */
    std::optional<std::pair<std::string, int>> grpc_address_and_port();

    /**
     * If sushi is to be started with gRPC, initialising it requires a valid gRPC port number.
     * Using this method, it is possible to incrementally increase the number passed,
     * to retry connecting.
     * @param options the SushiOptions structure containing the gRPC address field.
     * @return true if incrementing the value succeeded.
     */
    bool increment_grpc_port_number();
};

/**
 * This should be called only once in the lifetime of the embedding binary - or it will fail.
 * @param options
 */
void init_logger([[maybe_unused]] const SushiOptions& options);

class Sushi
{
public:
    Sushi() = default;
    virtual ~Sushi() = default;

    [[nodiscard]] virtual Status start() = 0;

    virtual void stop() = 0;

    virtual control::SushiControl* controller() = 0;

    virtual void set_sample_rate(float sample_rate) = 0;
    virtual float sample_rate() const = 0;
};

} // end namespace sushi

#endif // SUSHI_SUSHI_INTERFACE_H
