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
    int64_t timestamp = 0;
    auto e = new ParameterChangeEvent(ParameterChangeEvent::Subtype::FLOAT_PARAMETER_CHANGE,
                                      processor, parameter, value, timestamp);
    _event_dispatcher->post_event(e);
}

void BaseControlFrontend::send_string_parameter_change_event(ObjectId processor,
                                                             ObjectId parameter,
                                                             const std::string& value)
{
    int64_t timestamp = 0;
    auto e = new StringPropertyChangeEvent(processor, parameter, value, timestamp);
    _event_dispatcher->post_event(e);}


void BaseControlFrontend::send_keyboard_event(ObjectId processor,
                                              KeyboardEvent::Subtype type,
                                              int note,
                                              float velocity)
{
    int64_t timestamp = 0;
    auto e = new KeyboardEvent(type, processor, note, velocity, timestamp);
    _event_dispatcher->post_event(e);
}

void BaseControlFrontend::send_note_on_event(ObjectId processor, int note, float velocity)
{
    send_keyboard_event(processor, KeyboardEvent::Subtype::NOTE_ON, note, velocity);
}

void BaseControlFrontend::send_note_off_event(ObjectId processor, int note, float velocity)
{
    send_keyboard_event(processor, KeyboardEvent::Subtype::NOTE_OFF, note, velocity);
}

void BaseControlFrontend::send_add_chain_event(const std::string &name, int channels)
{
    int64_t timestamp = 0;
    auto e = new AddChainEvent(name, channels, timestamp);
    send_with_callback(e);
}

void BaseControlFrontend::send_remove_chain_event(const std::string &name)
{
    int64_t timestamp = 0;
    auto e = new RemoveChainEvent(name, timestamp);
    send_with_callback(e);
}

void BaseControlFrontend::send_add_processor_event(const std::string &chain, const std::string &uid,
                                                   const std::string &name, const std::string &file,
                                                   AddProcessorEvent::ProcessorType type)
{
    int64_t timestamp = 0;
    auto e = new AddProcessorEvent(chain, uid, name, file, type, timestamp);
    send_with_callback(e);
}

void BaseControlFrontend::send_remove_processor_event(const std::string &chain, const std::string &name)
{
    int64_t timestamp = 0;
    auto e = new RemoveProcessorEvent(name, chain, timestamp);
    send_with_callback(e);
}

void BaseControlFrontend::send_with_callback(Event* event)
{
    event->set_completion_cb(BaseControlFrontend::completion_callback, this);
    //std::lock_guard<std::mutex> _lock(_sendlist_mutex);
    //_sendlist.push_back(event->id());
    _event_dispatcher->post_event(event);
}


} // end namespace control_frontend
} // end namespace sushi