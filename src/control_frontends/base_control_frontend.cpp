#include <thread>

#include "logging.h"

#include "control_frontends/base_control_frontend.h"

namespace sushi {
namespace control_frontend {

MIND_GET_LOGGER;

constexpr int STOP_RETRIES = 200;
constexpr auto RETRY_INTERVAL = std::chrono::milliseconds(2);

void BaseControlFrontend::send_parameter_change_event(ObjectId processor,
                                                      ObjectId parameter,
                                                      float value)
{
    _queue->push(Event::make_parameter_change_event(processor, 0, parameter, value));
}


void BaseControlFrontend::send_keyboard_event(ObjectId processor,
                                              EventType type,
                                              int note,
                                              float value)
{
    _queue->push(Event::make_keyboard_event(type, processor, 0, note, value));
}

void BaseControlFrontend::add_chain(const std::string &name, int channels)
{
    if (_stop_engine())
    {
        auto status = _engine->create_plugin_chain(name, channels);
        if (status != engine::EngineReturnStatus::OK)
        {
            MIND_LOG_ERROR("Failed to create chain {}", name);
        }
    }
    _engine->run();

}

void BaseControlFrontend::delete_chain(const std::string &name)
{
    if (_stop_engine())
    {
        auto status = _engine->delete_plugin_chain(name);
        if (status != engine::EngineReturnStatus::OK)
        {
            MIND_LOG_ERROR("Failed to delete chain {}", name);
        }
    }
    _engine->run();
}

void BaseControlFrontend::add_processor(const std::string& chain, const std::string& uid, const std::string& name,
                                        const std::string& file, engine::PluginType type)
{
    if (_stop_engine())
    {
        auto status = _engine->add_plugin_to_chain(chain, uid, name, file, type);
        if (status != engine::EngineReturnStatus::OK)
        {
            MIND_LOG_ERROR("Failed to add plugin {} ({}) to {}", name, uid, chain);
        }
    }
    _engine->run();
}

void BaseControlFrontend::delete_processor(const std::string& chain, const std::string& name)
{
    if (_stop_engine())
    {
        auto status = _engine->remove_plugin_from_chain(chain, name);
        if (status != engine::EngineReturnStatus::OK)
        {
            MIND_LOG_ERROR("Failed to delete processor {}", name);
        }
    }
    _engine->run();
}

bool BaseControlFrontend::_stop_engine()
{
    if (!_engine->running())
    {
        return true;
    }
    _queue->push(Event::make_stop_engine_event());
    int retries = 0;
    while (retries < STOP_RETRIES)
    {
        if (!_engine->running())
        {
            return true;
        }
        std::this_thread::sleep_for(RETRY_INTERVAL);
        retries++;
    }
    MIND_LOG_WARNING("Stopping engine timed out");
    return false;
}


} // end namespace control_frontend
} // end namespace sushi