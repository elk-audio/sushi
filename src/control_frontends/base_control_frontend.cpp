
#include "control_frontends/base_control_frontend.h"

namespace sushi {
namespace control_frontend {

void BaseControlFrontend::send_parameter_change_event(const std::string& processor,
                                                      const std::string& parameter,
                                                      float value)
{
    // TODO - fix parameter lookup
    auto event = new ParameterChangeEvent(EventType::FLOAT_PARAMETER_CHANGE, 100, 0, 100, value);
    _queue->push(event);
}


void BaseControlFrontend::send_keyboard_event(const std::string& processor,
                                              EventType type,
                                              int note,
                                              float value)
{
    // TODO - fix process lookup
    auto event = new KeyboardEvent(type, 100, 0, note, value);
    _queue->push(event);
}


} // end namespace control_frontend
} // end namespace sushi