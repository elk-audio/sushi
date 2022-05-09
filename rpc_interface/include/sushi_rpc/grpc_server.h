/*
 * Copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk
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
 * @copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_GRPCSERVER_H
#define SUSHI_GRPCSERVER_H

#include <memory>
#include <thread>
#include <atomic>

/* Forward declare grpc and service classes so their definitions can be
 * kept completely separate from the rest of the Sushi codebase. The macro
 * conditions ensure compability with ubunut 20's 1.16 grpc version and the
 * 1.24 grpc version on Elk OS. */

#if GOOGLE_PROTOBUF_VERSION > 3007001
namespace grpc_impl {
#else
namespace grpc {
#endif
    class Server;
    class ServerBuilder;
    class ServerCompletionQueue;
}


namespace sushi_rpc {

class SystemControlService;
class TransportControlService;
class TimingControlService;
class KeyboardControlService;
class AudioGraphControlService;
class ParameterControlService;
class ProgramControlService;
class MidiControlService;
class AudioRoutingControlService;
class OscControlService;
class SessionControlService;
class NotificationControlService;

constexpr std::chrono::duration SERVER_SHUTDOWN_DEADLINE = std::chrono::milliseconds(50);

class GrpcServer
{
public:
    GrpcServer(const std::string& listenAddress, sushi::ext::SushiControl* controller);

    ~GrpcServer();

    void start();

    void stop();

    void waitForCompletion();

    void AsyncRpcLoop();

private:
    std::string                                     _listen_address;

    std::unique_ptr<SystemControlService>           _system_control_service;
    std::unique_ptr<TransportControlService>        _transport_control_service;
    std::unique_ptr<TimingControlService>           _timing_control_service;
    std::unique_ptr<KeyboardControlService>         _keyboard_control_service;
    std::unique_ptr<AudioGraphControlService>       _audio_graph_control_service;
    std::unique_ptr<ParameterControlService>        _parameter_control_service;
    std::unique_ptr<ProgramControlService>          _program_control_service;
    std::unique_ptr<MidiControlService>             _midi_control_service;
    std::unique_ptr<AudioRoutingControlService>     _audio_routing_control_service;
    std::unique_ptr<OscControlService>              _osc_control_service;
    std::unique_ptr<SessionControlService>          _session_control_service;
    std::unique_ptr<NotificationControlService>     _notification_control_service;

    std::unique_ptr<grpc::ServerBuilder>            _server_builder;
    std::unique_ptr<grpc::Server>                   _server;
    std::unique_ptr<grpc::ServerCompletionQueue>    _async_rpc_queue;
    std::thread                                     _worker;
    std::atomic<bool>                               _running;
};

}// sushi_rpc

#endif //SUSHI_GRPCSERVER_H
