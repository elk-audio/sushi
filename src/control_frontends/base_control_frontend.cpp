
#include "control_frontends/base_control_frontend.h"

namespace sushi {
namespace control_frontend {

void BaseControlFrontend::send_parameter_change_event(const std::string& processor,
                                                      const std::string& parameter,
                                                      float value)
{
    // TODO - fix parameter lookup
    _queue->push(Event::make_parameter_change_event(100, 0, 100, value));
}


void BaseControlFrontend::send_keyboard_event(const std::string& processor,
                                              EventType type,
                                              int note,
                                              float value)
{
    // TODO - fix process lookup
    _queue->push(Event::make_keyboard_event(type, 100, 0, note, value));
}


} // end namespace control_frontend
} // end namespace sushi