/**
 * @brief Main event class used for communiction across modules outside the rt part
 * @copyright MIND Music Labs AB, Stockholm
 */

#ifndef SUSHI_CONTROL_EVENT_H
#define SUSHI_CONTROL_EVENT_H

#include <string>
#include <cstdint>

#include "types.h"
#include "id_generator.h"

namespace sushi {

class Event;

enum class EventType
{
    KEYBOARD_EV,

    PARAMETER_CHANGE,
    PARAMETER_CHANGE_NOTIFICATION,

    ASYNCHRONOUS_WORK,
    ASYNC_WORK_COMPLETION_NOTIFICATION
};

/* This is a weakly typed enum to allow for an opaque communication channel
 * between Receivers and avoid having to define all possible values inside
 * the event class header */
namespace EventStatus {
enum EventStatus : int
{
    HANDLED_OK,
    NOT_HANDLED,
    UNRECOGNIZED_RECEIVER,
    UNRECOGNIZED_TYPE,
};
}

typedef void (*EventCompletionCallback)(void *arg, Event* event, int status);
/**
 * @brief Event baseclass
 */
class Event
{
    friend class EventDispatcher;
public:
    Event() {}
    virtual ~Event() {}

    EventType type() {return _type;}

    int64_t     time() {return _timestamp;}
    EventId     id() {return _id;}
    int         sender() {return _sender;}
    int         receiver() {return _receiver;}

    // TODO - put these under protected if possible
    EventCompletionCallback completion_cb() {return _completion_cb;}
    void*       callback_arg() {return _callback_arg;}
protected:


    EventCompletionCallback _completion_cb{nullptr};
    void*       _callback_arg{nullptr};
    int64_t     _timestamp;
    EventType   _type;
    EventId     _id{EventIdGenerator::new_id()};
    int         _sender;
    int         _receiver;
};

/**
 * @brief Baseclass for all events going to or from an rt processor
 */
class ProcessorEvent : public Event
{
public:
    virtual ~ProcessorEvent() {}

    const std::string& processor() {return _processor;}

protected:
    std::string _processor;
};


class KeyboardEvent : public ProcessorEvent
{
public:
    enum class Subtype
    {
        NOTE_ON,
        NOTE_OFF,
        NOTE_AFTERTOUCH,
        POLY_AFTERTOUCH,
        PITCH_BEND,
        RAW_MIDI
    };

    Subtype     subtype() {return _subtype;}
    int         note() {return _note;}
    float       velocity() {return _velocity;}
    uint8_t*    midi_data() {return _midi_data;}

protected:
    Subtype     _subtype;
    int         _note;
    float       _velocity;
    uint8_t     _midi_data[3];
};

class ParameterChangeEvent : public ProcessorEvent
{
public:
    enum class Subtype
    {
        BOOL_PARAMETER_CHANGE,
        INT_PARAMETER_CHANGE,
        FLOAT_PARAMETER_CHANGE,
        BLOB_PARAMETER_CHANGE
    };

    Subtype             subtype() {return _subtype;}
    const std::string&  parameter() {return _parameter;}
    float               float_value() {return _value;}
    int                 int_value() {return static_cast<int>(_value);}
    bool                bool_value() {return _value > 0.5f;}

protected:
    Subtype             _subtype;
    std::string         _parameter;
    float               _value;
};

class StringParameterChangeEvent : public ParameterChangeEvent
{
public:
    const std::string& string_value() {return _string_value;}

protected:
    std::string _string_value;
};

class BlobParameterChangeEvent : public ParameterChangeEvent
{
public:
    BlobData* string_value() {return _data;}

protected:
    BlobData* _data;
};


class ParameterChangeNotificationEvent : public ProcessorEvent
{
    enum class Subtype
    {
        BOOL_PARAMETER_CHANGE_NOT,
        INT_PARAMETER_CHANGE_NOT,
        FLOAT_PARAMETER_CHANGE_NOT,
        STRING_PARAMETER_CHANGE_NOT,
        BLOB_PARAMETER_CHANGE_NOT
    };

    Subtype     subtype() {return _subtype;}
    float       float_value() {return _value;}
    int         int_value() {return static_cast<int>(_value);}
    bool        bool_value() {return _value > 0.5f;}

protected:
    Subtype     _subtype;
    float       _value;
};

// TODO how to handle strings and blobs here?


typedef int (*AsynchronousWorkCallback)(void* data, EventId id);

class AsynchronousWorkEvent : public Event
{
public:

protected:
    AsynchronousWorkCallback _callback;
    ObjectId                 _rt_processor_id;
    EventId                  _rt_event_id;
};

class AsynchronousWorkCompletionNotification : public Event
{

protected:
    int         _return_value;
    ObjectId    _rt_processor_id;
    EventId     _rt_event_id;
};

} // end namespace sushi

#endif //SUSHI_CONTROL_EVENT_H
