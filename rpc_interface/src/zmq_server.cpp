/*
* Copyright 2017-2026 Elk Audio AB
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
 * @brief Zmq Server
 * @Copyright 2017-2026 Elk Audio AB, Stockholm
 */

#include <zmq.hpp>

#include "elklog/static_logger.h"

#include "sushi_rpc/zmq_server.h"
#include "zmq_control_service.h"

ELKLOG_GET_LOGGER_WITH_MODULE_NAME("zmq_server");

namespace sushi_ipc {
ZmqServer::ZmqServer(const std::string& address,
                     const std::string& sub_address,
                     sushi::control::SushiControl* controller) :_system_control_service{std::make_unique<SystemControlService>(controller)},
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
                                                                _zmq_context{std::make_unique<zmq::context_t>(1)},
                                                                _address{address},
                                                                _sub_address{sub_address},
                                                                _running{false}
{}

ZmqServer::~ZmqServer()
{
}

bool ZmqServer::start()
{
    try
    {
        _running = true;
        _notification_control_service->start(*_zmq_context.get(), _sub_address);
        _worker_tread = std::thread(&ZmqServer::_worker, this);
        return true;
    }
    catch (const std::runtime_error& e)
    {
        ELKLOG_LOG_ERROR("ZMQ Error: {}", e.what());
    }
    stop();
    return false;
}

void ZmqServer::stop()
{
    _notification_control_service->stop();
    _running = false;
    if (_worker_tread.joinable())
    {
        _worker_tread.join();
    }
}

struct Command
{
    Service service;
    int32_t endpoint;
};
static_assert(sizeof(Command) == sizeof(int32_t) * 2);

void ZmqServer::_worker()
{
    zmq::socket_t socket{*_zmq_context.get(), zmq::socket_type::rep};

    // 100 ms receive timeout so the loop can check _running
    socket.set(zmq::sockopt::rcvtimeo, 100);

    std::string response_frame;

    socket.bind(_address);

    while (_running)
    {
        zmq::message_t command_frame;
        zmq::message_t message_frame;

        /* An incoming sushi ipc request message has the following layout:
         * 4 bytes - Service code that maps to a controller service
         * 4 bytes - Command code that maps to function on a service
         * n bytes - Protobuf binary message deserialised by the receiving service object
         *
         * The response message has no Command prefix, as the receiving client knows what message to expect.
         */
        try
        {
            // Receive the service and command part
            if (!socket.recv(command_frame))
            {
                continue; // timeout
            }

            if (command_frame.size() != sizeof(Command))
            {
                ELKLOG_LOG_ERROR("Unexpected command frame size: {}", command_frame.size());
                continue;
            }
            // Receive the message part
            if (!socket.recv(message_frame))
            {
                ELKLOG_LOG_ERROR("Failed to receive message");
                continue;
            }
        }
        catch (const zmq::error_t& e)
        {
            ELKLOG_LOG_ERROR("ZMQ recv error: {}", e.what());
            // Avoid an infinite retry loop
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        Command*    cmd = reinterpret_cast<Command*>(command_frame.data());
        const auto* data = static_cast<const char*>(message_frame.data());
        const int   size = static_cast<int>(message_frame.size());
        ELKLOG_LOG_DEBUG("Received {} bytes of data with Service {} and command {}", size, cmd->service, cmd->endpoint);

        try
        {
            switch (cmd->service)
            {
                case Service::SYSTEM_CONTROLLER:
                    response_frame = _system_control_service->dispatch(cmd->endpoint, data, size);
                    break;

                case Service::TRANSPORT_CONTROLLER:
                    response_frame = _transport_control_service->dispatch(cmd->endpoint, data, size);
                    break;

                case Service::TIMING_CONTROLLER:
                    response_frame = _timing_control_service->dispatch(cmd->endpoint, data, size);
                    break;

                case Service::KEYBOARD_CONTROLLER:
                    response_frame = _keyboard_control_service->dispatch(cmd->endpoint, data, size);
                    break;

                case Service::AUDIO_GRAPH_CONTROLLER:
                    response_frame = _audio_graph_control_service->dispatch(cmd->endpoint, data, size);
                    break;

                case Service::PROGRAM_CONTROLLER:
                    response_frame = _program_control_service->dispatch(cmd->endpoint, data, size);
                    break;

                case Service::PARAMETER_CONTROLLER:
                    response_frame = _parameter_control_service->dispatch(cmd->endpoint, data, size);
                    break;

                case Service::MIDI_CONTROLLER:
                    response_frame = _midi_control_service->dispatch(cmd->endpoint, data, size);
                    break;

                case Service::AUDIO_ROUTING_CONTROLLER:
                    response_frame = _audio_routing_control_service->dispatch(cmd->endpoint, data, size);
                    break;

                case Service::CV_GATE_CONTROLLER:
                    response_frame = _cv_gate_control_service->dispatch(cmd->endpoint, data, size);
                    break;

                case Service::OSC_CONTROLLER:
                    response_frame = _osc_control_service->dispatch(cmd->endpoint, data, size);
                    break;

                case Service::SESSION_CONTROLLER:
                    response_frame = _session_control_service->dispatch(cmd->endpoint, data, size);
                    break;

                case Service::NOTIFICATION_CONTROLLER:
                    response_frame = _notification_control_service->dispatch(cmd->endpoint, data, size);
                    break;

                default:
                    ELKLOG_LOG_ERROR("Unknown service type: {}", cmd->service);
                    // Every request need to be met by a response to keep the zmq state machine happy, so send an empty response.
                    response_frame = "";
            }
            socket.send(zmq::const_buffer(response_frame.data(), response_frame.size()));
        }
        catch (const std::runtime_error& e)
        {
            ELKLOG_LOG_ERROR("Error receiving message: {}", e.what());
        }
    }
}

} // namespace sushi_ipc