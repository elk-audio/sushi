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
 * @brief Helper for asynchronous communication
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include <thread>

#include "elklog/static_logger.h"

#include "engine/receiver.h"

ELKLOG_GET_LOGGER_WITH_MODULE_NAME("event_receiver");

namespace sushi::internal::receiver {

constexpr int MAX_RETRIES = 100;

bool AsynchronousEventReceiver::wait_for_response(EventId id, std::chrono::milliseconds timeout)
{
    int retries = 0;
    while (retries < MAX_RETRIES)
    {
        RtEvent event;
        while (_queue->pop(event))
        {
            if (is_returnable_event(event))
            {
                auto typed_event = event.returnable_event();
                if (typed_event->event_id() == id)
                {
                    auto handled_ok = typed_event->status() == ReturnableRtEvent::EventStatus::HANDLED_OK;
                    ELKLOG_LOG_ERROR_IF(handled_ok == false, "RtEvent with id {} returned with error", id);
                    return handled_ok;
                }
                bool status = (typed_event->status() == ReturnableRtEvent::EventStatus::HANDLED_OK);
                _receive_list.push_back(Node{typed_event->event_id(), status});
            }
        }
        for (auto i = _receive_list.begin(); i != _receive_list.end(); ++i)
        {
            if (i->id == id)
            {
                bool status = i->status;
                _receive_list.erase(i);
                return status;
            }
        }
        std::this_thread::sleep_for(timeout / MAX_RETRIES);
        retries++;
    }
    ELKLOG_LOG_WARNING("Waiting for RtEvent with id {} timed out", id);
    return false;
}

} // end namespace sushi::internal::receiver
