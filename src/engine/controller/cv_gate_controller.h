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
 * @brief Implementation of external control interface for sushi.
 * @copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_CV_GATE_CONTROLLER_H
#define SUSHI_CV_GATE_CONTROLLER_H

#include "control_interface.h"
#include "engine/base_engine.h"
#include "engine/base_event_dispatcher.h"

namespace sushi {
namespace engine {
namespace controller_impl {

class CvGateController : public ext::CvGateController
{
public:
    explicit CvGateController([[maybe_unused]] engine::BaseEngine* engine) {}

    ~CvGateController() override = default;

    int get_cv_input_ports() const override;

    int get_cv_output_ports() const override;

    std::vector<ext::CvConnection> get_all_cv_input_connections() const override;

    std::vector<ext::CvConnection> get_all_cv_output_connections() const override;

    std::vector<ext::GateConnection> get_all_gate_input_connections() const override;

    std::vector<ext::GateConnection> get_all_gate_output_connections() const override;

    std::pair<ext::ControlStatus, std::vector<ext::CvConnection>>
    get_cv_input_connections_for_processor(int processor_id) const override;

    std::pair<ext::ControlStatus, std::vector<ext::CvConnection>>
    get_cv_output_connections_for_processor(int processor_id) const override;

    std::pair<ext::ControlStatus, std::vector<ext::GateConnection>>
    get_gate_input_connections_for_processor(int processor_id) const override;

    std::pair<ext::ControlStatus, std::vector<ext::GateConnection>>
    get_gate_output_connections_for_processor(int processor_id) const override;

    ext::ControlStatus connect_cv_input_to_parameter(int processor_id, int parameter_id, int cv_input_id) override;

    ext::ControlStatus connect_cv_output_from_parameter(int processor_id, int parameter_id, int cv_output_id) override;

    ext::ControlStatus connect_gate_input_to_processor(int processor_id, int gate_input_id, int channel, int note_no) override;

    ext::ControlStatus connect_gate_output_from_processor(int processor_id, int gate_output_id, int channel, int note_no) override;

    ext::ControlStatus disconnect_cv_input(int processor_id, int parameter_id, int cv_input_id) override;

    ext::ControlStatus disconnect_cv_output(int processor_id, int parameter_id, int cv_output_id) override;

    ext::ControlStatus disconnect_gate_input(int processor_id, int gate_input_id, int channel, int note_no) override;

    ext::ControlStatus disconnect_gate_output(int processor_id, int gate_output_id, int channel, int note_no) override;

    ext::ControlStatus disconnect_all_cv_inputs_from_processor(int processor_id) override;

    ext::ControlStatus disconnect_all_cv_outputs_from_processor(int processor_id) override;

    ext::ControlStatus disconnect_all_gate_inputs_from_processor(int processor_id) override;

    ext::ControlStatus disconnect_all_gate_outputs_from_processor(int processor_id) override;

private:
};

} // namespace controller_impl
} // namespace engine
} // namespace sushi

#endif //SUSHI_CV_GATE_CONTROLLER_H
