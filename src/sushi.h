/*
* Copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk
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
//
//namespace engine {
//    class AudioEngine;
//    class Controller;
//}
//
//namespace midi_frontend {
//    class BaseMidiFrontend;
//}
//
//namespace control_frontend {
//    class OSCFrontend;
//}
//
//namespace audio_frontend {
//    class BaseAudioFrontend;
//}
//
//namespace audio_frontend {
//    class BaseAudioFrontendConfiguration;
//}
//
//namespace midi_dispatcher {
//    class MidiDispatcher;
//}
//
//namespace jsonconfig {
//    class JsonConfigurator;
//}
//
//namespace sushi_rpc {
//    class GrpcServer;
//}

enum class FrontendType
{
    OFFLINE,
    DUMMY,
    JACK,
    PORTAUDIO,
    XENOMAI_RASPA,
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
    FAILED_MIDI_FRONTEND_INITIALIZATION,

    DUMPING_PARAMETERS
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

class Sushi
{
public:
    Sushi() {}

    ~Sushi() {}

    INIT_STATUS load_configuration(SushiOptions& options);

    INIT_STATUS init(SushiOptions& options);

    void init_logger(SushiOptions& options);

    void start(SushiOptions& options);

    void exit(SushiOptions& options);

    // TODO: Make private, expose getters:

    std::unique_ptr<engine::AudioEngine> engine {nullptr};

    std::unique_ptr<midi_frontend::BaseMidiFrontend> midi_frontend {nullptr};
    std::unique_ptr<control_frontend::OSCFrontend> osc_frontend {nullptr};
    std::unique_ptr<audio_frontend::BaseAudioFrontend> audio_frontend {nullptr};
    std::unique_ptr<audio_frontend::BaseAudioFrontendConfiguration> frontend_config {nullptr};

    std::unique_ptr<midi_dispatcher::MidiDispatcher> midi_dispatcher {nullptr};

    std::unique_ptr<jsonconfig::JsonConfigurator> configurator {nullptr};

    std::unique_ptr<engine::Controller> controller {nullptr};

    std::unique_ptr<sushi_rpc::GrpcServer> rpc_server {nullptr};

    int raspa_status {0};

private:


};

} // namespace Sushi

#endif // SUSHI_SUSHI_H
