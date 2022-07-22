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
 * @brief Class to manage parameter changes, rate limiting and sync between devices
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */
 
#ifndef SUSHI_PARAMETER_MANAGER_H
#define SUSHI_PARAMETER_MANAGER_H

#include <unordered_map>
#include <vector>

#include "library/types.h"
#include "library/constants.h"
#include "library/plugin_parameters.h"
#include "library/event_interface.h"
#include "library/event.h"

namespace sushi {

namespace engine { class BaseProcessorContainer;}
namespace dispatcher { class BaseEventDispatcher;}

class ParameterManager
{
public:
    /**
     * @brief Construct a ParameterManager object
     * @param update_rate The minimum time between 2 consecutive updates for a specific parameter
     * @param processor_container A ProcessorContainer object used to retrieve processors
     */
    ParameterManager(Time update_rate, const engine::BaseProcessorContainer* processor_container);

    /**
     * @brief Add a processor whose parameter values should be tracked
     * @param processor_id The id of the processor
     */
    void track_parameters(ObjectId processor_id);

    /**
     * @brief Remove all tracked parameters of a processor
     * @param processor_id The id of the Processor
     */
    void untrack_parameters(ObjectId processor_id);

    /**
     * @brief Mark a parameter as changed and queue a value update
     * @param processor_id The id of the Processor
     * @param parameter_id The id of the parameter
     * @param timestamp The time at which the value changed, can be in the future
     */
    void mark_parameter_changed(ObjectId processor_id, ObjectId parameter_id, Time timestamp);

    /**
     * @brief Mark all parameters of a processor as changed and queue updates for all parameters
     * @param processor_id The id of the Processor
     * @param timestamp The time at which the values changed, can be in the future
     */
    void mark_processor_changed(ObjectId processor_id, Time timestamp);

    /**
     * @brief Output ParameterChangedNotificationEvents for all queued parameter changes up until
     *        a given timestamp. If a parameter was queued several time, only one notification
     *        will be sent.
     * @param dispatcher The dispatcher to send events to.
     * @param target_time All queued updates with a timestamp equal to or lower that this will be processed
     */
    void output_parameter_notifications(dispatcher::BaseEventDispatcher* dispatcher, Time target_time);

private:
    void _output_parameter_notifications(dispatcher::BaseEventDispatcher* dispatcher, Time timestamp);

    void _output_processor_notifications(dispatcher::BaseEventDispatcher* dispatcher, Time timestamp);

    struct ParameterEntry
    {
        float value;
        Time last_update;
    };

    struct ParameterUpdate
    {
        ObjectId processor_id;
        ObjectId parameter_id;
        Time update_time;
    };

    struct ProcessorUpdate
    {
        ObjectId processor_id;
        Time update_time;
    };

    std::vector<ProcessorUpdate> _processor_change_queue;
    std::vector<ParameterUpdate> _parameter_change_queue;

    const engine::BaseProcessorContainer* _processors;
    Time _update_rate;

    // Note this is only accessed from the event loop thread, so no mutex is needed
    std::unordered_map<ObjectId, std::unordered_map<ObjectId, ParameterEntry>> _parameters;

};
} // namespace sushi
#endif //SUSHI_PARAMETER_MANAGER_H
