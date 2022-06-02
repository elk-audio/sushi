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

#include "include/sushi/sushi_interface.h"

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
}

/**
 * This should be called only once in the lifetime of the embedding binary - or it will fail.
 * @param options
 */
void init_logger(const SushiOptions& options);


class Sushi : public AbstractSushi
{
public:
    Sushi(std::unique_ptr<engine::AudioEngine> engine,
          std::unique_ptr<midi_dispatcher::MidiDispatcher> midi_dispatcher,
          std::unique_ptr<midi_frontend::BaseMidiFrontend> midi_frontend,
          std::unique_ptr<control_frontend::OSCFrontend> osc_frontend,
          std::unique_ptr<audio_frontend::BaseAudioFrontend> audio_frontend,
          std::unique_ptr<audio_frontend::BaseAudioFrontendConfiguration> frontend_config,
          std::unique_ptr<engine::Controller> engine_controller,
          std::unique_ptr<sushi_rpc::GrpcServer> rpc_server);

    ~Sushi() override;

    /**
     * Initializes the class.
     * @param options options for Sushi instance
     */
    void init(const SushiOptions& options) override;

    /**
     * Given Sushi is initialized successfully, call this before the audio callback is first invoked.
     */
    void start() override;

    /**
     * Stops the Sushi instance from running.
     */
//  TODO: Currently, once called, the instance will crash if is you subsequently again invoke start(...).
    void exit() override;

    /**
     * @return an instance of the Sushi controller - assuming Sushi has first been initialized.
     */
    engine::Controller* controller() override;

    void set_sample_rate(float sample_rate) override;

    float sample_rate() const override;

private:
    std::unique_ptr<engine::AudioEngine> _engine {nullptr};
    std::unique_ptr<midi_dispatcher::MidiDispatcher> _midi_dispatcher {nullptr};

    std::unique_ptr<midi_frontend::BaseMidiFrontend> _midi_frontend {nullptr};
    std::unique_ptr<control_frontend::OSCFrontend> _osc_frontend {nullptr};
    std::unique_ptr<audio_frontend::BaseAudioFrontend> _audio_frontend {nullptr};
    std::unique_ptr<audio_frontend::BaseAudioFrontendConfiguration> _frontend_config {nullptr};

    std::unique_ptr<engine::Controller> _engine_controller {nullptr};

    std::unique_ptr<sushi_rpc::GrpcServer> _rpc_server {nullptr};

    SushiOptions _options;
};

} // namespace Sushi

#endif // SUSHI_SUSHI_H
