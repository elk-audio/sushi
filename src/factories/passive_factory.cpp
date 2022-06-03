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

#include "include/sushi/passive_factory.h"

#include "engine/audio_engine.h"
#include "control_frontends/passive_midi_frontend.h"
#include "audio_frontends/passive_frontend.h"

#include "src/sushi.h"

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
#include "sushi_rpc/grpc_server.h"
#endif

namespace sushi
{

PassiveFactory::PassiveFactory() = default;
PassiveFactory::~PassiveFactory() = default;

void PassiveFactory::run(SushiOptions& options)
{
    init_logger(options); // This can only be called once.

    // Overriding whatever frontend settings may or may not have been set.
    options.frontend_type = FrontendType::PASSIVE;

    _engine = std::make_unique<engine::AudioEngine>(SUSHI_SAMPLE_RATE_DEFAULT,
                                                    options.rt_cpu_cores,
                                                    options.debug_mode_switches,
                                                    nullptr);

    _midi_dispatcher = std::make_unique<midi_dispatcher::MidiDispatcher>(_engine->event_dispatcher());
    auto midi_frontend = std::make_unique<midi_frontend::PassiveMidiFrontend>(_midi_dispatcher.get());
    _midi_frontend = std::move(midi_frontend);

    if (options.use_input_config_file)
    {
        _status = _configure_from_file(options);
    }
    else
    {
        _status = _configure_with_defaults(options);
    }

    _real_time_controller = std::make_unique<RealTimeController>(
        static_cast<audio_frontend::PassiveFrontend*>(_audio_frontend.get()),
        static_cast<midi_frontend::PassiveMidiFrontend*>(_midi_frontend.get()),
        _engine->transport());

    _sushi = std::make_unique<Sushi>(std::move(_engine),
                                     std::move(_midi_dispatcher),
                                     std::move(_midi_frontend),
                                     std::move(_osc_frontend),
                                     std::move(_audio_frontend),
                                     std::move(_frontend_config),
                                     std::move(_engine_controller),
                                     std::move(_rpc_server));

    _sushi->init(options);
}

std::unique_ptr<RealTimeController> PassiveFactory::rt_controller()
{
    return std::move(_real_time_controller);
}

}