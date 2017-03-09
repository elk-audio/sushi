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
    OSCFrontend* self = static_cast<OSCFrontend*>(user_data);
    std::string processor(&argv[0]->s);
    std::string parameter(&argv[1]->s);
    float value = argv[2]->f;
    self->send_parameter_change_event(processor, parameter, value);
    MIND_LOG_DEBUG("Sending parameter {} on processor {} change to {}.", parameter, processor, value);

    return 0;
}

static int osc_send_keyboard_event(const char* /*path*/,
                                   const char* /*types*/,
                                   lo_arg** argv,
                                   int /*argc*/,
                                   void* /*data*/,
                                   void* user_data)
{
    OSCFrontend* self = static_cast<OSCFrontend*>(user_data);
    std::string processor(&argv[0]->s);
    std::string event(&argv[1]->s);
    int note = argv[2]->i;
    float value = argv[3]->f;

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

    self->send_keyboard_event(processor, type, note, value);
    MIND_LOG_DEBUG("Sending {} on processor {}.", event, processor);
    return 0;
}

}; // anonymous namespace

void OSCFrontend::_start_server()
{
    std::stringstream port_stream;
    port_stream << _server_port;

    _osc_server = lo_server_thread_new(port_stream.str().c_str(), osc_error);
    lo_server_thread_add_method(_osc_server, "/parameter_change", "ssf", osc_send_parameter_change_event, this);
    lo_server_thread_add_method(_osc_server, "/keyboard_event", "ssif", osc_send_keyboard_event, this);

    int ret = lo_server_thread_start(_osc_server);
    if (ret < 0)
    {
        MIND_LOG_ERROR("Error {} while starting OSC server thread", ret);
    }
}

void OSCFrontend::_stop_server()
{
    int ret = lo_server_thread_stop(_osc_server);
    if (ret < 0)
    {
        MIND_LOG_ERROR("Error {} while stopping OSC server thread", ret);
    }
    lo_server_thread_free(_osc_server);
}

}
}