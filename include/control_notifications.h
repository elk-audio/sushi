
#ifndef SUSHI_CONTROL_NOTIFICATIONS_H
#define SUSHI_CONTROL_NOTIFICATIONS_H

#include <variant>
#include "control_interface.h"

namespace sushi {
namespace ext {

using TransportNotificationValue = std::variant<float, PlayingMode, SyncMode, TimeSignature>;

class TransportNotification : public ControlNotification
{
public:
    TransportNotification(TransportAction action,
                          TransportNotificationValue value,
                          Time timestamp)
            : ControlNotification(NotificationType::TRANSPORT_UPDATE, timestamp),
              _value(value),
              _action(action) {}

    TransportAction action() const {return _action;}
    TransportNotificationValue value() const {return _value;}

private:
    TransportNotificationValue _value;
    TransportAction _action;
};

class CpuTimingNotification : public ControlNotification
{
public:
    CpuTimingNotification(CpuTimings timings, Time timestamp)
            : ControlNotification(NotificationType::CPU_TIMING_UPDATE, timestamp),
              _cpu_timings(timings) {}

    CpuTimings cpu_timings() const {return _cpu_timings;}

private:
    CpuTimings _cpu_timings;
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
    ParameterChangeNotification(int processor_id, int parameter_id, float normalized_value,
                                float domain_value, const std::string& formatted_value, Time timestamp)
    : ControlNotification(NotificationType::PARAMETER_CHANGE, timestamp),
      _processor_id(processor_id),
      _parameter_id(parameter_id),
      _normalized_value(normalized_value),
      _domain_value(domain_value),
      _formatted_value(formatted_value) {}

    int processor_id() const {return _processor_id;}
    int parameter_id() const {return _parameter_id;}
    float value() const {return _normalized_value;}
    float domain_value() const {return _domain_value;}
    const std::string& formatted_value() const {return _formatted_value;};

private:
    int _processor_id;
    int _parameter_id;
    float _normalized_value;
    float _domain_value;
    std::string _formatted_value;
};

class PropertyChangeNotification : public ControlNotification
{
public:
    PropertyChangeNotification(int processor_id, int property_id, const std::string& value, Time timestamp)
    : ControlNotification(NotificationType::PROPERTY_CHANGE, timestamp),
      _processor_id(processor_id),
      _property_id(property_id),
      _value(value) {}

    PropertyChangeNotification(int processor_id, int property_id, std::string&& value, Time timestamp)
    : ControlNotification(NotificationType::PROPERTY_CHANGE, timestamp),
      _processor_id(processor_id),
      _property_id(property_id),
      _value(value) {}

    int processor_id() const {return _processor_id;}
    int parameter_id() const {return _property_id;}
    const std::string& value() const {return _value;}

private:
    int _processor_id;
    int _property_id;
    std::string _value;
};

} // ext
} // sushi

#endif //SUSHI_CONTROL_NOTIFICATIONS_H