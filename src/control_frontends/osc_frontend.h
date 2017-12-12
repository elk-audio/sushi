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

#include "lo/lo.h"

#include "base_control_frontend.h"
#include "engine/engine.h"

namespace sushi {
namespace control_frontend {

constexpr int DEFAULT_SERVER_PORT = 24024;

class OSCFrontend;
struct OscConnection
{
    ObjectId processor;
    ObjectId parameter;
    OSCFrontend* instance;
};

class OSCFrontend : public BaseControlFrontend
{
public:
    OSCFrontend(engine::BaseEngine* engine);

    ~OSCFrontend();

    /**
     * @brief Connect osc to a given parameter of a given processor.
     *        The resulting osc path will be:
     *        "/parameter/processor_name/parameter_name,f(value)"
     * @param processor_name Name of the processor
     * @param parameter_name Name of the parameter
     * @return
     */
    bool connect_to_parameter(const std::string &processor_name,
                              const std::string &parameter_name);

    bool connect_to_string_parameter(const std::string &processor_name,
                                     const std::string &parameter_name);
    /**
     * @brief Connect keyboard messages to a given plugin chain.
     *        The target osc path will be:
     *        "/keyboard_event/chain_name,sif(note_on/note_off, note_value, velocity)"
     * @param chain_name The track/plugin chain to send to
     * @return true
     */
    bool connect_kb_to_track(const std::string &chain_name);

    /**
     * @brief Register OSC callbacks far all parameters of all plugins and
     *        connect midi kb data to all chain.
     *        This should eventually be replaced by a more elaborate way of
     *        registering parameters.
     */
    void connect_all();

    virtual void run() override {_start_server();}

    virtual void stop() override {_stop_server();}

    /* Inherited from EventPoster */
    int process(Event* event);

    int poster_id() override {return EventPosterId::OSC_FRONTEND;}

private:
    void _completion_callback(Event* event, int return_status) override;

    void _start_server();

    void _stop_server();

    void setup_engine_control();

    lo_server_thread _osc_server;
    int _server_port;
    std::atomic_bool _running;

    /* Currently only stored here so they can be deleted */
    std::vector<std::unique_ptr<OscConnection>> _connections;
};

std::string spaces_to_underscore(const std::string &s);

}; // namespace user_frontend
}; // namespace sensei

#endif //SUSHI_OSC_FRONTEND_H_H
