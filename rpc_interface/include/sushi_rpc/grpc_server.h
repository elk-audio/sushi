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

#ifndef SUSHI_GRPCSERVER_H
#define SUSHI_GRPCSERVER_H

#include <memory>
#include <string>
#include <vector>

#include "../../include/control_interface.h"

namespace grpc {
    class Server;
    class ServerBuilder;
}

namespace sushi_rpc {

class SushiControlService;

class GrpcServer
{
public:
    GrpcServer(const std::string& listenAddress, sushi::ext::SushiControl* controller);

    ~GrpcServer();

    void start();

    void stop();

    void waitForCompletion();

private:

    std::string                          _listenAddress;
    std::unique_ptr<SushiControlService> _service;
    std::unique_ptr<grpc::ServerBuilder> _server_builder;
    std::unique_ptr<grpc::Server>        _server;
    sushi::ext::SushiControl*            _controller;
};


}// sushi_rpc

#endif //SUSHI_GRPCSERVER_H
