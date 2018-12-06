#include "sushi_rpc/grpc_server.h"
#include "control_service.h"

namespace sushi_rpc {

GrpcServer::GrpcServer(const std::string& listenAddress,
                       sushi::ext::SushiControl*controller) : _listenAddress{listenAddress},
                                                              _service{new SushiControlService(_controller)},
                                                              _server_builder{new grpc::ServerBuilder()},
                                                              _controller{controller}
{

}

GrpcServer::~GrpcServer() = default;

void GrpcServer::start()
{
    _server_builder->AddListeningPort(_listenAddress, grpc::InsecureServerCredentials());
    _server_builder->RegisterService(_service.get());
    _server = _server_builder->BuildAndStart();
}

void GrpcServer::stop()
{
    // Maybe redundant
}

void GrpcServer::waitForCompletion()
{
    _server->Wait();
}

}// sushi_rpc