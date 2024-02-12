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
 * @brief Concrete implementation of the Sushi factory for reactive use.
 *        This is a PIMPL class of sorts, used inside reactive_factory.
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include "elklog/static_logger.h"
#include "sushi/utils.h"

#include "reactive_factory_implementation.h"

#include "audio_frontends/reactive_frontend.h"
#include "control_frontends/reactive_midi_frontend.h"
#include "engine/audio_engine.h"

#include "src/concrete_sushi.h"

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
#include "sushi_rpc/grpc_server.h"
#endif

#include "engine/json_configurator.h"

#include "engine/controller/real_time_controller.h"
#include "control_frontends/oscpack_osc_messenger.h"

namespace sushi::internal {

ELKLOG_GET_LOGGER_WITH_MODULE_NAME("reactive-factory");

ReactiveFactoryImplementation::ReactiveFactoryImplementation() = default;

ReactiveFactoryImplementation::~ReactiveFactoryImplementation() = default;

std::pair<std::unique_ptr<Sushi>, Status> ReactiveFactoryImplementation::new_instance(SushiOptions& options)
{
    init_logger(options); // This can only be called once.

    // Overriding whatever frontend choice may or may not have been set.
    options.frontend_type = FrontendType::REACTIVE;

    _instantiate_subsystems(options);

    _real_time_controller = std::make_unique<RealTimeController>(
        static_cast<audio_frontend::ReactiveFrontend*>(_audio_frontend.get()), // because -frtti is not used
        static_cast<midi_frontend::ReactiveMidiFrontend*>(_midi_frontend.get()), // because -frtti is not used0
        _engine->transport());

    return {_make_sushi(), _status};
}

std::unique_ptr<RtController> ReactiveFactoryImplementation::rt_controller()
{
    return std::move(_real_time_controller);
}

Status ReactiveFactoryImplementation::_setup_audio_frontend([[maybe_unused]] const SushiOptions& options,
                                                            const jsonconfig::ControlConfig& config)
{
    int cv_inputs = config.cv_inputs.value_or(0);
    int cv_outputs = config.cv_outputs.value_or(0);

    ELKLOG_LOG_INFO("Setting up reactive frontend");
    _frontend_config = std::make_unique<audio_frontend::ReactiveFrontendConfiguration>(cv_inputs, cv_outputs);

    _audio_frontend = std::make_unique<audio_frontend::ReactiveFrontend>(_engine.get());

    return Status::OK;
}

Status ReactiveFactoryImplementation::_set_up_midi([[maybe_unused]] const SushiOptions& options,
                                                   const jsonconfig::ControlConfig& config)
{
    // Will always be 1 & 1 for reactive frontend.
    int midi_inputs = config.midi_inputs.value_or(1);
    int midi_outputs = config.midi_outputs.value_or(1);
    _midi_dispatcher->set_midi_inputs(midi_inputs);
    _midi_dispatcher->set_midi_outputs(midi_outputs);

    _midi_frontend = std::make_unique<midi_frontend::ReactiveMidiFrontend>(_midi_dispatcher.get());

    return Status::OK;
}

Status ReactiveFactoryImplementation::_load_json_events([[maybe_unused]] const SushiOptions& options,
                                                        jsonconfig::JsonConfigurator* configurator)
{
    auto status = configurator->load_events();

    if (status != jsonconfig::JsonConfigReturnStatus::OK &&
        status != jsonconfig::JsonConfigReturnStatus::NOT_DEFINED)
    {
        return Status::FAILED_LOAD_EVENTS;
    }

    return Status::OK;
}

} // end namespace sushi::internal