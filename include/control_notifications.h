
#ifndef SUSHI_CONTROL_NOTIFICATIONS_H
#define SUSHI_CONTROL_NOTIFICATIONS_H

#include "control_interface.h"

namespace sushi {
namespace ext {

class ProcessorNotification : public ControlNotification
{
public:
    ProcessorNotification(NotificationType type, int processor_id, Time timestamp)
            : ControlNotification(type, timestamp),
              _processor_id(processor_id) {}

    int processor_id() const {return _processor_id;}

private:
    int _processor_id;
};

class ParameterChangeNotification : public ControlNotification
{
public:
    ParameterChangeNotification(int processor_id, int parameter_id, float value, Time timestamp) 
    : ControlNotification(NotificationType::PARAMETER_CHANGE, timestamp),
      _processor_id(processor_id),
      _parameter_id(parameter_id),
      _value(value) {}

    int processor_id() const {return _processor_id;}
    int parameter_id() const {return _parameter_id;}
    float value() const {return _value;}

private:
    int _processor_id;
    int _parameter_id;
    float _value;
};

class TrackNotification : public ControlNotification
{
public:
    TrackNotification(NotificationType type, int track_id, Time timestamp)
            : ControlNotification(type, timestamp),
              _track_id(track_id) {}

    int track_id() const {return _track_id;}

private:
    int _track_id;
};

} // ext
} // sushi

#endif //SUSHI_CONTROL_NOTIFICATIONS_H