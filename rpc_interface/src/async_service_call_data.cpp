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

template<class VALUE, class NOTIFICATION_REQUEST>
void SubscribeToUpdatesCallData<VALUE, NOTIFICATION_REQUEST>::proceed()
{
    if (_status == CallStatus::CREATE)
    {
        _status = CallStatus::PROCESS;
        _subscribe();
        _in_completion_queue = true;
    }
    else if (_status == CallStatus::PROCESS)
    {
        if (_first_iteration)
        {
            _respawn();
            _active = true;
            _populateBlacklist();
            _first_iteration = false;
        }

        if (_notifications.empty() == false)
        {
            auto reply = _notifications.pop();
            if (_checkIfBlacklisted(*reply.get()) == false)
            {
                _in_completion_queue = true;
                _responder.Write(*reply.get(), this);
                _status = CallStatus::PUSH_TO_BACK;
                return;
            }
        }
        _in_completion_queue = false;
    }
    else if (_status == CallStatus::PUSH_TO_BACK)
    {
        // Since a call to write adds the object at the front of the completion
        // queue. We then place it at the back with an alarm to serve the clients
        // in a round robin fashion.
        _status = CallStatus::PROCESS;
        _alert();
    }
    else
    {
        assert(_status == CallStatus::FINISH);
        _unsubscribe();
        delete this;
    }
}

template<class VALUE, class NOTIFICATION_REQUEST>
void SubscribeToUpdatesCallData<VALUE, NOTIFICATION_REQUEST>::push(std::shared_ptr<VALUE> notification)
{
    if (_active)
    {
        _notifications.push(notification);
    }
    if (_in_completion_queue == false)
    {
        _alert();
    }
}

// Pre-Declaring what template instantiation is needed for SubscribeToUpdatesCallData,
// or it will not link.
template class SubscribeToUpdatesCallData<TrackUpdate, TrackNotificationRequest>;
template class SubscribeToUpdatesCallData<ProcessorUpdate, ProcessorNotificationRequest>;
template class SubscribeToUpdatesCallData<ParameterValue, ParameterNotificationRequest>;

void SubscribeToParameterUpdatesCallData::_respawn()
{
    new SubscribeToParameterUpdatesCallData(_service, _async_rpc_queue);
}

void SubscribeToParameterUpdatesCallData::_subscribe()
{
    _service->RequestSubscribeToParameterUpdates(&_ctx,
                                                 &_notification_request,
                                                 &_responder,
                                                 _async_rpc_queue,
                                                 _async_rpc_queue,
                                                 this);
    _service->subscribe(this);
}

void SubscribeToParameterUpdatesCallData::_unsubscribe()
{
    _service->unsubscribe(this);
}

bool SubscribeToParameterUpdatesCallData::_checkIfBlacklisted(const ParameterValue& reply)
{
    auto key =  _map_key(reply.parameter().parameter_id(),
                         reply.parameter().processor_id());

    return !(_blacklist.find(key) == _blacklist.end());
}

void SubscribeToParameterUpdatesCallData::_populateBlacklist()
{
    for (auto& identifier : _notification_request.parameters())
    {
        _blacklist[_map_key(identifier.parameter_id(),
                            identifier.processor_id())] = false;
    }
}

void SubscribeToProcessorChangesCallData::_respawn()
{
    new SubscribeToProcessorChangesCallData(_service, _async_rpc_queue);
}

void SubscribeToProcessorChangesCallData::_subscribe()
{
    _service->RequestSubscribeToProcessorChanges(&_ctx,
                                                 &_notification_request,
                                                 &_responder,
                                                 _async_rpc_queue,
                                                 _async_rpc_queue,
                                                 this);
    _service->subscribe(this);
}

void SubscribeToProcessorChangesCallData::_unsubscribe()
{
    _service->unsubscribe(this);
}

bool SubscribeToProcessorChangesCallData::_checkIfBlacklisted(const ProcessorUpdate& reply)
{
    return false;
}

void SubscribeToTrackChangesCallData::_respawn()
{
    new SubscribeToTrackChangesCallData(_service, _async_rpc_queue);
}

void SubscribeToTrackChangesCallData::_subscribe()
{
    _service->RequestSubscribeToTrackChanges(&_ctx,
                                             &_notification_request,
                                             &_responder,
                                             _async_rpc_queue,
                                             _async_rpc_queue,
                                             this);
    _service->subscribe(this);
}

void SubscribeToTrackChangesCallData::_unsubscribe()
{
    _service->unsubscribe(this);
}

bool SubscribeToTrackChangesCallData::_checkIfBlacklisted(const TrackUpdate& reply)
{
    return false;
}
} // namespace sushi_rpc
