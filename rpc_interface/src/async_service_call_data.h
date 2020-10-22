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
 * @brief Sushi Async gRPC Call Data. Objects to handle the async calls to sushi.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_ASYNCSERVICECALLDATA_H
#define SUSHI_ASYNCSERVICECALLDATA_H

#include <grpc++/alarm.h>
#include <grpcpp/grpcpp.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "sushi_rpc.grpc.pb.h"
#pragma GCC diagnostic pop

#include "library/synchronised_fifo.h"

#include "control_service.h"

namespace sushi_rpc {

class CallData
{
public:
    CallData(NotificationControlService* service,
             grpc::ServerCompletionQueue* async_rpc_queue) : _service(service),
                                                             _async_rpc_queue(async_rpc_queue),
                                                             _in_completion_queue(false),
                                                             _status(CallStatus::CREATE) {}

    virtual ~CallData() = default;

    virtual void proceed() = 0;

    /**
     * @brief Set the state of the call data to FINISH to make it destroy itself
     *        on the next call to proceed.
     */
    void stop();

protected:
    NotificationControlService* _service;
    grpc::ServerCompletionQueue* _async_rpc_queue;
    grpc::ServerContext _ctx;

    grpc::Alarm _alarm;

    bool _in_completion_queue;

    enum class CallStatus
    {
        CREATE,
        PROCESS,
        PUSH_TO_BACK,
        FINISH
    };

    CallStatus _status;

    /**
     * @brief Put the call data object at the back of the gRPC completion queue.
     *        This will result in an error if this object is already placed in
     *        the queue.
     */
    void _alert();
};

template <class VALUE, class NOTIFICATION_REQUEST>
class SubscribeToUpdatesCallData : public CallData
{
public:
    SubscribeToUpdatesCallData(NotificationControlService* service,
                               grpc::ServerCompletionQueue* async_rpc_queue)
            : CallData(service, async_rpc_queue),
              _responder(&_ctx)
    {
        // Classes inheriting from this, should call proceed() in their constructor.
    }

    void proceed() override
    {
        if (_status == CallStatus::CREATE)
        {
            _status = CallStatus::PROCESS;
            subscribe();
            _in_completion_queue = true;
        }
        else if (_status == CallStatus::PROCESS)
        {
            if (_first_iteration)
            {
                respawn();
                _active = true;
                populateBlacklist();
                _first_iteration = false;
            }

            if (_notifications.empty() == false)
            {
                auto reply = _notifications.pop();
                if (checkIfBlacklisted(*reply.get()) == false)
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
            unsubscribe();
            delete this;
        }
    }

    void push(std::shared_ptr<VALUE> notification)
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

protected:
    // Spawn a new CallData instance to serve new clients while we process
    // the one for this CallData. The instance will deallocate itself as
    // part of its FINISH state, in proceed().
    virtual void respawn() = 0;

    virtual void subscribe() = 0;
    virtual void unsubscribe() = 0;

    virtual bool checkIfBlacklisted(const VALUE& reply) = 0;
    virtual void populateBlacklist() = 0;

    NOTIFICATION_REQUEST _notification_request;
    grpc::ServerAsyncWriter<VALUE> _responder;

    std::unordered_map<int64_t, bool> _blacklist;

private:
    SynchronizedQueue<std::shared_ptr<VALUE>> _notifications;

    bool _first_iteration{true};
    bool _active{false};
};

class SubscribeToParameterUpdatesCallData : public SubscribeToUpdatesCallData<ParameterValue, ParameterNotificationRequest>
{
public:
    SubscribeToParameterUpdatesCallData(NotificationControlService* service,
                                        grpc::ServerCompletionQueue* async_rpc_queue)
            : SubscribeToUpdatesCallData(service, async_rpc_queue)
    {
        // TODO Ilias:
        // Proceed calls pure virtual functions, but if it is called from the constructor of the class
        // that defines them it should be ok no?
        proceed();
    }

    ~SubscribeToParameterUpdatesCallData() = default;

protected:
    void respawn() override
    {
        new SubscribeToParameterUpdatesCallData(_service, _async_rpc_queue);
    }

    void subscribe() override
    {
        _service->RequestSubscribeToParameterUpdates(&_ctx,
                                                     &_notification_request,
                                                     &_responder,
                                                     _async_rpc_queue,
                                                     _async_rpc_queue,
                                                     this);
        _service->subscribe_to_parameter_updates(this);
    }

    void unsubscribe() override
    {
        _service->unsubscribe_from_parameter_updates(this);
    }

    bool checkIfBlacklisted(const ParameterValue& reply) override
    {
        auto key =  _map_key(reply.parameter().parameter_id(),
                             reply.parameter().processor_id());

        return !(_blacklist.find(key) == _blacklist.end());
    }

    void populateBlacklist() override
    {
        for (auto& identifier : _notification_request.parameters())
        {
            _blacklist[_map_key(identifier.parameter_id(),
                                identifier.processor_id())] = false;
        }
    }

private:
    int64_t _map_key(int parameter_id, int processor_id)
    {
        return (static_cast<int64_t>(parameter_id) << 32) | processor_id;
    }
};

class SubscribeToProcessorChangesCallData : public SubscribeToUpdatesCallData<ProcessorUpdate, ProcessorNotificationRequest>
{
public:
    SubscribeToProcessorChangesCallData(NotificationControlService* service,
                                        grpc::ServerCompletionQueue* async_rpc_queue)
            : SubscribeToUpdatesCallData(service, async_rpc_queue)
    {
        // TODO Ilias:
        // Proceed calls pure virtual functions, but if it is called from the constructor of the class
        // that defines them it should be ok no?
        proceed();
    }

    ~SubscribeToProcessorChangesCallData() = default;

protected:
    void respawn() override
    {
        new SubscribeToProcessorChangesCallData(_service, _async_rpc_queue);
    }

    void subscribe() override
    {
        _service->RequestSubscribeToProcessorChanges(&_ctx,
                                                     &_notification_request,
                                                     &_responder,
                                                     _async_rpc_queue,
                                                     _async_rpc_queue,
                                                     this);
        _service->subscribe_to_processor_changes(this);
    }

    void unsubscribe() override
    {
        _service->unsubscribe_from_processor_changes(this);
    }

    bool checkIfBlacklisted(const ProcessorUpdate& reply) override
    {
        return false;
    }

    void populateBlacklist() override {}
};

}
#endif // SUSHI_ASYNCSERVICECALLDATA_H
