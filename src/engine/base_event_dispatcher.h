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
 * @Brief Main event handler in Sushi and responsible for conversion between
 *        regular and rt events.
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef SUSHI_BASE_EVENT_DISPATCHER_H
#define SUSHI_BASE_EVENT_DISPATCHER_H

#include "library/event.h"
#include "library/event_interface.h"

namespace sushi::internal::dispatcher {

enum class Status
{
    OK,
    ALREADY_SUBSCRIBED,
    UNKNOWN_POSTER
};

/* Abstract base class is solely for test mockups */
class BaseEventDispatcher
{
public:
    virtual ~BaseEventDispatcher() = default;

    virtual void run() = 0;
    virtual void stop() = 0;

    virtual void post_event(std::unique_ptr<Event> event) = 0;

    virtual Status subscribe_to_keyboard_events(EventPoster* /*receiver*/)
    {
        return Status::OK;
    }

    virtual Status subscribe_to_parameter_change_notifications(EventPoster* /*receiver*/)
    {
        return Status::OK;
    }

    virtual Status subscribe_to_engine_notifications(EventPoster* /*receiver*/)
    {
        return Status::OK;
    }

    virtual Status unsubscribe_from_keyboard_events(EventPoster* /*receiver*/)
    {
        return Status::OK;
    }

    virtual Status unsubscribe_from_parameter_change_notifications(EventPoster* /*receiver*/)
    {
        return Status::OK;
    }

    virtual Status unsubscribe_from_engine_notifications(EventPoster* /*receiver*/)
    {
        return Status::OK;
    }

    virtual void set_sample_rate(float /*sample_rate*/) = 0;
    virtual void set_time(Time /*timestamp*/)  = 0;

    virtual int dispatch(std::unique_ptr<Event> /*event*/) = 0;
};

} // end namespace sushi::internal::dispatcher

#endif // SUSHI_BASE_EVENT_DISPATCHER_H
