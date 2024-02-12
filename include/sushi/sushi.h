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
 * @brief Main entry point to Sushi
 * @copyright 2017-2023 Elk Audio AB, Stockholm
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

namespace internal::engine {
class AudioEngine;
}

namespace internal::audio_frontend {
struct BaseAudioFrontendConfiguration;
class BaseAudioFrontend;
}

namespace internal::midi_frontend {
class BaseMidiFrontend;
class ReactiveMidiFrontend;
}

enum class FrontendType
{
    OFFLINE,
    DUMMY,
    JACK,
    PORTAUDIO,
    APPLE_COREAUDIO,
    XENOMAI_RASPA,
    REACTIVE,
    NONE
};

enum class ConfigurationSource : int
{
    NONE = 0,
    FILE = 1,
    JSON_STRING = 2
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
    FRONTEND_IS_INCOMPATIBLE_WITH_STANDALONE = 17,
    
    SUSHI_ALREADY_STARTED = 18,
    SUSHI_THREW_EXCEPTION = 19,

    UNINITIALIZED = 20
};

std::string to_string(Status status);

/**
 * Collects all options for instantiating Sushi in one place.
 */
struct SushiOptions
{
    /**
     * Set this to choose what audio frontend type Sushi should use.
     * The options are defined in the FrontendType enum class.
     */
    FrontendType frontend_type = FrontendType::NONE;

    /**
     * Specify a directory to be the base of plugin paths used in JSON configuration files,
     * and over gRPC commands for plugin loading.
     */
    std::string base_plugin_path = std::filesystem::current_path();

    /**
     * Set this to choose how Sushi will be configured:
     * By a json-config file path (ConfigurationSource::FILE),
     * a string containing a json configuration (ConfigurationSource::JSON_STRING),
     * or no configuration (ConfigurationSource::NONE)
     * - in which case the minimum default config is set.
     */
    ConfigurationSource config_source = ConfigurationSource::FILE;

    /**
     * Only used if config_source is set to ConfigurationSource::FILE.
     */
    std::string config_filename = std::string(SUSHI_JSON_FILENAME_DEFAULT);

    /**
     * Only used if config_source is set to ConfigurationSource::JSON_STRING.
     */
    std::string json_string = std::string(SUSHI_JSON_STRING_DEFAULT);

    /**
     * Specify minimum logging level, from ('debug', 'info', 'warning', 'error')
     */
    std::string log_level = std::string(ELKLOG_LOG_LEVEL_DEFAULT);

    /**
     * Specify logging file destination.
     */
    std::string log_file = std::string(ELKLOG_LOG_FILE_DEFAULT);

    /**
     * These are only for the case of using a JACK audio frontend,
     * and even then, you will rarely need to alter the defaults.
     */
    std::string jack_client_name = std::string(SUSHI_JACK_CLIENT_NAME_DEFAULT);
    std::string jack_server_name = std::string("");

    /**
     * Try to automatically connect Jack ports at startup.
     */
    bool connect_ports = false;

    /**
     * Index of the device to use for audio input with portaudio frontend [default=system default].
     */
    std::optional<int> portaudio_input_device_id = std::nullopt;

    /**
     * Index of the device to use for audio output with portaudio frontend [default=system default]
     */
    std::optional<int> portaudio_output_device_id = std::nullopt;

    /**
     * UID of the device to use for audio input with Apple CoreAudio frontend.
     */
    std::optional<std::string> apple_coreaudio_input_device_uid = std::nullopt;

    /**
     * UID of the device to use for audio output with Apple CoreAudio frontend.
     */
    std::optional<std::string> apple_coreaudio_output_device_uid = std::nullopt;

    /**
     * Input latency in seconds to suggest to portaudio.
     * Will be rounded up to closest available latency depending on audio API.
     */
    float suggested_input_latency = SUSHI_PORTAUDIO_INPUT_LATENCY_DEFAULT;

    /**
     * Output latency in seconds to suggest to portaudio.
     * Will be rounded up to closest available latency depending on audio API.
     */
    float suggested_output_latency = SUSHI_PORTAUDIO_OUTPUT_LATENCY_DEFAULT;

    /**
     * If true, Sushi will dump available audio devices to stdout in JSON format, and immediately exit.
     * This requires a frontend to be specified.
     */
    bool enable_audio_devices_dump = false;

    /**
     * Dump plugin and parameter data to stdout in JSON format.
     * This will reflect the configuration currently loaded.
     */
    bool enable_parameter_dump = false;

    /**
     * Set this to false, to disable Open Sound Control completely.
     */
    bool use_osc = true;

    /**
     * If the OSC control frontend is enabled,
     * you will also need to configure the IP and Ports it uses.
     *
     *
     * The osc_server_port is the Port to listen for OSC messages on.
     * If it is unavailable, Sushi will fail to start, returning:
     * Status::FAILED_OSC_FRONTEND_INITIALIZATION.
     */
    int osc_server_port = SUSHI_OSC_SERVER_PORT_DEFAULT;

    /**
     * Port and IP to send OSC messages to.
     */
    int osc_send_port = SUSHI_OSC_SEND_PORT_DEFAULT;
    std::string osc_send_ip = SUSHI_OSC_SEND_IP_DEFAULT;

    /**
     * Set this to false, to disable gRPC completely.
     */
    bool use_grpc = true;

    /**
     * gRPC listening address in the format: address:port. By default accepts incoming connections from all ip:s.
     */
    std::string grpc_listening_address = SUSHI_GRPC_LISTENING_PORT_DEFAULT;

    /**
     * Set the path to the crash handler to use for sentry reports.
     */
    std::string sentry_crash_handler_path = SUSHI_SENTRY_CRASH_HANDLER_PATH_DEFAULT;

    /**
     * Set the DSN that sentry should upload crash logs to.
     */
    std::string sentry_dsn = SUSHI_SENTRY_DSN_DEFAULT;

    /**
     * These are used only if Sushi uses an Offline audio frontend.
     * Then, sushi uses the first path as its audio input,
     * and the second as output.
     */
    std::string input_filename;
    std::string output_filename;

    /**
     * Break to debugger if a mode switch is detected (Xenomai only).
     */
    bool debug_mode_switches = false;

    /**
     * Process audio multi-threaded, with n cores [default n=1 (off)].
     */
    int  rt_cpu_cores = 1;

    /**
     * Enable performance timings on all audio processors.
     */
    bool enable_timings = false;

    /**
     * Enable flushing the log periodically and specify the interval.
     */
    bool enable_flush_interval = false;
    std::chrono::seconds log_flush_interval = std::chrono::seconds(0);

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

    // This field is used internally by Sushi.
    std::optional<std::string> device_name = std::nullopt;
};

/**
 * base Sushi class API.
 * To create a Sushi instance, use one of the factories provided, depending on the use-case required:
 * - ReactiveFactory
 * - StandaloneFactory
 * - OfflineFactory
 */
class Sushi
{
protected:
    Sushi() = default;

public:
    virtual ~Sushi() = default;

    /**
     * Given Sushi is initialized successfully, call this before the audio callback is first invoked.
     * This is only meant to be called once during the instance lifetime.
     * @return Status will reflect if starting was successful, or what error occurred.
     */
    [[nodiscard]] virtual Status start() = 0;

    /**
     * Call to stop the Sushi instance.
     * This is only meant to be called once during the instance lifetime.
     */
    virtual void stop() = 0;

    /**
     * @return an instance of the Sushi controller - assuming Sushi has first been initialized.
     */
    virtual control::SushiControl* controller() = 0;

    /**
     * Setting the sample rate.
     * @param sample_rate
     */
    virtual void set_sample_rate(float sample_rate) = 0;

    /**
     * Querying the currently set sample-rate.
     * @return
     */
    virtual float sample_rate() const = 0;
};

} // end namespace sushi

#endif // SUSHI_SUSHI_INTERFACE_H
