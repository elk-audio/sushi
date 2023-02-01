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

#include "logging.h"

#include "engine/audio_engine.h"
#include "control_frontends/passive_midi_frontend.h"
#include "audio_frontends/passive_frontend.h"

#include "src/concrete_sushi.h"

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
#include "sushi_rpc/grpc_server.h"
#endif

#include "engine/json_configurator.h"

#include "engine/controller/real_time_controller.h"
#include "control_frontends/oscpack_osc_messenger.h"

namespace sushi
{

SUSHI_GET_LOGGER_WITH_MODULE_NAME("passive-factory");

PassiveFactory::PassiveFactory() = default;
PassiveFactory::~PassiveFactory() = default;

std::pair<std::unique_ptr<Sushi>, InitStatus> PassiveFactory::new_instance(SushiOptions& options)
{
    init_logger(options); // This can only be called once.

    // Overriding whatever frontend choice may or may not have been set.
    options.frontend_type = FrontendType::PASSIVE;

    _instantiate_subsystems(options);

    _real_time_controller = std::make_unique<RealTimeController>(
        static_cast<audio_frontend::PassiveFrontend*>(_audio_frontend.get()),
        static_cast<midi_frontend::PassiveMidiFrontend*>(_midi_frontend.get()),
        _engine->transport());

    return {_make_sushi(), _status};
}

std::unique_ptr<RtController> PassiveFactory::rt_controller()
{
    return std::move(_real_time_controller);
}

InitStatus PassiveFactory::_setup_audio_frontend([[maybe_unused]] const SushiOptions& options,
                                                 const jsonconfig::ControlConfig& config)
{
    int cv_inputs = config.cv_inputs.value_or(0);
    int cv_outputs = config.cv_outputs.value_or(0);

    SUSHI_LOG_INFO("Setting up passive frontend");
    _frontend_config = std::make_unique<audio_frontend::PassiveFrontendConfiguration>(cv_inputs, cv_outputs);

    _audio_frontend = std::make_unique<audio_frontend::PassiveFrontend>(_engine.get());

    return InitStatus::OK;
}

InitStatus PassiveFactory::_set_up_midi([[maybe_unused]] const SushiOptions& options,
                                        const jsonconfig::ControlConfig& config)
{
    // Will always be 1 & 1 for passive.
    int midi_inputs = config.midi_inputs.value_or(1);
    int midi_outputs = config.midi_outputs.value_or(1);
    _midi_dispatcher->set_midi_inputs(midi_inputs);
    _midi_dispatcher->set_midi_outputs(midi_outputs);

    _midi_frontend = std::make_unique<midi_frontend::PassiveMidiFrontend>(_midi_dispatcher.get());

    return InitStatus::OK;
}

InitStatus PassiveFactory::_set_up_control([[maybe_unused]] const SushiOptions& options,
                                           [[maybe_unused]] jsonconfig::JsonConfigurator* configurator)
{
    _engine_controller = std::make_unique<engine::Controller>(_engine.get(),
                                                              _midi_dispatcher.get(),
                                                              _audio_frontend.get());

    if (options.use_osc)
    {
        auto oscpack_messenger = new osc::OscpackOscMessenger(options.osc_server_port,
                                                              options.osc_send_port,
                                                              options.osc_send_ip);

        _osc_frontend = std::make_unique<control_frontend::OSCFrontend>(_engine.get(),
                                                                        _engine_controller.get(),
                                                                        oscpack_messenger);

        _engine_controller->set_osc_frontend(_osc_frontend.get());

        auto osc_status = _osc_frontend->init();
        if (osc_status != control_frontend::ControlFrontendStatus::OK)
        {
            return InitStatus::FAILED_OSC_FRONTEND_INITIALIZATION;
        }

        if (configurator)
        {
            configurator->set_osc_frontend(_osc_frontend.get());

            auto status = configurator->load_osc();
            if (status != jsonconfig::JsonConfigReturnStatus::OK &&
                status != jsonconfig::JsonConfigReturnStatus::NOT_DEFINED)
            {
                return InitStatus::FAILED_LOAD_OSC;
            }
        }
    }

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
    _rpc_server = std::make_unique<sushi_rpc::GrpcServer>(options.grpc_listening_address, _engine_controller.get());
    SUSHI_LOG_INFO("Instantiating gRPC server with address: {}", options.grpc_listening_address);
#endif

    return InitStatus::OK;
}

InitStatus PassiveFactory::_load_json_events([[maybe_unused]] const SushiOptions& options,
                                             jsonconfig::JsonConfigurator* configurator)
{
    auto status = configurator->load_events();

    if (status != jsonconfig::JsonConfigReturnStatus::OK &&
        status != jsonconfig::JsonConfigReturnStatus::NOT_DEFINED)
    {
        return InitStatus::FAILED_LOAD_EVENTS;
    }

    return InitStatus::OK;
}

} // namespace sushi