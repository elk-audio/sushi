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

#include "cv_gate_controller.h"


namespace sushi::internal::engine::controller_impl {

// TODO - Remove when stubs have been properly implemented
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"

int CvGateController::get_cv_input_ports() const
{
    return 0;
}

int CvGateController::get_cv_output_ports() const
{
    return 0;
}

std::vector<control::CvConnection> CvGateController::get_all_cv_input_connections() const
{
    return {};
}

std::vector<control::CvConnection> CvGateController::get_all_cv_output_connections() const
{
    return {};
}

std::vector<control::GateConnection> CvGateController::get_all_gate_input_connections() const
{
    return {};
}

std::vector<control::GateConnection> CvGateController::get_all_gate_output_connections() const
{
    return {};
}

std::pair<control::ControlStatus, std::vector<control::CvConnection>>
CvGateController::get_cv_input_connections_for_processor(int processor_id) const
{
    return {control::ControlStatus::UNSUPPORTED_OPERATION, std::vector<control::CvConnection>()};
}

std::pair<control::ControlStatus, std::vector<control::CvConnection>>
CvGateController::get_cv_output_connections_for_processor(int processor_id) const
{
    return {control::ControlStatus::UNSUPPORTED_OPERATION, std::vector<control::CvConnection>()};
}

std::pair<control::ControlStatus, std::vector<control::GateConnection>>
CvGateController::get_gate_input_connections_for_processor(int processor_id) const
{
    return {control::ControlStatus::UNSUPPORTED_OPERATION, std::vector<control::GateConnection>()};
}

std::pair<control::ControlStatus, std::vector<control::GateConnection>>
CvGateController::get_gate_output_connections_for_processor(int processor_id) const
{
    return {control::ControlStatus::UNSUPPORTED_OPERATION, std::vector<control::GateConnection>()};
}

control::ControlStatus CvGateController::connect_cv_input_to_parameter(int processor_id, int parameter_id, int cv_input_id)
{
    return control::ControlStatus::UNSUPPORTED_OPERATION;
}

control::ControlStatus CvGateController::connect_cv_output_from_parameter(int processor_id, int parameter_id, int cv_output_id)
{
    return control::ControlStatus::UNSUPPORTED_OPERATION;
}

control::ControlStatus CvGateController::connect_gate_input_to_processor(int processor_id, int gate_input_id, int channel, int note_no)
{
    return control::ControlStatus::UNSUPPORTED_OPERATION;
}

control::ControlStatus CvGateController::connect_gate_output_from_processor(int processor_id, int gate_output_id, int channel, int note_no)
{
    return control::ControlStatus::UNSUPPORTED_OPERATION;
}

control::ControlStatus CvGateController::disconnect_cv_input(int processor_id, int parameter_id, int cv_input_id)
{
    return control::ControlStatus::UNSUPPORTED_OPERATION;
}

control::ControlStatus CvGateController::disconnect_cv_output(int processor_id, int parameter_id, int cv_output_id)
{
    return control::ControlStatus::UNSUPPORTED_OPERATION;
}

control::ControlStatus CvGateController::disconnect_gate_input(int processor_id, int gate_input_id, int channel, int note_no)
{
    return control::ControlStatus::UNSUPPORTED_OPERATION;
}

control::ControlStatus CvGateController::disconnect_gate_output(int processor_id, int gate_output_id, int channel, int note_no)
{
    return control::ControlStatus::UNSUPPORTED_OPERATION;
}

control::ControlStatus CvGateController::disconnect_all_cv_inputs_from_processor(int processor_id)
{
    return control::ControlStatus::UNSUPPORTED_OPERATION;
}

control::ControlStatus CvGateController::disconnect_all_cv_outputs_from_processor(int processor_id)
{
    return control::ControlStatus::UNSUPPORTED_OPERATION;
}

control::ControlStatus CvGateController::disconnect_all_gate_inputs_from_processor(int processor_id)
{
    return control::ControlStatus::UNSUPPORTED_OPERATION;
}

control::ControlStatus CvGateController::disconnect_all_gate_outputs_from_processor(int processor_id)
{
    return control::ControlStatus::UNSUPPORTED_OPERATION;
}

#pragma GCC diagnostic pop

} // end namespace sushi::internal::engine::controller_impl