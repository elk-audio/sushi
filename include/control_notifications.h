
#ifndef SUSHI_CONTROL_NOTIFICATIONS_H
#define SUSHI_CONTROL_NOTIFICATIONS_H

#include "control_interface.h"

namespace sushi {
namespace ext {

class ParameterChangeNotification : public ControlNotification
{
public:
    ParameterChangeNotification(int processor_id, int parameter_id, float value, Time timestamp) : ControlNotification(NotificationType::PARAMETER_CHANGE,
                                                                                                                       timestamp),
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
    const TrackInfo& info() {return _info;}

protected:
    TrackNotification(NotificationType type, TrackInfo info, Time timestamp) : ControlNotification(type, timestamp),
                                                                               _info(info) {}
private:
    TrackInfo _info;
};

class TrackAddedNotification : public TrackNotification
{
public:
    TrackAddedNotification(TrackInfo info, Time timestamp) : TrackNotification(NotificationType::TRACK_ADDED, info, timestamp) {}
};

class TrackChangedNotification : public TrackNotification
{
public:
    TrackChangedNotification(TrackInfo info, Time timestamp) : TrackNotification(NotificationType::TRACK_CHANGED, info, timestamp) {}
};



} // ext
} // sushi

#endif //SUSHI_CONTROL_NOTIFICATIONS_H