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

#include "standalone_factory.h"

#include "logging.h"

#include "engine/audio_engine.h"
#include "src/sushi.h"

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
#include "sushi_rpc/grpc_server.h"
#endif

namespace sushi
{

SUSHI_GET_LOGGER_WITH_MODULE_NAME("sushi-factory");

StandaloneFactory::StandaloneFactory() = default;

StandaloneFactory::~StandaloneFactory() = default;

void StandaloneFactory::run(SushiOptions& options)
{
    init_logger(options);

    // TODO: TEST THAT THIS WORKS!

#ifdef SUSHI_BUILD_WITH_XENOMAI
    auto raspa_status = audio_frontend::XenomaiRaspaFrontend::global_init();
    if (raspa_status < 0)
    {
        _status = INIT_STATUS::FAILED_XENOMAI_INITIALIZATION;
        return;
    }

    if (options.frontend_type == FrontendType::XENOMAI_RASPA)
    {
        twine::init_xenomai(); // must be called before setting up any worker pools
    }
#endif

    _engine = std::make_unique<engine::AudioEngine>(SUSHI_SAMPLE_RATE_DEFAULT,
                                                    options.rt_cpu_cores,
                                                    options.debug_mode_switches,
                                                    nullptr);

    _midi_dispatcher = std::make_unique<midi_dispatcher::MidiDispatcher>(_engine->event_dispatcher());

    if (options.use_input_config_file)
    {
        _status = _configure_from_file(options);
    }
    else
    {
        _status = _configure_with_defaults(options);
    }

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

} // namespace sushi