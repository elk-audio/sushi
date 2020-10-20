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

void SubscribeToProcessorChangesCallData::proceed()
{
    if (_status == CallStatus::CREATE)
    {
        _status = CallStatus::PROCESS;
        _service->RequestSubscribeToProcessorChanges(&_ctx,
                                                     &_processor_notification_request,
                                                     &_responder,
                                                     _async_rpc_queue,
                                                     _async_rpc_queue,
                                                     this);
        _in_completion_queue = true;
        _service->subscribe_to_processor_changes(this);
    }
    else if (_status == CallStatus::PROCESS)
    {
        if (_first_iteration)
        {
            // Spawn a new CallData instance to serve new clients while we process
            // the one for this CallData. The instance will deallocate itself as
            // part of its FINISH state.
            new SubscribeToProcessorChangesCallData(_service, _async_rpc_queue);
            _active = true;

            _first_iteration = false;
        }

        if (_notifications.empty() == false)
        {
            auto _reply = _notifications.pop();

            _in_completion_queue = true;
            _responder.Write(*_reply.get(), this);
            _status = CallStatus::PUSH_TO_BACK;
            return;
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
        _service->unsubscribe_from_processor_changes(this);
        delete this;
    }
}

void SubscribeToProcessorChangesCallData::push(std::shared_ptr<ProcessorUpdate> notification)
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

void SubscribeToParameterUpdatesCallData::proceed()
{
    if (_status == CallStatus::CREATE)
    {
        _status = CallStatus::PROCESS;
        _service->RequestSubscribeToParameterUpdates(&_ctx,
                                                     &_parameter_notification_request,
                                                     &_responder,
                                                     _async_rpc_queue,
                                                     _async_rpc_queue,
                                                     this);
        _in_completion_queue = true;
        _service->subscribe_to_parameter_updates(this);
    }
    else if (_status == CallStatus::PROCESS)
    {
        if (_first_iteration)
        {
            // Spawn a new CallData instance to serve new clients while we process
            // the one for this CallData. The instance will deallocate itself as
            // part of its FINISH state.
            new SubscribeToParameterUpdatesCallData(_service, _async_rpc_queue);
            _active = true;

            for (auto& parameter_identifier : _parameter_notification_request.parameters())
            {
                _parameter_blacklist[_map_key(parameter_identifier.parameter_id(),
                                              parameter_identifier.processor_id())] = false;
            }
            _first_iteration = false;
        }

        if (_notifications.empty() == false)
        {
            auto _reply = _notifications.pop();
            auto key =  _map_key(_reply->parameter().parameter_id(),
                                 _reply->parameter().processor_id());
            if (_parameter_blacklist.find(key) == _parameter_blacklist.end())
            {
                _in_completion_queue = true;
                _responder.Write(*_reply.get(), this);
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
        _service->unsubscribe_from_parameter_updates(this);
        delete this;
    }
}

void SubscribeToParameterUpdatesCallData::push(std::shared_ptr<ParameterValue> notification)
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
} // namespace sushi_rpc
