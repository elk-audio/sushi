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

#include "offline_factory.h"

#include "logging.h"

#include "engine/audio_engine.h"
#include "src/sushi.h"

// TODO AUD-466: What happens if SUSHI_BUILD_WITH_RPC_INTERFACE is OFF!?
#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
#include "sushi_rpc/grpc_server.h"
#endif

#include "audio_frontends/offline_frontend.h"

#include "engine/json_configurator.h"
#include "control_frontends/oscpack_osc_messenger.h"

namespace sushi
{

SUSHI_GET_LOGGER_WITH_MODULE_NAME("sushi-factory");

OfflineFactory::OfflineFactory() = default;

OfflineFactory::~OfflineFactory() = default;

std::unique_ptr<AbstractSushi> OfflineFactory::run(SushiOptions& options)
{
    init_logger(options);

    _instantiate_subsystems(options);

    return _make_sushi();
}

InitStatus OfflineFactory::_setup_audio_frontend(const SushiOptions& options,
                                                 const jsonconfig::ControlConfig& config)
{
    int cv_inputs = config.cv_inputs.value_or(0);
    int cv_outputs = config.cv_outputs.value_or(0);

    bool dummy = false;

    if (options.frontend_type != FrontendType::OFFLINE)
    {
        dummy = true;
        SUSHI_LOG_INFO("Setting up dummy audio frontend");
    }
    else
    {
        SUSHI_LOG_INFO("Setting up offline audio frontend");
    }

    _frontend_config = std::make_unique<audio_frontend::OfflineFrontendConfiguration>(options.input_filename,
                                                                                      options.output_filename,
                                                                                      dummy,
                                                                                      cv_inputs,
                                                                                      cv_outputs);

    _audio_frontend = std::make_unique<audio_frontend::OfflineFrontend>(_engine.get());

    return InitStatus::OK;
}

InitStatus OfflineFactory::_set_up_midi([[maybe_unused]] const SushiOptions& options,
                                        const jsonconfig::ControlConfig& config)
{
    int midi_inputs = config.midi_inputs.value_or(1);
    int midi_outputs = config.midi_outputs.value_or(1);
    _midi_dispatcher->set_midi_inputs(midi_inputs);
    _midi_dispatcher->set_midi_outputs(midi_outputs);

    _midi_frontend = std::make_unique<midi_frontend::NullMidiFrontend>(_midi_dispatcher.get());

    return InitStatus::OK;
}

InitStatus OfflineFactory::_set_up_control(const SushiOptions& options,
                                           [[maybe_unused]] jsonconfig::JsonConfigurator* configurator)
{
    _engine_controller = std::make_unique<engine::Controller>(_engine.get(),
                                                              _midi_dispatcher.get(),
                                                              _audio_frontend.get());

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    _rpc_server = std::make_unique<sushi_rpc::GrpcServer>(options.grpc_listening_address, _engine_controller.get());
    SUSHI_LOG_INFO("Instantiating gRPC server with address: {}", options.grpc_listening_address);
#endif

    return InitStatus::OK;
}

InitStatus OfflineFactory::_load_json_events([[maybe_unused]] const SushiOptions& options,
                                             jsonconfig::JsonConfigurator* configurator)
{
    auto [status, events] = configurator->load_event_list();

    if (status == jsonconfig::JsonConfigReturnStatus::OK)
    {
        static_cast<audio_frontend::OfflineFrontend*>(_audio_frontend.get())->add_sequencer_events(events);
    }
    else if (status != jsonconfig::JsonConfigReturnStatus::NOT_DEFINED)
    {
        return InitStatus::FAILED_LOAD_EVENT_LIST;
    }

    return InitStatus::OK;
}

} // namespace sushi