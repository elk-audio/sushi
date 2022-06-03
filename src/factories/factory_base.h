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

#include "include/sushi/sushi_interface.h"

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
class PassiveFrontend;
}

namespace midi_frontend {
class BaseMidiFrontend;
class PassiveMidiFrontend;
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

class FactoryBase
{
public:
    FactoryBase();
    virtual ~FactoryBase();

    virtual void run(SushiOptions& options) = 0;

    virtual std::unique_ptr<AbstractSushi> sushi();

    InitStatus sushi_init_status();

protected:
    InitStatus _configure_from_file(SushiOptions& options);
    InitStatus _configure_with_defaults(SushiOptions& options);

    InitStatus _configure_engine(SushiOptions& options,
                                 const jsonconfig::ControlConfig& control_config,
                                 jsonconfig::JsonConfigurator* configurator);

    static InitStatus _load_json_configuration(jsonconfig::JsonConfigurator* configurator);

    static InitStatus _load_json_events(const SushiOptions& options,
                                        jsonconfig::JsonConfigurator* configurator,
                                        audio_frontend::BaseAudioFrontend* audio_frontend);

    InitStatus _setup_audio_frontend(const SushiOptions& options, const jsonconfig::ControlConfig& config);
    InitStatus _set_up_midi(const SushiOptions& options, const jsonconfig::ControlConfig& config);
    InitStatus _set_up_control(const SushiOptions& options, jsonconfig::JsonConfigurator* configurator);

    InitStatus _status {InitStatus::OK};

    std::unique_ptr<engine::AudioEngine> _engine {nullptr};
    std::unique_ptr<midi_dispatcher::MidiDispatcher> _midi_dispatcher {nullptr};

    std::unique_ptr<midi_frontend::BaseMidiFrontend> _midi_frontend {nullptr};
    std::unique_ptr<control_frontend::OSCFrontend> _osc_frontend {nullptr};
    std::unique_ptr<audio_frontend::BaseAudioFrontend> _audio_frontend {nullptr};
    std::unique_ptr<audio_frontend::BaseAudioFrontendConfiguration> _frontend_config {nullptr};

    std::unique_ptr<engine::Controller> _engine_controller {nullptr};

    std::unique_ptr<sushi_rpc::GrpcServer> _rpc_server {nullptr};

    std::unique_ptr<AbstractSushi> _sushi {nullptr};
};

} // namespace sushi

#endif //SUSHI_FACTORY_BASE_H
