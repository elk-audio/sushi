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

#include "compile_time_settings.h"

#include "engine/json_configurator.h"
#include "audio_frontends/offline_frontend.h"
#include "audio_frontends/jack_frontend.h"
#include "audio_frontends/portaudio_frontend.h"
#include "library/parameter_dump.h"
#include "engine/audio_engine.h"
#include "audio_frontends/xenomai_raspa_frontend.h"
#include "audio_frontends/embedded_frontend.h"
#include "control_frontends/osc_frontend.h"

#ifdef SUSHI_BUILD_WITH_ALSA_MIDI
#include "control_frontends/alsa_midi_frontend.h"
#endif

#ifdef SUSHI_BUILD_WITH_RT_MIDI
#include "control_frontends/rt_midi_frontend.h"
#endif

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
#include "sushi_rpc/grpc_server.h"
#endif

namespace sushi {

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

enum class INIT_STATUS
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

    FAILED_TWINE_INITIALIZATION,
    FAILED_OSC_FRONTEND_INITIALIZATION,
    FAILED_AUDIO_FRONTEND_MISSING,
    FAILED_AUDIO_FRONTEND_INITIALIZATION,
    FAILED_MIDI_FRONTEND_INITIALIZATION
};

std::string to_string(INIT_STATUS init_status);

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

void init_logger(const SushiOptions& options);

class Sushi
{
public:
    Sushi() = default;
    ~Sushi() = default;

    /**
     *
     * @param options
     * @return
     */
    INIT_STATUS init(SushiOptions& options);

    /**
     *
     * @param options
     */
    void start(SushiOptions& options);

    /**
     *
     * @param options
     */
    void exit(SushiOptions& options);

    /**
     *
     * @return
     */
    int raspa_status() { return _raspa_status; }

    engine::Controller* controller() { return _controller.get(); }

    /**
     * Exposing audio frontend for the context where Sushi is embedded in another audio host.
     * @return
     */
    audio_frontend::EmbeddedFrontend* audio_frontend()
    {
        return static_cast<audio_frontend::EmbeddedFrontend*>(_audio_frontend.get());
    }

private:
    INIT_STATUS _load_configuration(SushiOptions& options, audio_frontend::BaseAudioFrontend* audio_frontend);

    INIT_STATUS _setup_audio_frontend(SushiOptions& options, int cv_inputs, int cv_outputs);
    INIT_STATUS _set_up_control(SushiOptions& options, int midi_inputs, int midi_outputs);

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
};

} // namespace Sushi

#endif // SUSHI_SUSHI_H
