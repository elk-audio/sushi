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
 * @brief Sushi Async gRPC Call Data. Objects to handle the async calls to sushi.
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
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

using BlocklistKey = int64_t;

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

template <class ValueType, class BlocklistType>
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

    void proceed() override;

    void push(std::shared_ptr<ValueType> notification);

protected:
    // Spawns a new CallData instance to serve new clients while we process
    // the one for this CallData. The instance will deallocate itself as
    // part of its FINISH state, in proceed().
    virtual void _respawn() = 0;

    virtual void _subscribe() = 0;
    virtual void _unsubscribe() = 0;

    virtual bool _check_if_blocklisted(const ValueType& reply) = 0;
    virtual void _populate_blocklist() = 0;

    BlocklistType _notification_blocklist;
    grpc::ServerAsyncWriter<ValueType> _responder;

private:
    SynchronizedQueue<std::shared_ptr<ValueType>> _notifications;

    bool _first_iteration{true};
    bool _active{false};
};

class SubscribeToTransportChangesCallData : public SubscribeToUpdatesCallData<TransportUpdate, GenericVoidValue>
{
public:
    SubscribeToTransportChangesCallData(NotificationControlService* service,
                                        grpc::ServerCompletionQueue* async_rpc_queue)
            : SubscribeToUpdatesCallData(service, async_rpc_queue)
    {
        proceed();
    }

    ~SubscribeToTransportChangesCallData() = default;

protected:
    void _respawn() override;
    void _subscribe() override;
    void _unsubscribe() override;
    bool _check_if_blocklisted(const TransportUpdate& reply) override;
    void _populate_blocklist() override {}
};

class SubscribeToCpuTimingUpdatesCallData : public SubscribeToUpdatesCallData<CpuTimings, GenericVoidValue>
{
public:
    SubscribeToCpuTimingUpdatesCallData(NotificationControlService* service,
                                        grpc::ServerCompletionQueue* async_rpc_queue)
            : SubscribeToUpdatesCallData(service, async_rpc_queue)
    {
        proceed();
    }

    ~SubscribeToCpuTimingUpdatesCallData() = default;

protected:
    void _respawn() override;
    void _subscribe() override;
    void _unsubscribe() override;
    bool _check_if_blocklisted(const CpuTimings& reply) override;
    void _populate_blocklist() override {}
};

class SubscribeToTrackChangesCallData : public SubscribeToUpdatesCallData<TrackUpdate, GenericVoidValue>
{
public:
    SubscribeToTrackChangesCallData(NotificationControlService* service,
                                    grpc::ServerCompletionQueue* async_rpc_queue)
            : SubscribeToUpdatesCallData(service, async_rpc_queue)
    {
        proceed();
    }

    ~SubscribeToTrackChangesCallData() = default;

protected:
    void _respawn() override;
    void _subscribe() override;
    void _unsubscribe() override;
    bool _check_if_blocklisted(const TrackUpdate& reply) override;
    void _populate_blocklist() override {}
};

class SubscribeToProcessorChangesCallData : public SubscribeToUpdatesCallData<ProcessorUpdate, GenericVoidValue>
{
public:
    SubscribeToProcessorChangesCallData(NotificationControlService* service,
                                        grpc::ServerCompletionQueue* async_rpc_queue)
            : SubscribeToUpdatesCallData(service, async_rpc_queue)
    {
        proceed();
    }

    ~SubscribeToProcessorChangesCallData() = default;

protected:
    void _respawn() override;
    void _subscribe() override;
    void _unsubscribe() override;
    bool _check_if_blocklisted(const ProcessorUpdate& reply) override;
    void _populate_blocklist() override {}
};

class SubscribeToParameterUpdatesCallData : public SubscribeToUpdatesCallData<ParameterUpdate, ParameterNotificationBlocklist>
{
public:
    SubscribeToParameterUpdatesCallData(NotificationControlService* service,
                                        grpc::ServerCompletionQueue* async_rpc_queue)
            : SubscribeToUpdatesCallData(service, async_rpc_queue)
    {
        proceed();
    }

    ~SubscribeToParameterUpdatesCallData() = default;

protected:
    void _respawn() override;
    void _subscribe() override;
    void _unsubscribe() override;
    bool _check_if_blocklisted(const ParameterUpdate& reply) override;
    void _populate_blocklist() override;

private:
    std::unordered_map<BlocklistKey, bool> _blocklist;
};

class SubscribeToPropertyUpdatesCallData : public SubscribeToUpdatesCallData<PropertyValue, PropertyNotificationBlocklist>
{
public:
    SubscribeToPropertyUpdatesCallData(NotificationControlService* service,
                                       grpc::ServerCompletionQueue* async_rpc_queue)
            : SubscribeToUpdatesCallData(service, async_rpc_queue)
    {
        proceed();
    }

    ~SubscribeToPropertyUpdatesCallData() = default;

protected:
    void _respawn() override;
    void _subscribe() override;
    void _unsubscribe() override;
    bool _check_if_blocklisted(const PropertyValue& reply) override;
    void _populate_blocklist() override;

private:
    std::unordered_map<BlocklistKey, bool> _blocklist;
};

}
#endif // SUSHI_ASYNCSERVICECALLDATA_H
