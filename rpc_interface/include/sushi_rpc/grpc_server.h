/**
 * @brief Grpc Server
 * @copyright MIND Music Labs AB, Stockholm
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

[[maybe_unused]] static const char* DEFAULT_LISTENING_ADDRESS = "localhost:51051";

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

    std::string _listenAddress;
    std::unique_ptr<SushiControlService> _service;
    std::unique_ptr<grpc::ServerBuilder> _server_builder;
    std::unique_ptr<grpc::Server>        _server;
    sushi::ext::SushiControl*            _controller;
};


}// sushi_rpc

#endif //SUSHI_GRPCSERVER_H
