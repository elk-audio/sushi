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
 * @brief Implementation of external control interface for sushi.
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef SUSHI_PARAMETER_CONTROLLER_H
#define SUSHI_PARAMETER_CONTROLLER_H

#include "sushi/control_interface.h"

#include "engine/base_event_dispatcher.h"
#include "engine/base_processor_container.h"

namespace sushi::internal::engine {

class BaseEngine;

namespace controller_impl {

class ParameterController : public control::ParameterController
{
public:
    explicit ParameterController(BaseEngine* engine);

    std::pair<control::ControlStatus, std::vector<control::ParameterInfo>> get_processor_parameters(int processor_id) const override;

    std::pair<control::ControlStatus, std::vector<control::ParameterInfo>> get_track_parameters(int track_id) const override;

    std::pair<control::ControlStatus, int> get_parameter_id(int processor_id, const std::string& parameter) const override;

    std::pair<control::ControlStatus, control::ParameterInfo> get_parameter_info(int processor_id, int parameter_id) const override;

    std::pair<control::ControlStatus, float> get_parameter_value(int processor_id, int parameter_id) const override;

    std::pair<control::ControlStatus, float> get_parameter_value_in_domain(int processor_id, int parameter_id) const override;

    std::pair<control::ControlStatus, std::string> get_parameter_value_as_string(int processor_id, int parameter_id) const override;

    control::ControlStatus set_parameter_value(int processor_id, int parameter_id, float value) override;

    std::pair<control::ControlStatus, std::vector<control::PropertyInfo>>  get_processor_properties(int processor_id) const override;

    std::pair<control::ControlStatus, std::vector<control::PropertyInfo>>  get_track_properties(int processor_id) const override;

    std::pair<control::ControlStatus, int> get_property_id(int processor_id, const std::string& property_name) const override;

    std::pair<control::ControlStatus, control::PropertyInfo> get_property_info(int processor_id, int property_id) const override;

    std::pair<control::ControlStatus, std::string> get_property_value(int processor_id, int parameter_id) const override;

    control::ControlStatus set_property_value(int processor_id, int property_id, const std::string& value) override;

private:
    dispatcher::BaseEventDispatcher*        _event_dispatcher;
    const engine::BaseProcessorContainer*   _processors;
};

} // end namespace controller_impl

} // end namespace sushi::internal::engine

#endif // SUSHI_PARAMETER_CONTROLLER_H
