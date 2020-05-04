#include "async_service_call_data.h"
#include "control_service.h"

namespace sushi_rpc {

void SubscribeToParameterUpdatesCallData::proceed()
{
    if (_status == CallStatus::CREATE)
    {
        _status = CallStatus::PROCESS;
        _service->RequestSubscribeToParameterUpdates(&_ctx, &_request, &_responder,
                                                     _async_rpc_queue, _async_rpc_queue, this);
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

            for (auto& parameter_identifier : _request.parameters())
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

} // namespace sushi_rpc
