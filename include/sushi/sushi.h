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

#ifndef SUSHI_SUSHI_H
#define SUSHI_SUSHI_H

#include <cassert>
#include <memory>
#include <chrono>
#include <optional>

#include "compile_time_settings.h"

namespace sushi_rpc {
class GrpcServer;
}

namespace sushi {

namespace engine {
class JsonConfigurator;
class AudioEngine;
class Controller;
}

namespace audio_frontend {
class BaseAudioFrontendConfiguration;
class BaseAudioFrontend;
class XenomaiRaspaFrontend;
class PortAudioFrontend;
class OfflineFrontend;
class JackFrontend;
class EmbeddedFrontend;
}

namespace midi_frontend {
class BaseMidiFrontend;
}

namespace midi_dispatcher {
class MidiDispatcher;
}

namespace control_frontend {
class OSCFrontend;
class AlsaMidiFrontend;
class RtMidiFrontend;
}

namespace jsonconfig {
class JsonConfigurator;
}

enum class FrontendType
{
    OFFLINE,
    DUMMY,
    JACK,
    PORTAUDIO,
    XENOMAI_RASPA,
    EMBEDDED,
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

    std::string log_level = std::string(CompileTimeSettings::log_level_default);
    std::string log_filename = std::string(CompileTimeSettings::log_filename_default);
    std::string config_filename = std::string(CompileTimeSettings::json_filename_default);
    std::string jack_client_name = std::string(CompileTimeSettings::jack_client_name_default);
    std::string jack_server_name = std::string("");
    int osc_server_port = CompileTimeSettings::osc_server_port;
    int osc_send_port = CompileTimeSettings::osc_send_port;
    std::optional<int> portaudio_input_device_id = std::nullopt;
    std::optional<int> portaudio_output_device_id = std::nullopt;
    std::string grpc_listening_address = CompileTimeSettings::grpc_listening_port;
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

class Sushi
{
public:
    Sushi();
    ~Sushi();

    /**
     * Initializes the class.
     * @param options options for Sushi instance
     * @return The success or failure of the init process.
     */
    InitStatus init(const SushiOptions& options);

    /**
     * Given Sushi is initialized successfully, call this before the audio callback is first invoked.
     */
    void start();

    /**
     * Stops the Sushi instance from running.
     */
//  TODO: Currently, once called, the instance will crash if is you subsequently again invoke start(...).
    void exit();

    /**
     * Only needed if the raspa frontend is used, to check on its initialization status.
     * @return
     */
    int raspa_status() { return _raspa_status; }

    /**
     * @return an instance of the Sushi controller - assuming Sushi has first been initialized.
     */
    engine::Controller* controller();

    /**
     * Exposing audio frontend for the context where Sushi is embedded in another audio host.
     * @return
     */
    audio_frontend::EmbeddedFrontend* audio_frontend();

private:
    InitStatus _load_configuration(const SushiOptions& options, audio_frontend::BaseAudioFrontend* audio_frontend);

    InitStatus _setup_audio_frontend(const SushiOptions& options, int cv_inputs, int cv_outputs);
    InitStatus _set_up_control(const SushiOptions& options, int midi_inputs, int midi_outputs);

    std::unique_ptr<engine::AudioEngine> _engine {nullptr};

    std::unique_ptr<midi_frontend::BaseMidiFrontend> _midi_frontend {nullptr};
    std::unique_ptr<control_frontend::OSCFrontend> _osc_frontend {nullptr};
    std::unique_ptr<audio_frontend::BaseAudioFrontend> _audio_frontend {nullptr};
    std::unique_ptr<audio_frontend::BaseAudioFrontendConfiguration> _frontend_config {nullptr};

    std::unique_ptr<midi_dispatcher::MidiDispatcher> _midi_dispatcher {nullptr};

    std::unique_ptr<jsonconfig::JsonConfigurator> _configurator {nullptr};

    std::unique_ptr<engine::Controller> _controller {nullptr};

    std::unique_ptr<sushi_rpc::GrpcServer> _rpc_server {nullptr};

    int _raspa_status {0};

    SushiOptions _options;
};

} // namespace Sushi

#endif // SUSHI_SUSHI_H
