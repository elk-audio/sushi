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

#include "base_factory.h"

#include "engine/audio_engine.h"
#include "engine/json_configurator.h"

#include "concrete_sushi.h"
#include "sushi/json_utils.h"

namespace sushi::internal {

BaseFactory::BaseFactory() = default;

BaseFactory::~BaseFactory() = default;

std::unique_ptr<Sushi> BaseFactory::_make_sushi()
{
    if (_status == Status::OK)
    {
        // It's clearer to have FactoryBase as friend of Sushi.
        auto sushi = std::unique_ptr<ConcreteSushi>(new ConcreteSushi());

        sushi->_engine = std::move(_engine);
        sushi->_midi_dispatcher = std::move(_midi_dispatcher);
        sushi->_midi_frontend = std::move(_midi_frontend);
        sushi->_osc_frontend = std::move(_osc_frontend);
        sushi->_audio_frontend = std::move(_audio_frontend);
        sushi->_frontend_config = std::move(_frontend_config);
        sushi->_engine_controller = std::move(_engine_controller);

#ifdef SUSHI_BUILD_WITH_RPC_INTERFACE
        sushi->_rpc_server = std::move(_rpc_server);
#endif
        return sushi;
    }
    else
    {
        return nullptr;
    }
}

void BaseFactory::_instantiate_subsystems(SushiOptions& options)
{
    _engine = std::make_unique<engine::AudioEngine>(SUSHI_SAMPLE_RATE_DEFAULT,
                                                    options.rt_cpu_cores,
                                                    options.debug_mode_switches);

    if (!options.base_plugin_path.empty())
    {
        _engine->set_base_plugin_path(options.base_plugin_path);
    }

    if (options.enable_timings)
    {
        _engine->performance_timer()->enable(true);
    }

    _midi_dispatcher = std::make_unique<midi_dispatcher::MidiDispatcher>(_engine->event_dispatcher());

    if (options.config_source == ConfigurationSource::FILE)
    {
        _status = _configure_from_file(options);
    }
    else if (options.config_source == ConfigurationSource::JSON_STRING)
    {
        _status = _configure_from_json(options);
    }
    else if (options.config_source == ConfigurationSource::DEFAULT)
    {
        _status = _configure_with_defaults(options);
    }
}

Status BaseFactory::_configure_from_file(SushiOptions& options)
{
    bool status;

    std::tie(status, options.json_string) = load_config_file(options.config_filename);

    if (!status)
    {
        return Status::FAILED_INVALID_FILE_PATH;
    }

    return _configure_from_json(options);
}

Status BaseFactory::_configure_from_json(SushiOptions& options)
{
    auto configurator = std::make_unique<jsonconfig::JsonConfigurator>(_engine.get(),
                                                                        _midi_dispatcher.get(),
                                                                        _engine->processor_container(),
                                                                        options.json_string);

    auto [control_config_status, control_config] = configurator->load_control_config();
    if (control_config_status != jsonconfig::JsonConfigReturnStatus::OK)
    {
        return Status::FAILED_INVALID_CONFIGURATION_FILE;
    }

    auto engine_status = _configure_engine(options, control_config, configurator.get());
    if (engine_status != Status::OK)
    {
        return engine_status;
    }

    auto configuration_status = _load_json_configuration(configurator.get());
    if (configuration_status != Status::OK)
    {
        return configuration_status;
    }

    auto event_status = _load_json_events(options, configurator.get());
    if (event_status != Status::OK)
    {
        return event_status;
    }

    return Status::OK;
}

Status BaseFactory::_configure_with_defaults(SushiOptions& options)
{
    jsonconfig::ControlConfig control_config;
    control_config.midi_inputs = 1;
    control_config.midi_outputs = 1;
    control_config.cv_inputs = 0;
    control_config.cv_outputs = 0;

    return _configure_engine(options, control_config, nullptr); // nullptr for configurator
}

Status BaseFactory::_configure_engine(SushiOptions& options,
                                      const jsonconfig::ControlConfig& control_config,
                                      jsonconfig::JsonConfigurator* configurator)
{
    auto status = _setup_audio_frontend(options, control_config);

    auto audio_frontend_status = _audio_frontend->init(_frontend_config.get());
    if (audio_frontend_status != audio_frontend::AudioFrontendStatus::OK)
    {
        return Status::FAILED_AUDIO_FRONTEND_INITIALIZATION;
    }

    if (status != Status::OK)
    {
        return status;
    }

    status = _set_up_midi(options, control_config);

    auto midi_ok = _midi_frontend->init();
    if (!midi_ok)
    {
        return Status::FAILED_MIDI_FRONTEND_INITIALIZATION;
    }

    _midi_dispatcher->set_frontend(_midi_frontend.get());

    if (status != Status::OK)
    {
        return status;
    }

    status = _set_up_control(options, configurator);
    if (status != Status::OK)
    {
        return status;
    }

    return Status::OK;
}

Status BaseFactory::_load_json_configuration(jsonconfig::JsonConfigurator* configurator)
{
    auto status = configurator->load_host_config();
    if (status != jsonconfig::JsonConfigReturnStatus::OK)
    {
        return Status::FAILED_LOAD_HOST_CONFIG;
    }

    status = configurator->load_tracks();
    if (status != jsonconfig::JsonConfigReturnStatus::OK)
    {
        return Status::FAILED_LOAD_TRACKS;
    }

    status = configurator->load_midi();
    if (status != jsonconfig::JsonConfigReturnStatus::OK &&
        status != jsonconfig::JsonConfigReturnStatus::NOT_DEFINED)
    {
        return Status::FAILED_LOAD_MIDI_MAPPING;
    }

    status = configurator->load_cv_gate();
    if (status != jsonconfig::JsonConfigReturnStatus::OK &&
        status != jsonconfig::JsonConfigReturnStatus::NOT_DEFINED)
    {
        return Status::FAILED_LOAD_CV_GATE;
    }

    status = configurator->load_initial_state();
    if (status != jsonconfig::JsonConfigReturnStatus::OK &&
        status != jsonconfig::JsonConfigReturnStatus::NOT_DEFINED)
    {
        return Status::FAILED_LOAD_PROCESSOR_STATES;
    }

    return Status::OK;
}

} // end namespace sushi::internal