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
 * @brief Interface for events handlers/dispatchers/queues
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 *
 * Abstract interface for classes that receive and pass on events
 * to other. In some sense separate from Processor since this is
 * not intended to process or consume events, only pass them on.
 */

#ifndef SUSHI_RT_EVENT_PIPE_H
#define SUSHI_RT_EVENT_PIPE_H

#include "library/rt_event.h"

#include "elk-warning-suppressor/warning_suppressor.hpp"

ELK_PUSH_WARNING
ELK_DISABLE_TRIVIAL_DESTRUCTOR_NOT_VIRTUAL

namespace sushi::internal {

class RtEventPipe
{
public:
    virtual void send_event(const RtEvent& event) = 0;
};

} // end namespace sushi::internal

ELK_POP_WARNING

#endif // SUSHI_RT_EVENT_PIPE_H
