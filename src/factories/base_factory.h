/*
* Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
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

#ifndef SUSHI_FACTORY_BASE_H
#define SUSHI_FACTORY_BASE_H

#include "sushi/sushi.h"

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
#include "sushi_rpc/grpc_server.h"
#endif

namespace sushi {

namespace engine {
class JsonConfigurator;
class AudioEngine;
class Controller;
}

namespace audio_frontend {
struct BaseAudioFrontendConfiguration;
class BaseAudioFrontend;
}

namespace midi_frontend {
class BaseMidiFrontend;
}

namespace midi_dispatcher {
class MidiDispatcher;
}

namespace control_frontend {
class OSCFrontend;
}

namespace jsonconfig {
class JsonConfigurator;
struct ControlConfig;
}

/**
 * @brief Virtual base class for Sushi Factories.
 *        This implements as much as possible that is in common for factories,
 *        leaving a numbed of virtual methods to be populated by sub-classes.
 *
 *        The design choice has been to not have any switch / if clauses here,
 *        constructing things differently depending on what frontend or other running
 *        configuration has been made.
 *
 *        Ownership of all subsystem is held by the factory until the point where a Sushi class
 *        is constructed and returned, at which point it takes over ownership.
 *
 *        Each factory instance is meant to be run only once and discarded.
 */
class BaseFactory
{
public:
    BaseFactory();
    virtual ~BaseFactory();

    virtual std::pair<std::unique_ptr<Sushi>, Status> new_instance(SushiOptions& options) = 0;

protected:
    std::unique_ptr<Sushi> _make_sushi();

    void _instantiate_subsystems(SushiOptions& options);

    Status _configure_from_file(SushiOptions& options);
    Status _configure_with_defaults(SushiOptions& options);

    Status _configure_engine(SushiOptions& options,
                             const jsonconfig::ControlConfig& control_config,
                             jsonconfig::JsonConfigurator* configurator);

    static Status _load_json_configuration(jsonconfig::JsonConfigurator* configurator);

    /**
     * @brief Inherit and populate this to instantiate and configure the audio frontend.
     * @param options A populated SushiOptions structure.
     * @param config
     * @return
     */
    virtual Status _setup_audio_frontend(const SushiOptions& options,
                                         const jsonconfig::ControlConfig& config) = 0;

    /**
     * @brief Inherit and populate this to instantiate and configure the midi frontend.
     * @param options A populated SushiOptions structure.
     * @return
     */
    virtual Status _set_up_midi(const SushiOptions& options,
                                const jsonconfig::ControlConfig& config) = 0;

    /**
     * @brief Inherit and populate this to instantiate and configure sgRPC, OSC, and eventual other control.
     * @param options A populated SushiOptions structure.
     * @param configurator
     * @return
     */
    virtual Status _set_up_control(const SushiOptions& options,
                                   jsonconfig::JsonConfigurator* configurator) = 0;

    /**
     * @brief Handle sequenced events from configuration file here.
     * @param options A populated SushiOptions structure.
     * @param configurator
     * @return
     */
    virtual Status _load_json_events(const SushiOptions& options,
                                     jsonconfig::JsonConfigurator* configurator) = 0;

    Status _status { Status::OK};

    std::unique_ptr<engine::AudioEngine> _engine {nullptr};
    std::unique_ptr<midi_dispatcher::MidiDispatcher> _midi_dispatcher {nullptr};

    std::unique_ptr<midi_frontend::BaseMidiFrontend> _midi_frontend {nullptr};
    std::unique_ptr<control_frontend::OSCFrontend> _osc_frontend {nullptr};
    std::unique_ptr<audio_frontend::BaseAudioFrontend> _audio_frontend {nullptr};
    std::unique_ptr<audio_frontend::BaseAudioFrontendConfiguration> _frontend_config {nullptr};

    std::unique_ptr<engine::Controller> _engine_controller {nullptr};

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    std::unique_ptr<sushi_rpc::GrpcServer> _rpc_server {nullptr};
#endif
};

} // namespace sushi

#endif //SUSHI_FACTORY_BASE_H
