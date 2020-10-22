/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Sushi Async gRPC Call Data implementation. Objects to handle async calls to sushi.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "async_service_call_data.h"
#include "control_service.h"

namespace sushi_rpc {

void CallData::_alert()
{
    _in_completion_queue = true;
    _alarm.Set(_async_rpc_queue, gpr_now(gpr_clock_type::GPR_CLOCK_REALTIME), this);
}

void CallData::stop()
{
    _status = CallStatus::FINISH;
}

} // namespace sushi_rpc
