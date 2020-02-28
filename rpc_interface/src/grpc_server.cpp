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
 * @brief gRPC Server
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "sushi_rpc/grpc_server.h"
#include "control_service.h"
#include "async_service_call_data.h"

namespace sushi_rpc {

GrpcServer::GrpcServer(const std::string& listenAddress,
                       sushi::ext::SushiControl* controller) : _listenAddress{listenAddress},
                                                               _service{new SushiControlService(controller)},
                                                               _server_builder{new grpc::ServerBuilder()},
                                                               _controller{controller},
                                                               _running{false}
{

}

GrpcServer::~GrpcServer() = default;

void GrpcServer::HandleRpcs()
{
    new SubscribeToParameterUpdatesCallData(_service.get(), _cq.get(), _service->parameter_notifications());

    void* tag;
    bool ok;
    while (_running.load())
    {
        // GPR_ASSERT(cq->Next(&tag, &ok));
        // GPR_ASSERT(ok);
        // assert(_cq->Next(&tag, &ok));
        _cq->Next(&tag, &ok);
        // assert(ok);
        if (ok != true)
        {
            static_cast<CallData*>(tag)->Stop();
            continue;
        }
        static_cast<CallData*>(tag)->proceed();
    }
}

void GrpcServer::start()
{
    _server_builder->AddListeningPort(_listenAddress, grpc::InsecureServerCredentials());
    _server_builder->RegisterService(_service.get());
    _cq = _server_builder->AddCompletionQueue();
    _server = _server_builder->BuildAndStart();
    _running.store(true);
    _worker = std::thread(&GrpcServer::HandleRpcs, this);
}

void GrpcServer::stop()
{
    _running.store(false);
    std::chrono::system_clock::time_point shutdown_deadline = std::chrono::system_clock::now() + std::chrono::milliseconds(50);
    _server->Shutdown(shutdown_deadline);
    _cq->Shutdown();
    void* tag;
    bool ok;
    while(_cq->Next(&tag, &ok));
    
    if (_worker.joinable())
    {
        _worker.join();
    }
}

void GrpcServer::waitForCompletion()
{
    _server->Wait();
}

}// sushi_rpc