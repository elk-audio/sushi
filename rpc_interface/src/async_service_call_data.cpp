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
 * @brief Sushi Async gRPC Call Data implementation. Objects to handle async calls to sushi.
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "async_service_call_data.h"

#include "control_service.h"

namespace sushi_rpc {

inline BlocklistKey create_key(int parameter_id, int processor_id)
{
    return (static_cast<BlocklistKey>(parameter_id) << 32) | processor_id;
}

void CallData::_alert()
{
    _in_completion_queue = true;
    _alarm.Set(_async_rpc_queue, gpr_now(gpr_clock_type::GPR_CLOCK_REALTIME), this);
}

void CallData::stop()
{
    _status = CallStatus::FINISH;
}

template<class ValueType, class BlocklistType>
void SubscribeToUpdatesCallData<ValueType, BlocklistType>::proceed()
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
            _populate_blocklist();
            _first_iteration = false;
        }

        if (_notifications.empty() == false)
        {
            auto reply = _notifications.pop();
            if (_check_if_blocklisted(*reply.get()) == false)
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

template<class ValueType, class BlocklistType>
void SubscribeToUpdatesCallData<ValueType, BlocklistType>::push(std::shared_ptr<ValueType> notification)
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
template class SubscribeToUpdatesCallData<TransportUpdate, GenericVoidValue>;
template class SubscribeToUpdatesCallData<CpuTimings, GenericVoidValue>;
template class SubscribeToUpdatesCallData<TrackUpdate, GenericVoidValue>;
template class SubscribeToUpdatesCallData<ProcessorUpdate, GenericVoidValue>;
template class SubscribeToUpdatesCallData<ParameterUpdate, ParameterNotificationBlocklist>;
template class SubscribeToUpdatesCallData<PropertyValue, PropertyNotificationBlocklist>;

void SubscribeToTransportChangesCallData::_respawn()
{
    new SubscribeToTransportChangesCallData(_service, _async_rpc_queue);
}

void SubscribeToTransportChangesCallData::_subscribe()
{
    _service->RequestSubscribeToTransportChanges(&_ctx,
                                                 &_notification_blocklist,
                                                 &_responder,
                                                 _async_rpc_queue,
                                                 _async_rpc_queue,
                                                 this);
    _service->subscribe(this);
}

void SubscribeToTransportChangesCallData::_unsubscribe()
{
    _service->unsubscribe(this);
}

bool SubscribeToTransportChangesCallData::_check_if_blocklisted(const TransportUpdate&)
{
    return false;
}

void SubscribeToCpuTimingUpdatesCallData::_respawn()
{
    new SubscribeToCpuTimingUpdatesCallData(_service, _async_rpc_queue);
}

void SubscribeToCpuTimingUpdatesCallData::_subscribe()
{
    _service->RequestSubscribeToEngineCpuTimingUpdates(&_ctx,
                                                       &_notification_blocklist,
                                                       &_responder,
                                                       _async_rpc_queue,
                                                       _async_rpc_queue,
                                                       this);
    _service->subscribe(this);
}

void SubscribeToCpuTimingUpdatesCallData::_unsubscribe()
{
    _service->unsubscribe(this);
}

bool SubscribeToCpuTimingUpdatesCallData::_check_if_blocklisted(const CpuTimings&)
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
                                             &_notification_blocklist,
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

bool SubscribeToTrackChangesCallData::_check_if_blocklisted(const TrackUpdate&)
{
    return false;
}

void SubscribeToProcessorChangesCallData::_respawn()
{
    new SubscribeToProcessorChangesCallData(_service, _async_rpc_queue);
}

void SubscribeToProcessorChangesCallData::_subscribe()
{
    _service->RequestSubscribeToProcessorChanges(&_ctx,
                                                 &_notification_blocklist,
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

bool SubscribeToProcessorChangesCallData::_check_if_blocklisted(const ProcessorUpdate&)
{
    return false;
}

void SubscribeToParameterUpdatesCallData::_respawn()
{
    new SubscribeToParameterUpdatesCallData(_service, _async_rpc_queue);
}

void SubscribeToParameterUpdatesCallData::_subscribe()
{
    _service->RequestSubscribeToParameterUpdates(&_ctx,
                                                 &_notification_blocklist,
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

bool SubscribeToParameterUpdatesCallData::_check_if_blocklisted(const ParameterUpdate& reply)
{
    auto key = create_key(reply.parameter().parameter_id(), reply.parameter().processor_id());
    return !(_blocklist.find(key) == _blocklist.end());
}

void SubscribeToParameterUpdatesCallData::_populate_blocklist()
{
    for (auto& identifier : _notification_blocklist.parameters())
    {
        _blocklist[create_key(identifier.parameter_id(), identifier.processor_id())] = false;
    }
}

void SubscribeToPropertyUpdatesCallData::_respawn()
{
    new SubscribeToPropertyUpdatesCallData(_service, _async_rpc_queue);
}

void SubscribeToPropertyUpdatesCallData::_subscribe()
{
    _service->RequestSubscribeToPropertyUpdates(&_ctx,
                                                &_notification_blocklist,
                                                &_responder,
                                                _async_rpc_queue,
                                                _async_rpc_queue,
                                                this);
    _service->subscribe(this);
}

void SubscribeToPropertyUpdatesCallData::_unsubscribe()
{
    _service->unsubscribe(this);
}

bool SubscribeToPropertyUpdatesCallData::_check_if_blocklisted(const PropertyValue& reply)
{
    auto key = create_key(reply.property().property_id(), reply.property().processor_id());
    return !(_blocklist.find(key) == _blocklist.end());
}

void SubscribeToPropertyUpdatesCallData::_populate_blocklist()
{
    for (auto& identifier : _notification_blocklist.properties())
    {
        _blocklist[create_key(identifier.property_id(), identifier.processor_id())] = false;
    }
}
} // namespace sushi_rpc
