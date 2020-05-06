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

#ifndef SUSHI_OSC_CONTROLLER_H
#define SUSHI_OSC_CONTROLLER_H

#include "control_interface.h"
#include "engine/base_engine.h"

namespace sushi {
namespace engine {
namespace controller_impl {

class OscController : public ext::OscController
{
public:
    OscController(BaseEngine* engine) : _engine(engine) {}

    ~OscController() override = default;

    int get_send_port() const override;

    int get_receive_port() const override;

    std::vector<std::string> get_enabled_parameter_outputs() const override;

    ext::ControlStatus enable_output_for_parameter(int processor_id, int parameter_id) override;

    ext::ControlStatus disable_output_for_parameter(int processor_id, int parameter_id) override;

private:
    BaseEngine* _engine;
};

} // namespace controller_impl
} // namespace engine
} // namespace sushi

#endif //SUSHI_OSC_CONTROLLER_H
