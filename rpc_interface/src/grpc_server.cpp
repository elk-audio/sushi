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

GrpcServer::GrpcServer(const std::string& listen_address,
                       sushi::ext::SushiControl* controller) : _listen_address{listen_address},
                                                _service{new SushiControlService(controller)},
                                                _server_builder{new grpc::ServerBuilder()},
                                                _controller{controller},
                                                _running{false}
{

}

GrpcServer::~GrpcServer() = default;

void GrpcServer::AsyncRpcLoop()
{
    new SubscribeToParameterUpdatesCallData(_service.get(), _async_rpc_queue.get());

    while (_running.load())
    {
        void* tag;
        bool ok;

        _async_rpc_queue->Next(&tag, &ok);
        if (ok != true)
        {
            static_cast<CallData*>(tag)->stop();
        }
        static_cast<CallData*>(tag)->proceed();
    }
}

void GrpcServer::start()
{
    _server_builder->AddListeningPort(_listen_address, grpc::InsecureServerCredentials());
    _server_builder->RegisterService(_service.get());
    _async_rpc_queue = _server_builder->AddCompletionQueue();
    _server = _server_builder->BuildAndStart();
    _running.store(true);
    _worker = std::thread(&GrpcServer::AsyncRpcLoop, this);
}

void GrpcServer::stop()
{
    auto now = std::chrono::high_resolution_clock::now();
    _running.store(false);
    _server->Shutdown(now + SERVER_SHUTDOWN_DEADLINE);
    _async_rpc_queue->Shutdown();

    if (_worker.joinable())
    {
        _worker.join();
    }

    _service->stop_all_call_data();

    void* tag;
    bool ok;

    //empty completion queue
    while(_async_rpc_queue->Next(&tag, &ok));
}

void GrpcServer::waitForCompletion()
{
    _server->Wait();
}

}// sushi_rpc
