/*
 * Copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI.  If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Implementation of external control interface of sushi.
 * @copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "osc_controller.h"

namespace sushi {
namespace engine {
namespace controller_impl {

// TODO - Remove when stubs have been properly implemented
#pragma GCC diagnostic ignored "-Wunused-parameter"

OscController::OscController(BaseEngine* engine) : _engine(engine),
                                                   _event_dispatcher(engine->event_dispatcher()),
                                                   _processors(engine->processor_container()) {}

int OscController::get_send_port() const
{
    return _osc_frontend->send_port();
}

int OscController::get_receive_port() const
{
    return _osc_frontend->receive_port();
}

std::vector<std::string> OscController::get_enabled_parameter_outputs() const
{
    return _osc_frontend->get_enabled_parameter_outputs();
}

ext::ControlStatus OscController::enable_output_for_parameter(int processor_id, int parameter_id)
{
    auto lambda = [=] () -> int
    {
        // Here we SHOULD use name, since it is needed for building the OSC "Address Path".
        // We could avoid the _processors dependency here, though not crucial, by having 4 parameters to the call.

        auto processor = _processors->processor(processor_id);
        if (processor == nullptr)
        {
            return EventStatus::ERROR;
        }

        auto parameter_descriptor = processor->parameter_from_id(parameter_id);
        if (parameter_descriptor == nullptr)
        {
            return EventStatus::ERROR;
        }

        bool status = _osc_frontend->connect_from_parameter(processor->name(), parameter_descriptor->name());

        if(status == false)
        {
            return EventStatus::ERROR;
        }

        return EventStatus::HANDLED_OK;
    };

    auto event = new LambdaEvent(lambda, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus OscController::disable_output_for_parameter(int processor_id, int parameter_id)
{
    auto lambda = [=] () -> int
    {
        // Here we SHOULD use name, since it is needed for building the OSC "Address Path".
        // We could avoid the _processors dependency here, though not crucial, by having 4 parameters to the call.

        auto processor = _processors->processor(processor_id);
        if (processor == nullptr)
        {
            return EventStatus::ERROR;
        }

        auto parameter_descriptor = processor->parameter_from_id(parameter_id);
        if (parameter_descriptor == nullptr)
        {
            return EventStatus::ERROR;
        }

        bool status = _osc_frontend->disconnect_from_parameter(processor->name(), parameter_descriptor->name());

        if(status == false)
        {
            return EventStatus::ERROR;
        }

        return EventStatus::HANDLED_OK;
    };

    auto event = new LambdaEvent(lambda, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

void OscController::set_osc_frontend(control_frontend::OSCFrontend* osc_frontend)
{
    _osc_frontend = osc_frontend;
}

ext::ControlStatus OscController::enable_all_output()
{
    auto lambda = [=] () -> int
    {
        _osc_frontend->connect_from_all_parameters();
        return EventStatus::HANDLED_OK;
    };

    auto event = new LambdaEvent(lambda, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

ext::ControlStatus OscController::disable_all_output()
{
    auto lambda = [=] () -> int
    {
        _osc_frontend->disconnect_from_all_parameters();
        return EventStatus::HANDLED_OK;
    };

    auto event = new LambdaEvent(lambda, IMMEDIATE_PROCESS);
    _event_dispatcher->post_event(event);
    return ext::ControlStatus::OK;
}

#pragma GCC diagnostic pop

} // namespace controller_impl
} // namespace engine
} // namespace sushi