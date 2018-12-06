#include "grpc_server.h"
#include "control_service.h"

namespace sushi_rpc {

GrpcServer::GrpcServer(const std::string listenAddress, const std::vector<std::string>& notificationAddresses,
                       sushi::ext::SushiControl*controller) : _listenAddress{listenAddress},
                                                              _server_builder{new grpc::ServerBuilder()},
                                                              _controller{controller}
{

}

void GrpcServer::start()
{

}

void GrpcServer::stop()
{

}

void GrpcServer::waitForCompletion()
{
    _server->Wait();
}

}// sushi_rpc