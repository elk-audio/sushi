#include "async_service_call_data.h"
#include "control_service.h"

namespace sushi_rpc {

void SubscribeToParameterUpdatesCallData::proceed()
{
    if (_status == CallStatus::CREATE)
    {
        _status = CallStatus::PROCESS;
        _service->RequestSubscribeToParameterUpdates(&_ctx, &_request, &_responder, _cq, _cq, this);
        _in_completion_queue = true;
    }
    else if (_status == CallStatus::PROCESS)
    {
        std::scoped_lock lock(_alert_lock);
        _in_completion_queue = false;
        if (_first_iteration)
        {
            // Spawn a new CallData instance to serve new clients while we process
            // the one for this CallData. The instance will deallocate itself as
            // part of its FINISH state.
            new SubscribeToParameterUpdatesCallData(_service, _cq);
            _service->subscribe_to_parameter_updates(this);
            _subscribed = true;

            for (auto& parameter_identifier : _request.parameters())
            {
                _parameter_blacklist[_create_map_key(parameter_identifier.parameter_id(),
                                                     parameter_identifier.processor_id())] = false;
            }
            _first_iteration = false;
        }

        else
        {
            if (_notifications.empty() == false)
            {
                _reply = *_notifications.pop();
                auto key =  _create_map_key(_reply.parameter().parameter_id(),
                                            _reply.parameter().processor_id());
                if (_parameter_blacklist.find(key) == _parameter_blacklist.end())
                {
                    _responder.Write(_reply, this);
                    _in_completion_queue = true;
                    _status = CallStatus::PUSH_TO_BACK;
                }
            }
        }
    }
    else if (_status == CallStatus::PUSH_TO_BACK)
    {
        // Since a call to write adds the object at the front of the completion
        // queue. We then place it at the back with an alarm to serve the clients
        // in a round robin fashion.
        alert();
    }
    else
    {
        assert(_status == CallStatus::FINISH);
        if (_subscribed)
        {
            _service->unsubscribe_from_parameter_updates(this);
        }

        // empty notification queue
        while(_notifications.empty() == false)
        {
            _notifications.pop();
        }
        std::cout << this << " deleted" << std::endl;
        delete this;
    }
}

} // namespace sushi_rpc
