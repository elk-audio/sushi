
#include "control_frontends/base_control_frontend.h"

namespace sushi {
namespace control_frontend {

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


} // end namespace control_frontend
} // end namespace sushi