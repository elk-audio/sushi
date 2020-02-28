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

#include <grpcpp/alarm.h>

#include "control_service.h"

namespace sushi_rpc {

class CallData
{
public:
    CallData(SushiControlService* service, grpc::ServerCompletionQueue* cq)
        : _service(service),
          _cq(cq),
          _status(CallStatus::CREATE) {}

    virtual ~CallData() = default;

    virtual void proceed() = 0;

    void Stop()
    {
        _status = CallStatus::FINISH;
    }

protected:
    SushiControlService* _service;
    grpc::ServerCompletionQueue* _cq;
    grpc::ServerContext _ctx;

    enum class CallStatus
    {
        CREATE,
        PROCESS,
        PUSH_TO_BACK,
        FINISH
    };

    CallStatus _status;
};

class SubscribeToParameterUpdatesCallData : public CallData
{
public:
    SubscribeToParameterUpdatesCallData(SushiControlService* service, 
                                        grpc::ServerCompletionQueue* cq, 
                                        const NotificationContainer<ParameterSetRequest>* notifications)
        : CallData(service, cq),
            _responder(&_ctx),
            _first_iteration(true),
            _last_notification_id(0),
            _notifications(notifications) 
    {
        proceed();
    }

    virtual void proceed() override;

private:
    int64_t _create_map_key(int parameter_id, int processor_id) 
    { 
        return ((int64_t)parameter_id << 32) | processor_id;
    }

    ParameterNotificationRequest _request;
    ParameterSetRequest _reply;
    grpc::ServerAsyncWriter<ParameterSetRequest> _responder;

    grpc::Alarm _alarm;

    bool _first_iteration;
    unsigned int _last_notification_id;

    std::unordered_map<int64_t, bool> _parameter_blacklist;
    const NotificationContainer<ParameterSetRequest>* _notifications;
};

}
#endif // SUSHI_ASYNCSERVICECALLDATA_H