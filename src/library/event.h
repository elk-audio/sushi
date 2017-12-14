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

constexpr size_t MIDI_DATA_SIZE = 4;

class Event;

enum class EventType
{
    BASIC_EVENT,
    KEYBOARD_EVENT,

    PARAMETER_CHANGE,
    STRING_PROPERTY_CHANGE,
    PARAMETER_CHANGE_NOTIFICATION,

    ADD_TRACK,
    REMOVE_TRACK,
    ADD_PROCESSOR,
    REMOVE_PROCESSOR,

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
    EVENT_SPECIFIC
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
    Event(EventType type,
          int64_t timestamp) : _type(type),
                               _timestamp(timestamp) {}

    virtual ~Event() {}

    EventType   type() {return _type;}
    int64_t     time() {return _timestamp;}
    int         receiver() {return _receiver;}
    EventId     id() {return _id;}
    /**
     * @brief Set a callback function that will be called after the event has been handled
     * @param callback A function pointer that will be called on completion
     * @param data Data that will be passed as function argument
     */
    void set_completion_cb(EventCompletionCallback callback, void* data)
    {
        _completion_cb = callback;
        _callback_arg = data;
    }
    //int         sender() {return _sender;}

    // TODO - put these under protected if possible
    EventCompletionCallback completion_cb() {return _completion_cb;}
    void*       callback_arg() {return _callback_arg;}
protected:
    /* Only the dispatcher can set the receiver */
    void set_receiver(int receiver) {_receiver = receiver;}

    EventType   _type;
    int         _receiver{0};
    int64_t     _timestamp;
    EventCompletionCallback _completion_cb{nullptr};
    void*       _callback_arg{nullptr};
    EventId     _id{EventIdGenerator::new_id()};

    //int         _sender;
};

/**
 * @brief Baseclass for all events going to or from an rt processor
 */
class ProcessorEvent : public Event
{
public:
    ProcessorEvent(EventType type,
                   int64_t timestamp,
                   const std::string& processor) : Event(type,  timestamp),
                                                   _processor(processor),
                                                   _access_by_id(false),
                                                   _processor_id(0) {}
    ProcessorEvent(EventType type,
                   int64_t timestamp,
                   ObjectId& processor_id) : Event(type,  timestamp),
                                             _processor(),
                                             _access_by_id{true},
                                             _processor_id(processor_id){}
    virtual ~ProcessorEvent() {}

    const std::string& processor() {return _processor;}
    ObjectId           processor_id() {return _processor_id;}
    /* If true, processor is accessed by integer id and not by name */
    bool access_by_id() {return _access_by_id;}

protected:
    std::string _processor;
    bool _access_by_id;
    ObjectId _processor_id;
};


class KeyboardEvent : public ProcessorEvent
{
public:
    enum class Subtype
    {
        NOTE_ON,
        NOTE_OFF,
        NOTE_AFTERTOUCH,
        AFTERTOUCH,
        PITCH_BEND,
        MODULATION,
        WRAPPED_MIDI
    };
    KeyboardEvent(Subtype subtype,
                  const std::string& processor,
                  float value,
                  int64_t timestamp) : ProcessorEvent(EventType::KEYBOARD_EVENT, timestamp, processor),
                                       _subtype(subtype),
                                       _note(0),
                                       _velocity(value) {}

    KeyboardEvent(Subtype subtype,
                  ObjectId processor_id,
                  float value,
                  int64_t timestamp) : ProcessorEvent(EventType::KEYBOARD_EVENT, timestamp, processor_id),
                                       _subtype(subtype),
                                       _note(0),
                                       _velocity(value) {}

    KeyboardEvent(Subtype subtype,
                  const std::string& processor,
                  int note,
                  float velocity,
                  int64_t timestamp) : ProcessorEvent(EventType::KEYBOARD_EVENT, timestamp, processor),
                                       _subtype(subtype),
                                       _note(note),
                                       _velocity(velocity) {}

    KeyboardEvent(Subtype subtype,
                  ObjectId processor_id,
                  int note,
                  float velocity,
                  int64_t timestamp) : ProcessorEvent(EventType::KEYBOARD_EVENT, timestamp, processor_id),
                                       _subtype(subtype),
                                       _note(note),
                                       _velocity(velocity) {}

    KeyboardEvent(Subtype subtype,
                  const std::string& processor,
                  MidiDataByte midi_data,
                  int64_t timestamp) : ProcessorEvent(EventType::KEYBOARD_EVENT, timestamp, processor),
                                       _subtype(subtype),
                                       _midi_data(midi_data) {}

    KeyboardEvent(Subtype subtype,
                  ObjectId processor_id,
                  MidiDataByte midi_data,
                  int64_t timestamp) : ProcessorEvent(EventType::KEYBOARD_EVENT, timestamp, processor_id),
                                       _subtype(subtype),
                                       _midi_data(midi_data) {}

    Subtype         subtype() {return _subtype;}
    int             note() {return _note;}
    float           velocity() {return _velocity;}
    float           value() {return _velocity;}
    MidiDataByte    midi_data() {return _midi_data;}

protected:
    Subtype         _subtype;
    int             _note;
    float           _velocity;
    MidiDataByte    _midi_data;
};

class ParameterChangeEvent : public ProcessorEvent
{
public:
    enum class Subtype
    {
        BOOL_PARAMETER_CHANGE,
        INT_PARAMETER_CHANGE,
        FLOAT_PARAMETER_CHANGE,
        STRING_PROPERTY_CHANGE,
        BLOB_PROPERTY_CHANGE
    };

    ParameterChangeEvent(Subtype subtype,
                         const std::string& processor,
                         const std::string& parameter,
                         float value,
                         int64_t timestamp) : ProcessorEvent(EventType::PARAMETER_CHANGE, timestamp, processor),
                                              _subtype(subtype),
                                              _parameter(parameter),
                                              _parameter_id(0),
                                              _value(value) {}

    ParameterChangeEvent(Subtype subtype,
                         ObjectId processor_id,
                         ObjectId parameter_id,
                         float value,
                         int64_t timestamp) : ProcessorEvent(EventType::PARAMETER_CHANGE, timestamp, processor_id),
                                              _subtype(subtype),
                                              _parameter(),
                                              _parameter_id(parameter_id),
                                              _value(value) {}

    Subtype             subtype() {return _subtype;}
    const std::string&  parameter() {return _parameter;}
    ObjectId            parameter_id() {return _parameter_id;}
    float               float_value() {return _value;}
    int                 int_value() {return static_cast<int>(_value);}
    bool                bool_value() {return _value > 0.5f;}

protected:
    Subtype             _subtype;
    std::string         _parameter;
    ObjectId            _parameter_id;
    float               _value;
};

class StringPropertyChangeEvent : public ParameterChangeEvent
{
public:
    StringPropertyChangeEvent(const std::string& processor,
                              const std::string& property,
                              const std::string& string_value,
                              int64_t timestamp) : ParameterChangeEvent(Subtype::STRING_PROPERTY_CHANGE,
                                                                        processor,
                                                                        property,
                                                                        0.0f,
                                                                        timestamp),
                                                   _string_value(string_value)
    {
        _type = EventType::STRING_PROPERTY_CHANGE;
    }

    StringPropertyChangeEvent(ObjectId processor_id,
                              ObjectId property_id,
                              const std::string& string_value,
                              int64_t timestamp) : ParameterChangeEvent(Subtype::STRING_PROPERTY_CHANGE,
                                                                        processor_id,
                                                                        property_id,
                                                                        0.0f,
                                                                        timestamp),
                                                   _string_value(string_value)
    {
        _type = EventType::STRING_PROPERTY_CHANGE;
    }

    const std::string& string_value() {return _string_value;}

protected:
    std::string _string_value;
};

/*class BlobPropertyChangeEvent : public ParameterChangeEvent
{
public:
    BlobData* string_value() {return _data;}

protected:
    BlobData* _data;
};*/


class ParameterChangeNotificationEvent : public ProcessorEvent
{
public:
    enum class Subtype
    {
        BOOL_PARAMETER_CHANGE_NOT,
        INT_PARAMETER_CHANGE_NOT,
        FLOAT_PARAMETER_CHANGE_NOT,
        STRING_PARAMETER_CHANGE_NOT,
        BLOB_PARAMETER_CHANGE_NOT
    };
    ParameterChangeNotificationEvent(Subtype subtype,
                                     const std::string& processor,
                                     const std::string& parameter,
                                     float value,
                                     int64_t timestamp) : ProcessorEvent(EventType::PARAMETER_CHANGE_NOTIFICATION, timestamp, processor),
                                                          _subtype(subtype),
                                                          _parameter(parameter),
                                                          _value(value) {}
    Subtype             subtype() {return _subtype;}
    const std::string&  parameter() {return _parameter;}
    float               float_value() {return _value;}
    int                 int_value() {return static_cast<int>(_value);}
    bool                bool_value() {return _value > 0.5f;}

protected:
    Subtype     _subtype;
    std::string _parameter;
    float       _value;
};

// TODO how to handle strings and blobs here?


class AddTrackEvent : public Event
{
public:
    enum Status : int
    {
        INVALID_NAME = EventStatus::EVENT_SPECIFIC
    };
    AddTrackEvent(const std::string& name, int channels, int64_t timestamp) : Event(EventType::ADD_TRACK, timestamp),
                                                                              _name(name),
                                                                              _channels(channels){}
    const std::string& name() {return _name;}
    int channels() {return _channels;}

private:
    std::string _name;
    int _channels;
};

class RemoveTrackEvent : public Event
{
public:
    enum Status : int
    {
        INVALID_TRACK = EventStatus::EVENT_SPECIFIC
    };
    RemoveTrackEvent(const std::string& name, int64_t timestamp) : Event(EventType::REMOVE_TRACK, timestamp),
                                                                   _name(name) {}
    const std::string& name() {return _name;}

private:
    std::string _name;
};

class AddProcessorEvent : public Event
{
public:
    enum Status : int
    {
        INVALID_NAME = EventStatus::EVENT_SPECIFIC,
        INVALID_CHAIN,
        INVALID_UID,
        INVALID_PLUGIN
    };
    enum class ProcessorType
    {
        INTERNAL,
        VST2X,
        VST3X
    };
    AddProcessorEvent(const std::string& track, const std::string& uid,
                      const std::string& name, const std::string& file,
                      ProcessorType plugin_type, int64_t timestamp) : Event(EventType::ADD_PROCESSOR, timestamp),
                                                                    _track(track),
                                                                    _uid(uid),
                                                                    _name(name),
                                                                    _file(file),
                                                                    _plugin_type(plugin_type) {}
    const std::string& track() {return _track;}
    const std::string& uid() {return _uid;}
    const std::string& name() {return _name;}
    const std::string& file() {return _file;}
    ProcessorType processor_type() {return _plugin_type;}

private:
    std::string _track;
    std::string _uid;
    std::string _name;
    std::string _file;
    ProcessorType _plugin_type;

};

class RemoveProcessorEvent : public Event
{
public:
    enum Status : int
    {
        INVALID_NAME = EventStatus::EVENT_SPECIFIC,
        INVALID_CHAIN,
    };
    RemoveProcessorEvent(const std::string& name, const std::string& track,
                         int64_t timestamp) : Event(EventType::REMOVE_PROCESSOR, timestamp),
                                              _name(name),
                                              _track(track) {}

    const std::string& name() {return _name;}
    const std::string& track() {return _track;}

private:
    std::string _name;
    std::string _track;
};

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
