/*
 * Copyright 2017-2023 Elk Audio AB
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
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef SUSHI_OSC_CONTROLLER_H
#define SUSHI_OSC_CONTROLLER_H

#include "sushi/control_interface.h"

#include "control_frontends/osc_frontend.h"
#include "engine/base_engine.h"
#include "engine/base_processor_container.h"

namespace sushi::internal::engine::controller_impl {

class OscController : public control::OscController
{
public:
    explicit OscController(BaseEngine* engine);

    void set_osc_frontend(control_frontend::OSCFrontend* osc_frontend);

    ~OscController() override = default;

    std::string get_send_ip() const override;

    int get_send_port() const override;

    int get_receive_port() const override;

    std::vector<std::string> get_enabled_parameter_outputs() const override;

    control::ControlStatus enable_output_for_parameter(int processor_id, int parameter_id) override;

    control::ControlStatus disable_output_for_parameter(int processor_id, int parameter_id) override;

    control::ControlStatus enable_all_output() override;

    control::ControlStatus disable_all_output() override;

private:
    dispatcher::BaseEventDispatcher* _event_dispatcher;
    control_frontend::OSCFrontend* _osc_frontend {nullptr};
    const engine::BaseProcessorContainer* _processors;
};

} // end namespace sushi::internal::engine::controller_impl

#endif // SUSHI_OSC_CONTROLLER_H
