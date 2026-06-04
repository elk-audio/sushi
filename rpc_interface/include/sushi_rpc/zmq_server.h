/*
 * Copyright 2017-2023 Elk Audio AB
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI. If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief gRPC Server
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef SUSHI_ZMQSERVER_H
#define SUSHI_ZMQSERVER_H

#include <memory>
#include <thread>
#include <atomic>

#include "sushi/control_interface.h"

namespace zmq {class context_t;}

namespace sushi_ipc {

constexpr auto ZMQ_IPC_ADDRESS = "ipc:///tmp/sushi_ipc.sock";

class SystemControlService;
class TransportControlService;
class TimingControlService;
class KeyboardControlService;
class AudioGraphControlService;
class ParameterControlService;
class ProgramControlService;
class MidiControlService;
class AudioRoutingControlService;
class CvGateControlService;
class OscControlService;
class SessionControlService;
class NotificationControlService;


class ZmqServer
{
public:
    ZmqServer(const std::string& socket, sushi::control::SushiControl* controller);

    ~ZmqServer();

    /**
     * Attempts to instantiate and start the zmq ipc server.
     * @return bool, reflecting the status of the resulting server.
     */
    [[nodiscard]] bool start();

    void stop();

private:
    std::unique_ptr<SystemControlService>           _system_control_service;
    std::unique_ptr<TransportControlService>        _transport_control_service;
    std::unique_ptr<TimingControlService>           _timing_control_service;
    std::unique_ptr<KeyboardControlService>         _keyboard_control_service;
    std::unique_ptr<AudioGraphControlService>       _audio_graph_control_service;
    std::unique_ptr<ParameterControlService>        _parameter_control_service;
    std::unique_ptr<ProgramControlService>          _program_control_service;
    std::unique_ptr<MidiControlService>             _midi_control_service;
    std::unique_ptr<AudioRoutingControlService>     _audio_routing_control_service;
    std::unique_ptr<CvGateControlService>           _cv_gate_control_service;
    std::unique_ptr<OscControlService>              _osc_control_service;
    std::unique_ptr<SessionControlService>          _session_control_service;
    std::unique_ptr<NotificationControlService>     _notification_control_service;

    std::unique_ptr<zmq::context_t>                 _zmq_context;
    std::string                                     _socket;
    std::thread                                     _worker_tread;
    std::atomic<bool>                               _running;
    void                                            _worker();
};


} // sushi_rpc

#endif //SUSHI_ZMQSERVER_H
