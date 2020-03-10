// /*
//  * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
//  *
//  * SUSHI is free software: you can redistribute it and/or modify it under the terms of
//  * the GNU Affero General Public License as published by the Free Software Foundation,
//  * either version 3 of the License, or (at your option) any later version.
//  *
//  * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
//  * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  * PURPOSE.  See the GNU Affero General Public License for more details.
//  *
//  * You should have received a copy of the GNU Affero General Public License along with
//  * SUSHI.  If not, see http://www.gnu.org/licenses/
//  */

// /**
//  * @brief Sushi Async gRPC Call Data. Objects to handle the async calls to sushi.
//  * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
//  */

// #ifndef SUSHI_ASYNCSERVICECALLDATA_H
// #define SUSHI_ASYNCSERVICECALLDATA_H

// #include <grpc++/alarm.h>

// #include "control_service.h"

// namespace sushi_rpc {

// class CallData
// {
// public:
//     CallData(SushiControlService* service,
//              grpc::ServerCompletionQueue* cq,
//              std::unordered_map<void*, bool>* event_handled_by_call_data)
//         : _service(service),
//           _cq(cq),
//           _status(CallStatus::CREATE),
//           _event_handled_by_call_data(event_handled_by_call_data) {}

//     virtual ~CallData() = default;

//     virtual void proceed(google::protobuf::Message* notification_message) = 0;

//     void stop()
//     {
//         _status = CallStatus::FINISH;
//     }

//     void push_to_back()
//     {
//         _status = CallStatus::PUSH_TO_BACK;
//     }

// protected:
//     SushiControlService* _service;
//     grpc::ServerCompletionQueue* _cq;
//     grpc::ServerContext _ctx;

//     enum class CallStatus
//     {
//         CREATE,
//         PROCESS,
//         PUSH_TO_BACK,
//         FINISH
//     };

//     CallStatus _status;
//     std::unordered_map<void*, bool>* _event_handled_by_call_data;
// };

// class SubscribeToParameterUpdatesCallData : public CallData
// {
// public:
//     SubscribeToParameterUpdatesCallData(SushiControlService* service,
//                                         grpc::ServerCompletionQueue* cq,
//                                         std::unordered_map<void*, bool>* event_handled_by_call_data)
//         : CallData(service, cq, event_handled_by_call_data),
//             _responder(&_ctx),
//             _first_iteration(true),
//             _last_notification_id(0)
//     {
//         proceed(nullptr);
//     }

//     ~SubscribeToParameterUpdatesCallData()
//     {
//         std::cout << "destroying " << this << std::endl;
//         _event_handled_by_call_data->erase(this);
//     }

//     virtual void proceed(google::protobuf::Message* notification_message) override;

// private:
//     int64_t _create_map_key(int parameter_id, int processor_id)
//     {
//         return ((int64_t)parameter_id << 32) | processor_id;
//     }

//     ParameterNotificationRequest _request;
//     ParameterSetRequest _reply;
//     grpc::ServerAsyncWriter<ParameterSetRequest> _responder;

//     // grpc::Alarm _alarm;

//     bool _first_iteration;
//     unsigned int _last_notification_id;

//     std::unordered_map<int64_t, bool> _parameter_blacklist;
//     const NotificationContainer<ParameterSetRequest>* _notifications;
// };

// }
// #endif // SUSHI_ASYNCSERVICECALLDATA_H
