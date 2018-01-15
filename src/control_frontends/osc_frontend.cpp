/**
 * @brief OSC runtime user frontend
 * @copyright MIND Music Labs AB, Stockholm
 */

#include <algorithm>

#include "osc_frontend.h"
#include "logging.h"

#include <sstream>

namespace sushi {
namespace control_frontend {

MIND_GET_LOGGER;

namespace {

static void osc_error(int num, const char* msg, const char* path)
{
    if (msg && path) // Sometimes liblo passes a nullpointer for msg
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

static int osc_send_string_parameter_change_event(const char* /*path*/,
                                                  const char* /*types*/,
                                                  lo_arg** argv,
                                                  int /*argc*/,
                                                  void* /*data*/,
                                                  void* user_data)
{
    auto connection = static_cast<OscConnection*>(user_data);
    std::string value(&argv[0]->s);
    connection->instance->send_string_parameter_change_event(connection->processor, connection->parameter, value);
    MIND_LOG_DEBUG("Sending string parameter {} on processor {} change to {}.", connection->parameter, connection->processor, value);
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

    if (event == "note_on")
    {
        connection->instance->send_note_on_event(connection->processor, note, value);
    }
    else if (event == "note_off")
    {
        connection->instance->send_note_off_event(connection->processor, note, value);
    }
    else if (event == "program_change")
    {
        connection->instance->send_program_change_event(connection->processor, note);
    }
    else
    {
        MIND_LOG_WARNING("Unrecognized event: {}.", event);
        return 0;
    }
    MIND_LOG_DEBUG("Sending {} on processor {}.", event, connection->processor);
    return 0;
}

static int osc_add_track(const char* /*path*/,
                         const char* /*types*/,
                         lo_arg** argv,
                         int /*argc*/,
                         void* /*data*/,
                         void* user_data)
{
    auto instance = static_cast<OSCFrontend*>(user_data);
    std::string name(&argv[0]->s);
    int channels = argv[1]->i;
    MIND_LOG_DEBUG("Got an osc_add_track request {} {}", name, channels);
    instance->send_add_track_event(name, channels);
    return 0;
}

static int osc_delete_track(const char* /*path*/,
                            const char* /*types*/,
                            lo_arg** argv,
                            int /*argc*/,
                            void* /*data*/,
                            void* user_data)
{
    auto instance = static_cast<OSCFrontend*>(user_data);
    std::string name(&argv[0]->s);
    MIND_LOG_DEBUG("Got an osc_delete_track request {}", name);
    instance->send_remove_track_event(name);
    return 0;
}

static int osc_add_processor(const char* /*path*/,
                             const char* /*types*/,
                             lo_arg** argv,
                             int /*argc*/,
                             void* /*data*/,
                             void* user_data)
{
    auto instance = static_cast<OSCFrontend*>(user_data);
    std::string track(&argv[0]->s);
    std::string uid(&argv[1]->s);
    std::string name(&argv[2]->s);
    std::string file(&argv[3]->s);
    std::string type(&argv[4]->s);
    // TODO If these are eventually to be accessed by a user we must sanitize
    // the input and disallow supplying a direct library path for loading.
    MIND_LOG_DEBUG("Got an add_processor request {}", name);
    AddProcessorEvent::ProcessorType processor_type;
    if (type == "internal")
    {
        processor_type = AddProcessorEvent::ProcessorType::INTERNAL;
    }
    else if (type == "vst2x")
    {
        processor_type = AddProcessorEvent::ProcessorType::VST2X;
    }
    else if (type == "vst3x")
    {
        processor_type = AddProcessorEvent::ProcessorType::VST3X;
    }
    else
    {
        MIND_LOG_WARNING("Unrecognized plugin type \"{}\"", type);
        return 0;
    }
    instance->send_add_processor_event(track, uid, name, file, processor_type);
    return 0;
}

static int osc_delete_processor(const char* /*path*/,
                                const char* /*types*/,
                                lo_arg** argv,
                                int /*argc*/,
                                void* /*data*/,
                                void* user_data)
{
    auto instance = static_cast<OSCFrontend*>(user_data);
    std::string track(&argv[0]->s);
    std::string name(&argv[1]->s);
    MIND_LOG_DEBUG("Got a delete_processor request {} from {}", name, track);
    instance->send_remove_processor_event(track, name);
    return 0;
}

}; // anonymous namespace

OSCFrontend::OSCFrontend(engine::BaseEngine* engine) : BaseControlFrontend(engine, EventPosterId::OSC_FRONTEND),
                                                       _osc_server(nullptr),
                                                       _server_port(DEFAULT_SERVER_PORT),
                                                       _send_port(DEFAULT_SEND_PORT)
{
    std::stringstream port_stream;
    port_stream << _server_port;
    _osc_server = lo_server_thread_new(port_stream.str().c_str(), osc_error);

    std::stringstream send_port_stream;
    send_port_stream << _send_port;
    _osc_out_address = lo_address_new(nullptr, send_port_stream.str().c_str());
    setup_engine_control();
    _event_dispatcher->subscribe_to_parameter_change_notifications(this);
}

OSCFrontend::~OSCFrontend()
{
    if (_running)
    {
        _stop_server();
    }
    lo_server_thread_free(_osc_server);
    lo_address_free(_osc_out_address);
    _event_dispatcher->unsubscribe_from_parameter_change_notifications(this);
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
    osc_path = osc_path + spaces_to_underscore(processor_name) + "/" + spaces_to_underscore(parameter_name);
    OscConnection* connection = new OscConnection;
    connection->processor = processor_id;
    connection->parameter = parameter_id;
    connection->instance = this;
    _connections.push_back(std::unique_ptr<OscConnection>(connection));
    lo_server_thread_add_method(_osc_server, osc_path.c_str(), "f", osc_send_parameter_change_event, connection);
    MIND_LOG_INFO("Added osc callback {}", osc_path);
    return true;
}

bool OSCFrontend::connect_to_string_parameter(const std::string &processor_name,
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
    osc_path = osc_path + spaces_to_underscore(processor_name) + "/" + spaces_to_underscore(parameter_name);
    OscConnection* connection = new OscConnection;
    connection->processor = processor_id;
    connection->parameter = parameter_id;
    connection->instance = this;
    _connections.push_back(std::unique_ptr<OscConnection>(connection));
    lo_server_thread_add_method(_osc_server, osc_path.c_str(), "s", osc_send_string_parameter_change_event, connection);
    MIND_LOG_INFO("Added osc callback {}", osc_path);
    return true;
}

bool OSCFrontend::connect_from_parameter(const std::string& processor_name, const std::string& parameter_name)
{
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
    std::string id_string = "/parameter/" + spaces_to_underscore(processor_name) + "/" +
            spaces_to_underscore(parameter_name);
    _outgoing_connections[processor_id][parameter_id] = id_string;
    MIND_LOG_INFO("Added osc output from parameter {}/{}", processor_name, parameter_name);
    return true;
}

bool OSCFrontend::connect_kb_to_track(const std::string &track_name)
{
    std::string osc_path = "/keyboard_event/";
    engine::EngineReturnStatus status;
    ObjectId processor_id;
    std::tie(status, processor_id) = _engine->processor_id_from_name(track_name);
    if (status != engine::EngineReturnStatus::OK)
    {
        return false;
    }
    osc_path = osc_path + spaces_to_underscore(track_name);
    OscConnection* connection = new OscConnection;
    connection->processor = processor_id;
    connection->parameter = 0;
    connection->instance = this;
    _connections.push_back(std::unique_ptr<OscConnection>(connection));
    lo_server_thread_add_method(_osc_server, osc_path.c_str(), "sif", osc_send_keyboard_event, connection);
    MIND_LOG_INFO("Added osc callback {}", osc_path);
    return true;
}

void OSCFrontend::connect_all()
{
    auto& processors = _engine->all_processors();
    for (auto& processor : processors)
    {
        auto parameters = processor.second->all_parameters();
        for (auto& param : parameters)
        {
            if (param->type() == ParameterType::FLOAT)
            {
                connect_to_parameter(processor.second->name(), param->name());
                connect_from_parameter(processor.second->name(), param->name());
            }
            if (param->type() == ParameterType::STRING)
            {
                connect_to_string_parameter(processor.second->name(), param->name());
            }
        }
    }
    auto& tracks = _engine->all_tracks();
    for (auto& track : tracks)
    {
        connect_kb_to_track(track->name());
    }
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

void OSCFrontend::setup_engine_control()
{
    lo_server_thread_add_method(_osc_server, "/engine/add_track", "si", osc_add_track, this);
    lo_server_thread_add_method(_osc_server, "/engine/delete_track", "s", osc_delete_track, this);
    lo_server_thread_add_method(_osc_server, "/engine/add_processor", "sssss", osc_add_processor, this);
    lo_server_thread_add_method(_osc_server, "/engine/delete_processor", "ss", osc_delete_processor, this);
}

int OSCFrontend::process(Event* event)
{
    if (event->is_parameter_change_notification())
    {
        auto typed_event = static_cast<ParameterChangeNotificationEvent*>(event);
        const auto& node = _outgoing_connections.find(typed_event->processor_id());
        if (node != _outgoing_connections.end())
        {
            const auto& param_node = node->second.find(typed_event->parameter_id());
            if (param_node != node->second.end())
            {
                lo_send(_osc_out_address, param_node->second.c_str(), "f", typed_event->float_value());
            }
        }
        //MIND_LOG_INFO("Outputing parameter change {} {} {}", typed_event->processor_id(), typed_event->parameter_id(), typed_event->float_value());
        return EventStatus::HANDLED_OK;
    }
    return EventStatus::NOT_HANDLED;
}

void OSCFrontend::_completion_callback(Event* event, int return_status)
{
    MIND_LOG_INFO("EngineEvent {} completed with status {}({})", event->id(), return_status == 0? "ok" : "failure", return_status);
}

std::string spaces_to_underscore(const std::string &s)
{
    std::string us_s = s;
    std::replace(us_s.begin(), us_s.end(), ' ', '_');
    return us_s;
}


}
}