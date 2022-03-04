/*
 * Copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk
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
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "session_controller.h"
#include "logging.h"

SUSHI_GET_LOGGER_WITH_MODULE_NAME("controller");

namespace sushi {
namespace engine {
namespace controller_impl {

SessionController::SessionController(BaseEngine* engine,
                                     midi_dispatcher::MidiDispatcher* midi_dispatcher) : _event_dispatcher(engine->event_dispatcher()),
                                                                                         _engine(engine),
                                                                                         _midi_dispatcher(midi_dispatcher)
{

}

void SessionController::set_osc_frontend(control_frontend::OSCFrontend* osc_frontend)
{
    _osc_frontend = osc_frontend;
}

ext::SessionState SessionController::save_session() const
{
    return ext::SessionState();
}

ext::ControlStatus SessionController::restore_session(const ext::SessionState& state)
{
    return ext::ControlStatus::OUT_OF_RANGE;
}

} // namespace controller_impl
} // namespace engine
} // namespace sushi
