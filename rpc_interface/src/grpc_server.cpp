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
#include <unordered_map>
#include <vector>
#include <string>

#include "control_service.h"
#include "sushi_rpc/grpc_server.h"
#include "async_service_call_data.h"

namespace sushi_rpc {

GrpcServer::GrpcServer(const std::string& listen_address,
                       sushi::ext::SushiControl* controller) : _listen_address{listen_address},
                                                               _system_control_service{std::make_unique<SystemControlService>(controller)},
                                                               _transport_control_service{std::make_unique<TransportControlService>(controller)},
                                                               _timing_control_service{std::make_unique<TimingControlService>(controller)},
                                                               _keyboard_control_service{std::make_unique<KeyboardControlService>(controller)},
                                                               _audio_graph_control_service{std::make_unique<AudioGraphControlService>(controller)},
                                                               _parameter_control_service{std::make_unique<ParameterControlService>(controller)},
                                                               _program_control_service{std::make_unique<ProgramControlService>(controller)},
                                                               _midi_control_service{std::make_unique<MidiControlService>(controller)},
                                                               _audio_routing_control_service{std::make_unique<AudioRoutingControlService>(controller)},
                                                               _osc_control_service{std::make_unique<OscControlService>(controller)},
                                                               _session_control_service{std::make_unique<SessionControlService>(controller)},
                                                               _notification_control_service{std::make_unique<NotificationControlService>(controller)},
                                                               _server_builder{std::make_unique<grpc::ServerBuilder>()},
                                                               _running{false}
{}

GrpcServer::~GrpcServer() = default;

void GrpcServer::AsyncRpcLoop()
{
    new SubscribeToTransportChangesCallData(_notification_control_service.get(), _async_rpc_queue.get());
    new SubscribeToCpuTimingUpdatesCallData(_notification_control_service.get(), _async_rpc_queue.get());
    new SubscribeToTrackChangesCallData(_notification_control_service.get(), _async_rpc_queue.get());
    new SubscribeToProcessorChangesCallData(_notification_control_service.get(), _async_rpc_queue.get());
    new SubscribeToParameterUpdatesCallData(_notification_control_service.get(), _async_rpc_queue.get());
    new SubscribeToPropertyUpdatesCallData(_notification_control_service.get(), _async_rpc_queue.get());

    while (_running.load())
    {
        void* tag;
        bool ok;

        _async_rpc_queue->Next(&tag, &ok);
        if (ok == false)
        {
            static_cast<CallData*>(tag)->stop();
        }
        static_cast<CallData*>(tag)->proceed();
    }
}

void GrpcServer::start()
{
    _server_builder->AddListeningPort(_listen_address, grpc::InsecureServerCredentials());

    _server_builder->RegisterService(_system_control_service.get());
    _server_builder->RegisterService(_transport_control_service.get());
    _server_builder->RegisterService(_timing_control_service.get());
    _server_builder->RegisterService(_keyboard_control_service.get());
    _server_builder->RegisterService(_audio_graph_control_service.get());
    _server_builder->RegisterService(_parameter_control_service.get());
    _server_builder->RegisterService(_program_control_service.get());
    _server_builder->RegisterService(_midi_control_service.get());
    _server_builder->RegisterService(_audio_routing_control_service.get());
    _server_builder->RegisterService(_osc_control_service.get());
    _server_builder->RegisterService(_session_control_service.get());
    _server_builder->RegisterService(_notification_control_service.get());

    _async_rpc_queue = _server_builder->AddCompletionQueue();
    _server = _server_builder->BuildAndStart();
    _running.store(true);
    _worker = std::thread(&GrpcServer::AsyncRpcLoop, this);
}

void GrpcServer::stop()
{
    auto now = std::chrono::system_clock::now();
    _running.store(false);
    _server->Shutdown(now + SERVER_SHUTDOWN_DEADLINE);
    _async_rpc_queue->Shutdown();
    if (_worker.joinable())
    {
        _worker.join();
    }

    void* tag;
    bool ok;

    // Empty completion queue
    while(_async_rpc_queue->Next(&tag, &ok));
    _notification_control_service->delete_all_subscribers();
}

void GrpcServer::waitForCompletion()
{
    _server->Wait();
}

} // sushi_rpc
