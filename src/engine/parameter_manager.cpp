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

#include "parameter_manager.h"
#include "library/processor.h"
#include "engine/base_processor_container.h"

namespace sushi {

inline void send_parameter_notification(ObjectId processor_id,
                                        ObjectId parameter_id,
                                        float normalized_value,
                                        float domain_value,
                                        const std::string& formatted_value,
                                        dispatcher::BaseEventDispatcher* dispatcher)
{
    ParameterChangeNotificationEvent event(processor_id,
                                           parameter_id,
                                           normalized_value,
                                           domain_value,
                                           formatted_value,
                                           IMMEDIATE_PROCESS);
    dispatcher->process(&event);
}

ParameterManager::ParameterManager(Time update_rate,
                                   const sushi::engine::BaseProcessorContainer* processor_container) : _processors(processor_container),
                                                                                                       _update_rate(update_rate)
{}

void ParameterManager::track_parameters(ObjectId processor_id)
{
    if (auto processor = _processors->processor(processor_id))
    {
        auto& param_map = _parameters[processor->id()];

        for (const auto& p: processor->all_parameters())
        {
            auto type = p->type();
            if (type == ParameterType::BOOL || type == ParameterType::INT || type == ParameterType::FLOAT)
            {
                param_map.insert({p->id(), {.value = processor->parameter_value(p->id()).second, .last_update = Time(0)}});
            }
        }
    }
}

void ParameterManager::untrack_parameters(ObjectId processor_id)
{
    _parameters.erase(processor_id);
}

void ParameterManager::mark_parameter_changed(ObjectId processor_id, ObjectId parameter_id, Time timestamp)
{
    _parameter_change_queue.push_back(ParameterUpdate{processor_id, parameter_id, timestamp});
}

void ParameterManager::mark_processor_changed(ObjectId processor_id, Time timestamp)
{
    auto entry = std::find_if(_processor_change_queue.begin(),
                              _processor_change_queue.end(),
                              [&](const auto& i){return i.processor_id == processor_id;});
    if (entry == _processor_change_queue.end())
    {
        _processor_change_queue.push_back(ProcessorUpdate{processor_id, timestamp});
    }
    else
    {
        entry->update_time = timestamp;
    }
}

void ParameterManager::output_parameter_notifications(dispatcher::BaseEventDispatcher* dispatcher, Time target_time)
{
    _output_processor_notifications(dispatcher, target_time);
    _output_parameter_notifications(dispatcher, target_time);
}

void ParameterManager::_output_parameter_notifications(dispatcher::BaseEventDispatcher* dispatcher, Time timestamp)
{
    auto i = _parameter_change_queue.begin();
    auto swap_iter = i;
    while (i != _parameter_change_queue.end())
    {
        if (auto proc_entry = _parameters.find(i->processor_id); proc_entry != _parameters.end())
        {
            auto& entry = proc_entry->second[i->parameter_id];

            /* Send update if the update time has passed and the last update was sent
             * longer than _update_rate ago */
            if (i->update_time <= timestamp && (entry.last_update + _update_rate) <= timestamp)
            {
                if (auto processor = _processors->processor(i->processor_id))
                {
                    float value = processor->parameter_value(i->parameter_id).second;
                    if (value != entry.value)
                    {
                        send_parameter_notification(i->processor_id, i->parameter_id, value,
                                                    processor->parameter_value_in_domain(i->parameter_id).second,
                                                    processor->parameter_value_formatted(i->parameter_id).second,
                                                    dispatcher);
                        entry.last_update = timestamp;
                        entry.value = value;
                    }
                }
            }
            /* If this parameter was not a duplicate, but still updated too recently,
             * put it at the front of the queue and check next time */
            else if (entry.last_update != timestamp)
            {
                std::iter_swap(i, swap_iter);
                swap_iter++;
            }
        }
        i++;
    }
    _parameter_change_queue.erase(swap_iter, _parameter_change_queue.end());
}

void ParameterManager::_output_processor_notifications(dispatcher::BaseEventDispatcher* dispatcher, Time timestamp)
{
    auto i = _processor_change_queue.begin();
    auto swap_iter = i;
    while (i != _processor_change_queue.end())
    {
        /* When notifying all parameters of a processor, we ignore the last_update time
         * and send a notification anyway, regardless if one was sent recently */
        if (i->update_time <= timestamp)
        {
            if (auto processor = _processors->processor(i->processor_id))
            {
                auto& param_entries = _parameters[i->processor_id];
                for (const auto& p: param_entries)
                {
                    auto& entry = param_entries[p.first];
                    float value = processor->parameter_value(p.first).second;
                    if (value != entry.value)
                    {
                        send_parameter_notification(i->processor_id, p.first, value,
                                                    processor->parameter_value_in_domain(p.first).second,
                                                    processor->parameter_value_formatted(p.first).second, dispatcher);
                        entry.value = value;
                        entry.last_update = timestamp;
                    }
                }
            }
        }
        else
        {
            std::iter_swap(i, swap_iter);
            swap_iter++;
        }
        i++;
    }
    _processor_change_queue.erase(swap_iter, _processor_change_queue.end());
}

} // namespace sushi