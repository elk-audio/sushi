
#ifndef SUSHI_CONTROL_NOTIFICATIONS_H
#define SUSHI_CONTROL_NOTIFICATIONS_H

#include "control_interface.h"

namespace sushi {
namespace ext {

class ProcessorNotification : public ControlNotification
{
public:
    ProcessorNotification(ProcessorAction action, int processor_id, int parent_track_id, Time timestamp)
            : ControlNotification(NotificationType::PROCESSOR_UPDATE, timestamp),
              _processor_id(processor_id),
              _parent_track_id(parent_track_id),
              _action(action) {}

    int processor_id() const {return _processor_id;}
    int parent_track_id() const {return _parent_track_id;}
    ProcessorAction action() const {return _action;}

private:
    int _processor_id;
    int _parent_track_id;
    ProcessorAction _action;
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
    TrackNotification(TrackAction action, int track_id, Time timestamp)
            : ControlNotification(NotificationType::TRACK_UPDATE, timestamp),
              _track_id(track_id),
              _action(action) {}

    int track_id() const {return _track_id;}
    TrackAction action() const {return _action;}

private:
    int _track_id;
    TrackAction _action;
};

} // ext
} // sushi

#endif //SUSHI_CONTROL_NOTIFICATIONS_H