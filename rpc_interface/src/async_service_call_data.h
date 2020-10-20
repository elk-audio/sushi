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

namespace sushi_rpc {

class NotificationControlService;

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

// TODO Ilias: Reduce duplication between CallData subclasses when I've understood them better.
// Templatize on ProcessorUpdate / ParameterValue?
class SubscribeToProcessorChangesCallData : public CallData
{
public:
    SubscribeToProcessorChangesCallData(NotificationControlService* service,
                                        grpc::ServerCompletionQueue* async_rpc_queue)
            : CallData(service, async_rpc_queue),
              _responder(&_ctx),
              _first_iteration(true),
              _active(false)
    {
        proceed();
    }

    void proceed() override;

    void push(std::shared_ptr<ProcessorUpdate> notification);

private:
    ProcessorNotificationRequest _processor_notification_request;

    grpc::ServerAsyncWriter<ProcessorUpdate> _responder;

    bool _first_iteration;
    bool _active;

    SynchronizedQueue<std::shared_ptr<ProcessorUpdate>> _notifications;
};

class SubscribeToParameterUpdatesCallData : public CallData
{
public:
    SubscribeToParameterUpdatesCallData(NotificationControlService* service,
                                        grpc::ServerCompletionQueue* async_rpc_queue)
            : CallData(service, async_rpc_queue),
              _responder(&_ctx),
              _first_iteration(true),
              _active(false)
    {
        proceed();
    }

    void proceed() override;

    void push(std::shared_ptr<ParameterValue> notification);

private:
    int64_t _map_key(int parameter_id, int processor_id)
    {
        return (static_cast<int64_t>(parameter_id) << 32) | processor_id;
    }

    ParameterNotificationRequest _parameter_notification_request;
    grpc::ServerAsyncWriter<ParameterValue> _responder;

    bool _first_iteration;
    bool _active;

    std::unordered_map<int64_t, bool> _parameter_blacklist;
    SynchronizedQueue<std::shared_ptr<ParameterValue>> _notifications;
};

}
#endif // SUSHI_ASYNCSERVICECALLDATA_H
