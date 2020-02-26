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

class CallDataBase
{
public:
    CallDataBase(SushiControlService* service, grpc::ServerCompletionQueue* cq)
        : _service(service),
          _cq(cq),
          _status(CREATE) {}

    virtual ~CallDataBase() = default;

    virtual void Proceed() = 0;

    void Stop()
    {
        _status = FINISH;
    }

protected:
    SushiControlService* _service;
    grpc::ServerCompletionQueue* _cq;
    grpc::ServerContext _ctx;

    enum CallStatus
    {
        CREATE,
        PROCESS,
        FINISH,
        PUSH_TO_BACK
    };
    CallStatus _status;
};

class SubscribeToParameterUpdatesCallData : public CallDataBase
{
public:
    SubscribeToParameterUpdatesCallData(SushiControlService* service, 
                                        grpc::ServerCompletionQueue* cq, 
                                        const NotificationContainer<ParameterSetRequest>* notifications)
        : CallDataBase(service, cq),
            _responder(&_ctx),
            _first_iteration(true),
            _last_notification_id(0),
            _notifications(notifications) 
    {
        Proceed();
    }

    virtual void Proceed() override;

private:
    std::string _create_map_key(int parameter_id, int processor_id) 
    { 
        return std::to_string(parameter_id) + "_" + std::to_string(processor_id);
    }

    ParameterNotificationRequest _request;
    ParameterSetRequest _reply;
    grpc::ServerAsyncWriter<ParameterSetRequest> _responder;

    grpc::Alarm _alarm;

    bool _first_iteration;
    unsigned int _last_notification_id;

    std::unordered_map<std::string, bool> _parameter_blacklist;
    const NotificationContainer<ParameterSetRequest>* _notifications;
};

}
#endif // SUSHI_ASYNCSERVICECALLDATA_H