/**
 * @brief OSC runtime control frontend
 * @copyright MIND Music Labs AB, Stockholm
 *
 * Starts a thread listening for OSC commands at the given port
 * (configurable with proper command sent with apply_command.
 *
 * OSC paths and arguments:
 *
 *  /parameter_change processor_id parameter_id value
 *  /keyboard_event processor_id note_on/note_off note_value velocity
 *
 */
#ifndef SUSHI_OSC_FRONTEND_H_H
#define SUSHI_OSC_FRONTEND_H_H

#include "base_control_frontend.h"
#include "lo/lo.h"

namespace sushi {
namespace control_frontend {

constexpr int DEFAULT_SERVER_PORT = 24024;

class OSCFrontend : public BaseControlFrontend
{
public:
    OSCFrontend(EventFifo* queue) :
            BaseControlFrontend(queue),
            _osc_server(nullptr),
            _server_port(DEFAULT_SERVER_PORT) {}

    ~OSCFrontend()
    {
        if (_osc_server)
        {
            _stop_server();
        }
    }

    virtual void run() override {_start_server();}

private:
    void _start_server();

    void _stop_server();

    lo_server_thread _osc_server;
    int _server_port;
};

}; // namespace user_frontend
}; // namespace sensei

#endif //SUSHI_OSC_FRONTEND_H_H
