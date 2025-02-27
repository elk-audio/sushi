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
 * @brief Concrete implementation of the Sushi factory for standalone use.
 *        This is a PIMPL class of sorts, used inside standalone_factory.
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#define TWINE_EXPOSE_INTERNALS
#include "twine/twine.h"

#include "elklog/static_logger.h"

#include "standalone_factory_implementation.h"

#include "engine/audio_engine.h"
#include "src/concrete_sushi.h"

#ifdef SUSHI_BUILD_WITH_ALSA_MIDI
#include "control_frontends/alsa_midi_frontend.h"
#endif

#ifdef SUSHI_BUILD_WITH_RT_MIDI
#include "control_frontends/rt_midi_frontend.h"
#endif

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
#include "sushi_rpc/grpc_server.h"
#endif

#include "audio_frontends/jack_frontend.h"
#include "audio_frontends/portaudio_frontend.h"
#include "audio_frontends/xenomai_raspa_frontend.h"

#include "engine/json_configurator.h"
#include "control_frontends/oscpack_osc_messenger.h"

#include "audio_frontends/apple_coreaudio_frontend.h"

namespace sushi::internal {

ELKLOG_GET_LOGGER_WITH_MODULE_NAME("standalone-factory");

StandaloneFactoryImplementation::StandaloneFactoryImplementation() = default;

StandaloneFactoryImplementation::~StandaloneFactoryImplementation() = default;

std::pair<std::unique_ptr<Sushi>, Status> StandaloneFactoryImplementation::new_instance(SushiOptions& options)
{

#ifdef SUSHI_BUILD_WITH_RASPA
    auto raspa_status = audio_frontend::XenomaiRaspaFrontend::global_init();
    if (raspa_status < 0)
    {
        _status = Status::FAILED_XENOMAI_INITIALIZATION;
        return {nullptr, _status};
    }

    if (options.frontend_type == FrontendType::XENOMAI_RASPA)
    {
        twine::init_xenomai(); // must be called before setting up any worker pools
    }
#endif

    _instantiate_subsystems(options);

    return {_make_sushi(), _status};
}

Status
    StandaloneFactoryImplementation::_setup_audio_frontend(const SushiOptions& options,
                                                           const jsonconfig::ControlConfig& config)
{
    int cv_inputs = config.cv_inputs.value_or(0);
    int cv_outputs = config.cv_outputs.value_or(0);

    switch (options.frontend_type)
    {
        case FrontendType::JACK:
        {
#ifdef SUSHI_BUILD_WITH_JACK
            ELKLOG_LOG_INFO("Setting up Jack audio frontend");
            _frontend_config = std::make_unique<audio_frontend::JackFrontendConfiguration>(options.jack_client_name,
                                                                                           options.jack_server_name,
                                                                                           options.connect_ports,
                                                                                           cv_inputs,
                                                                                           cv_outputs);

            _audio_frontend = std::make_unique<audio_frontend::JackFrontend>(_engine.get());
#endif
            break;
        }
        case FrontendType::PORTAUDIO:
        {
            ELKLOG_LOG_INFO("Setting up PortAudio frontend");
            _frontend_config = std::make_unique<audio_frontend::PortAudioFrontendConfiguration>(options.portaudio_input_device_id,
                                                                                                options.portaudio_output_device_id,
                                                                                                options.suggested_input_latency,
                                                                                                options.suggested_output_latency,
                                                                                                cv_inputs,
                                                                                                cv_outputs);

            _audio_frontend = std::make_unique<audio_frontend::PortAudioFrontend>(_engine.get());
            break;
        }

        case FrontendType::APPLE_COREAUDIO:
        {
            ELKLOG_LOG_INFO("Setting up Apple CoreAudio frontend");

            _frontend_config = std::make_unique<audio_frontend::AppleCoreAudioFrontendConfiguration>(options.apple_coreaudio_input_device_uid,
                                                                                                     options.apple_coreaudio_output_device_uid,
                                                                                                     cv_inputs,
                                                                                                     cv_outputs);

            _audio_frontend = std::make_unique<audio_frontend::AppleCoreAudioFrontend>(_engine.get());
            break;
        }

#ifdef SUSHI_BUILD_WITH_RASPA
        case FrontendType::XENOMAI_RASPA:
        {
            ELKLOG_LOG_INFO("Setting up Xenomai RASPA frontend");
            _frontend_config = std::make_unique<audio_frontend::XenomaiRaspaFrontendConfiguration>(options.debug_mode_switches,
                                                                                                   cv_inputs,
                                                                                                   cv_outputs);

            _audio_frontend = std::make_unique<audio_frontend::XenomaiRaspaFrontend>(_engine.get());
            break;
        }
#endif
        case FrontendType::DUMMY:
        case FrontendType::OFFLINE:
        {
            ELKLOG_LOG_ERROR("Standalone Factory cannot be used to create DUMMY/OFFLINE frontends.");
            assert(false);
            break;
        }

        default:
        {
            return Status::FAILED_AUDIO_FRONTEND_MISSING;
        }
    }

    return Status::OK;
}

Status StandaloneFactoryImplementation::_set_up_midi([[maybe_unused]] const SushiOptions& options,
                                                     const jsonconfig::ControlConfig& config)
{
    int midi_inputs = config.midi_inputs.value_or(1);
    int midi_outputs = config.midi_outputs.value_or(1);
    _midi_dispatcher->set_midi_inputs(midi_inputs);
    _midi_dispatcher->set_midi_outputs(midi_outputs);

#ifdef SUSHI_BUILD_WITH_ALSA_MIDI
    _midi_frontend = std::make_unique<midi_frontend::AlsaMidiFrontend>(midi_inputs,
                                                                       midi_outputs,
                                                                       _midi_dispatcher.get());
#elif SUSHI_BUILD_WITH_RT_MIDI
    auto rt_midi_input_mappings = config.rt_midi_input_mappings;
    auto rt_midi_output_mappings = config.rt_midi_output_mappings;

    _midi_frontend = std::make_unique<midi_frontend::RtMidiFrontend>(midi_inputs,
                                                                     midi_outputs,
                                                                     std::move(rt_midi_input_mappings),
                                                                     std::move(rt_midi_output_mappings),
                                                                     _midi_dispatcher.get());
#else
    _midi_frontend = std::make_unique<midi_frontend::NullMidiFrontend>(midi_inputs,
                                                                       midi_outputs,
                                                                       _midi_dispatcher.get());
#endif

    return Status::OK;
}

Status StandaloneFactoryImplementation::_load_json_events([[maybe_unused]] const SushiOptions& options,
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
