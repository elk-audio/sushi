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

#ifndef SUSHI_SUSHI_H
#define SUSHI_SUSHI_H

#include <cassert>
#include <memory>
#include <chrono>
#include <optional>

#include "sushi/sushi.h"
#include "sushi/compile_time_settings.h"

namespace sushi_rpc {
class GrpcServer;
}

namespace sushi {

namespace internal {

class BaseFactory;

namespace engine {
class JsonConfigurator;
class AudioEngine;
class Controller;
}

namespace audio_frontend {
struct BaseAudioFrontendConfiguration;
class BaseAudioFrontend;
class XenomaiRaspaFrontend;
class PortAudioFrontend;
class OfflineFrontend;
class JackFrontend;
class ReactiveFrontend;
}

namespace midi_frontend {
class BaseMidiFrontend;
class ReactiveMidiFrontend;
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

class ConcreteSushiAccessor;

class ConcreteSushi : public Sushi
{
public:
    ~ConcreteSushi() override;

    /**
     * Given Sushi is initialized successfully, call this before the audio callback is first invoked.
     */
    Status start() override;

    /**
     * Stops the Sushi instance from running.
     */
    void stop() override;

    /**
     * @return an instance of the Sushi controller - assuming Sushi has first been initialized.
     */
    control::SushiControl* controller() override;

    void set_sample_rate(float sample_rate) override;

    [[nodiscard]] float sample_rate() const override;

protected:
    /**
     * @brief To create a Sushi instance, call _make_sushi(...) from inside a class inheriting from FactoryBase.
     */
    ConcreteSushi();

    friend BaseFactory;
    friend ConcreteSushiAccessor;

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

class ConcreteSushiAccessor
{
public:
    explicit ConcreteSushiAccessor(ConcreteSushi& f) : _friend(f) {}

    std::unique_ptr<engine::AudioEngine>& engine()
    {
        return _friend._engine;
    }

    std::unique_ptr<midi_dispatcher::MidiDispatcher>& midi_dispatcher()
    {
        return _friend._midi_dispatcher;
    }

    std::unique_ptr<midi_frontend::BaseMidiFrontend>& midi_frontend()
    {
        return _friend._midi_frontend;
    }

    std::unique_ptr<control_frontend::OSCFrontend>& osc_frontend()
    {
        return _friend._osc_frontend;
    }

    std::unique_ptr<audio_frontend::BaseAudioFrontend>& audio_frontend()
    {
        return _friend._audio_frontend;
    }

    std::unique_ptr<audio_frontend::BaseAudioFrontendConfiguration>& frontend_config()
    {
        return _friend._frontend_config;
    }

    std::unique_ptr<engine::Controller>& engine_controller()
    {
        return _friend._engine_controller;
    }

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    std::unique_ptr<sushi_rpc::GrpcServer>& rpc_server()
    {
        return _friend._rpc_server;
    }
#endif

private:
    ConcreteSushi& _friend;
};

} // end namespace internal

} // end namespace sushi::internal

#endif // SUSHI_SUSHI_H
