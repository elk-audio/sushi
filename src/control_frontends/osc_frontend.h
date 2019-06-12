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

#include <vector>
#include <map>

#include "lo/lo.h"

#include "base_control_frontend.h"

namespace sushi {
namespace control_frontend {

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
    OSCFrontend(engine::BaseEngine* engine, int server_port= 24024, int send_port= 24023);

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
     * @brief Output changes from the given parameter of the given
     *        processor to osc messages. The output will be on the form:
     *        "/parameter/processor_name/parameter_name,f(value)"
     * @param processor_name
     * @param parameter_name
     * @return
     */
    bool connect_from_parameter(const std::string &processor_name,
                                const std::string &parameter_name);
    /**
     * @brief Connect keyboard messages to a given track.
     *        The target osc path will be:
     *        "/keyboard_event/track_name,sif(note_on/note_off, note_value, velocity)"
     * @param track_name The track to send to
     * @return true
     */
    bool connect_kb_to_track(const std::string &track_name);

    /**
     * @brief Register OSC callbacks far all parameters of all plugins and
     *        connect midi kb data to a track.
     *        This should eventually be replaced by a more elaborate way of
     *        registering parameters.
     */
    void connect_all();

    void run() override {_start_server();}

    void stop() override {_stop_server();}

    /* Inherited from EventPoster */
    int process(Event* event) override;

    int poster_id() override {return EventPosterId::OSC_FRONTEND;}

private:
    void _completion_callback(Event* event, int return_status) override;

    void _start_server();

    void _stop_server();

    void setup_engine_control();

    lo_server_thread _osc_server;
    int _server_port;
    int _send_port;
    lo_address _osc_out_address;

    std::atomic_bool _running;

    /* Currently only stored here so they can be deleted */
    std::vector<std::unique_ptr<OscConnection>> _connections;
    std::map<ObjectId, std::map<ObjectId, std::string>> _outgoing_connections;
};

std::string spaces_to_underscore(const std::string &s);

}; // namespace user_frontend
}; // namespace sensei

#endif //SUSHI_OSC_FRONTEND_H_H
