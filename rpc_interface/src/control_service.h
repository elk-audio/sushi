/**
 * @brief Sushi Control Service, gRPC service for external control of Sushi
 * @copyright MIND Music Labs AB, Stockholm
 */

#ifndef SUSHI_SUSHICONTROLSERVICE_H
#define SUSHI_SUSHICONTROLSERVICE_H

#include <grpc++/grpc++.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#include "sushi_rpc.grpc.pb.h"
#pragma GCC diagnostic pop


namespace sushi_rpc {

class SushiControlService : public sushi_rpc::SushiController::Service
{
public:
    SushiControlService();

};

}// sushi_rpc

#endif //SUSHI_SUSHICONTROLSERVICE_H
