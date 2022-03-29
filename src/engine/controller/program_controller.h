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

#ifndef SUSHI_PROGRAM_CONTROLLER_H
#define SUSHI_PROGRAM_CONTROLLER_H

#include "control_interface.h"
#include "engine/base_engine.h"
#include "engine/base_event_dispatcher.h"
#include "engine/base_processor_container.h"

namespace sushi {
namespace engine {
namespace controller_impl {

class ProgramController : public ext::ProgramController
{
public:
    explicit ProgramController(BaseEngine* engine);

    ~ProgramController() override = default;

    std::pair<ext::ControlStatus, int> get_processor_current_program(int processor_id) const override;

    std::pair<ext::ControlStatus, std::string> get_processor_current_program_name(int processor_id) const override;

    std::pair<ext::ControlStatus, std::string> get_processor_program_name(int processor_id, int program_id) const override;

    std::pair<ext::ControlStatus, std::vector<std::string>> get_processor_programs(int processor_id) const override;

    ext::ControlStatus set_processor_program(int processor_id, int program_id) override;

private:
    dispatcher::BaseEventDispatcher*    _event_dispatcher;
    const BaseProcessorContainer*       _processors;
};

} // namespace controller_impl
} // namespace engine
} // namespace sushi

#endif //SUSHI_PROGRAM_CONTROLLER_H
