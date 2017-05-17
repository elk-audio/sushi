/**
 * @brief OSC runtime user frontend
 * @copyright MIND Music Labs AB, Stockholm
 */

#include "osc_frontend.h"
#include "logging.h"

#include <sstream>

namespace sushi {
namespace control_frontend {

MIND_GET_LOGGER;

namespace {

static void osc_error(int num, const char* msg, const char* path)
{
    if (msg && path ) // Sometimes liblo passes a nullpointer for msg
    {
        MIND_LOG_ERROR("liblo server error {} in path {}: {}", num, path, msg);
    }
}

static int osc_send_parameter_change_event(const char* /*path*/,
                                           const char* /*types*/,
                                           lo_arg** argv,
                                           int /*argc*/,
                                           void* /*data*/,
                                           void* user_data)
{
    auto connection = static_cast<OscConnection*>(user_data);
    float value = argv[0]->f;
    connection->instance->send_parameter_change_event(connection->processor, connection->parameter, value);
    MIND_LOG_DEBUG("Sending parameter {} on processor {} change to {}.", connection->parameter, connection->processor, value);
    return 0;
}

static int osc_send_keyboard_event(const char* /*path*/,
                                   const char* /*types*/,
                                   lo_arg** argv,
                                   int /*argc*/,
                                   void* /*data*/,
                                   void* user_data)
{
    auto connection = static_cast<OscConnection*>(user_data);
    std::string event(&argv[0]->s);
    int note = argv[1]->i;
    float value = argv[2]->f;

    EventType type;
    if (event == "note_on")
    {
        type = EventType::NOTE_ON;
    }
    else if (event == "note_off")
    {
        type = EventType::NOTE_OFF;
    }
    else
    {
        MIND_LOG_WARNING("Unrecognized event: {}.", event);
        return 0;
    }

    connection->instance->send_keyboard_event(connection->processor, type, note, value);
    MIND_LOG_DEBUG("Sending {} on processor {}.", event, connection->processor);
    return 0;
}

}; // anonymous namespace

OSCFrontend::OSCFrontend(EventFifo* queue, engine::BaseEngine* engine) : BaseControlFrontend(queue),
                                                                         _osc_server(nullptr),
                                                                         _server_port(DEFAULT_SERVER_PORT),
                                                                         _engine(engine)
{
    std::stringstream port_stream;
    port_stream << _server_port;

    _osc_server = lo_server_thread_new(port_stream.str().c_str(), osc_error);
}

OSCFrontend::~OSCFrontend()
{
    if (_running)
    {
        _stop_server();
    }
    lo_server_thread_free(_osc_server);
}

bool OSCFrontend::connect_to_parameter(const std::string &processor_name,
                                       const std::string &parameter_name)
{
    std::string osc_path = "/parameter/";
    engine::EngineReturnStatus status;
    ObjectId processor_id;
    std::tie(status, processor_id) = _engine->processor_id_from_name(processor_name);
    if (status != engine::EngineReturnStatus::OK)
    {
        return false;
    }
    ObjectId parameter_id;
    std::tie(status, parameter_id) = _engine->parameter_id_from_name(processor_name, parameter_name);
    if (status != engine::EngineReturnStatus::OK)
    {
        return false;
    }
    osc_path = osc_path + processor_name + "/" + parameter_name;
    OscConnection* connection = new OscConnection;
    connection->processor = processor_id;
    connection->parameter = parameter_id;
    connection->instance = this;
    _connections.push_back(std::unique_ptr<OscConnection>(connection));
    lo_server_thread_add_method(_osc_server, osc_path.c_str(), "f", osc_send_parameter_change_event, connection);
    return true;
}

bool OSCFrontend::connect_kb_to_track(const std::string &chain_name)
{
    std::string osc_path = "/keyboard_event/";
    engine::EngineReturnStatus status;
    ObjectId processor_id;
    std::tie(status, processor_id) = _engine->processor_id_from_name(chain_name);
    if (status != engine::EngineReturnStatus::OK)
    {
        return false;
    }
    osc_path = osc_path + chain_name;
    OscConnection* connection = new OscConnection;
    connection->processor = processor_id;
    connection->parameter = 0;
    connection->instance = this;
    _connections.push_back(std::unique_ptr<OscConnection>(connection));
    lo_server_thread_add_method(_osc_server, osc_path.c_str(), "sif", osc_send_keyboard_event, connection);
    return true;
}

void OSCFrontend::_start_server()
{
    _running.store(true);

    int ret = lo_server_thread_start(_osc_server);
    if (ret < 0)
    {
        MIND_LOG_ERROR("Error {} while starting OSC server thread", ret);
    }
}

void OSCFrontend::_stop_server()
{
    _running.store(false);
    int ret = lo_server_thread_stop(_osc_server);
    if (ret < 0)
    {
        MIND_LOG_ERROR("Error {} while stopping OSC server thread", ret);
    }
}

}
}