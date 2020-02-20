#ifndef SUSHI_ASYNCSERVICECALLDATA_H
#define SUSHI_ASYNCSERVICECALLDATA_H

#include <grpcpp/alarm.h>
// #include <grpc/support/log.h>

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
    SubscribeToParameterUpdatesCallData(SushiControlService* service, grpc::ServerCompletionQueue* cq, const NotificationContainer<ParameterSetRequest>* notifications)
        : CallDataBase(service, cq),
            _responder(&_ctx),
            _times(0),
            _last_notification_id(0),
            _notifications(notifications) 
    {
        Proceed();
    }

    virtual void Proceed() override;

private:
    GenericVoidValue _request;
    ParameterSetRequest _reply;
    grpc::ServerAsyncWriter<ParameterSetRequest> _responder;

    grpc::Alarm _alarm;

    int _times;
    unsigned int _last_notification_id;

    const NotificationContainer<ParameterSetRequest>* _notifications;
};

}
#endif // SUSHI_ASYNCSERVICECALLDATA_H