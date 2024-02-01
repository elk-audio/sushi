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
 * @brief Concrete implementation of the Sushi factory for offline use.
 *        This is a PIMPL class of sorts, used inside offline_factory.
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include "elklog/static_logger.h"

#include "offline_factory_implementation.h"

#include "engine/audio_engine.h"
#include "src/concrete_sushi.h"

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
#include "sushi_rpc/grpc_server.h"
#endif

#include "audio_frontends/offline_frontend.h"

#include "engine/json_configurator.h"

namespace sushi::internal {

ELKLOG_GET_LOGGER_WITH_MODULE_NAME("offline-factory");

OfflineFactoryImplementation::OfflineFactoryImplementation() = default;

OfflineFactoryImplementation::~OfflineFactoryImplementation() = default;

std::pair<std::unique_ptr<Sushi>, Status> OfflineFactoryImplementation::new_instance(SushiOptions& options)
{
    // For the offline frontend, OSC control is not supported.
    // So, the SushiOptions flag is overridden.
    options.use_osc = false;

#ifdef TWINE_BUILD_WITH_APPLE_COREAUDIO
    ELKLOG_LOG_WARNING("Using the Offline frontend with more than 1 CPU core is not currently supported on Apple. "
                       "The threads need to be attached to a workgroup, "
                       "which will not exist if there is no audio interface. "
                       "Sushi will proceed to run, but on a single CPU core.");

    options.rt_cpu_cores = 1;
#endif

    _instantiate_subsystems(options);

    return {_make_sushi(), _status};
}

Status OfflineFactoryImplementation::_setup_audio_frontend(const SushiOptions& options,
                                                           const jsonconfig::ControlConfig& config)
{
    int cv_inputs = config.cv_inputs.value_or(0);
    int cv_outputs = config.cv_outputs.value_or(0);

    bool dummy = false;

    if (options.frontend_type != FrontendType::OFFLINE)
    {
        dummy = true;
        ELKLOG_LOG_INFO("Setting up dummy audio frontend");
    }
    else
    {
        ELKLOG_LOG_INFO("Setting up offline audio frontend");
    }

    _frontend_config = std::make_unique<audio_frontend::OfflineFrontendConfiguration>(options.input_filename,
                                                                                      options.output_filename,
                                                                                      dummy,
                                                                                      cv_inputs,
                                                                                      cv_outputs);

    _audio_frontend = std::make_unique<audio_frontend::OfflineFrontend>(_engine.get());

    return Status::OK;
}

Status OfflineFactoryImplementation::_set_up_midi([[maybe_unused]] const SushiOptions& options,
                                                  const jsonconfig::ControlConfig& config)
{
    int midi_inputs = config.midi_inputs.value_or(1);
    int midi_outputs = config.midi_outputs.value_or(1);
    _midi_dispatcher->set_midi_inputs(midi_inputs);
    _midi_dispatcher->set_midi_outputs(midi_outputs);

    _midi_frontend = std::make_unique<midi_frontend::NullMidiFrontend>(_midi_dispatcher.get());

    return Status::OK;
}

Status OfflineFactoryImplementation::_load_json_events([[maybe_unused]] const SushiOptions& options,
                                                       jsonconfig::JsonConfigurator* configurator)
{
    auto [status, events] = configurator->load_event_list();

    if (status == jsonconfig::JsonConfigReturnStatus::OK)
    {
        static_cast<audio_frontend::OfflineFrontend*>(_audio_frontend.get())->add_sequencer_events(std::move(events));
    }
    else if (status != jsonconfig::JsonConfigReturnStatus::NOT_DEFINED)
    {
        return Status::FAILED_LOAD_EVENT_LIST;
    }

    return Status::OK;
}

} // end namespace sushi::internal
