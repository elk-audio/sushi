/**
 * @brief Main event class used for communication across modules outside the rt part
 * @copyright MIND Music Labs AB, Stockholm
 */

#ifndef SUSHI_CONTROL_EVENT_H
#define SUSHI_CONTROL_EVENT_H

#include <string>
#include <cstdint>

#include "types.h"
#include "id_generator.h"
#include "library/rt_event.h"

namespace sushi {
namespace dispatcher {class EventDispatcher;};

class Event;

/* This is a weakly typed enum to allow for an opaque communication channel
 * between Receivers and avoid having to define all possible values inside
 * the event class header */
namespace EventStatus {
enum EventStatus : int
{
    HANDLED_OK,
    NOT_HANDLED,
    QUEUED_HANDLING,
    UNRECOGNIZED_RECEIVER,
    UNRECOGNIZED_EVENT,
    EVENT_SPECIFIC
};
}

typedef void (*EventCompletionCallback)(void *arg, Event* event, int status);
/**
 * @brief Event baseclass
 */
class Event
{
    friend class dispatcher::EventDispatcher;
public:

    virtual ~Event() {}

    /**
     * @brief Creates an Event from its RtEvent counterpart if possible
     * @param rt_event The RtEvent to convert from
     * @param timestamp the timestamp to assign to the Event
     * @return pointer to an Event if successful, nullptr if there is no possible conversion
     */
    static Event* from_rt_event(RtEvent& rt_event, int64_t timestamp);

    int64_t     time() {return _timestamp;}
    int         receiver() {return _receiver;}
    EventId     id() {return _id;}

    /**
     * @brief Whether the event should be processes asynchronously in a low priority thread or not
     * @return true if the Event should be processed asynchronously, false otherwise.
     */
    virtual bool process_asynchronously() {return false;}

    /* Convertible to KeyboardEvent */
    virtual bool is_keyboard_event() {return false;}

    /* Convertible to ParameterChangeEvent */
    virtual bool is_parameter_change_event() {return false;}

    /* Convertible to EngineEvent */
    virtual bool is_engine_event() {return false;}

    /* Convertible to AsynchronousWorkEvent */
    virtual bool is_async_work_event() {return false;}

    /* Event is directly convertible to an RtEvent */
    virtual bool maps_to_rt_event() {return false;}

    /** Return the RtEvent counterpart of the Event */
    virtual RtEvent to_rt_event(int /*sample_offset*/) {return RtEvent();}

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
    explicit Event(int64_t timestamp) : _timestamp(timestamp) {}

    /* Only the dispatcher can set the receiver */
    void set_receiver(int receiver) {_receiver = receiver;}

    int         _receiver{0};
    int64_t     _timestamp;
    EventCompletionCallback _completion_cb{nullptr};
    void*       _callback_arg{nullptr};
    EventId     _id{EventIdGenerator::new_id()};
};

class KeyboardEvent : public Event
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
                  ObjectId processor_id,
                  float value,
                  int64_t timestamp) : Event(timestamp),
                                       _subtype(subtype),
                                       _processor_id(processor_id),
                                       _note(0),
                                       _velocity(value) {}

    KeyboardEvent(Subtype subtype,
                  ObjectId processor_id,
                  int note,
                  float velocity,
                  int64_t timestamp) : Event(timestamp),
                                       _subtype(subtype),
                                       _processor_id(processor_id),
                                       _note(note),
                                       _velocity(velocity) {}

    KeyboardEvent(Subtype subtype,
                  ObjectId processor_id,
                  MidiDataByte midi_data,
                  int64_t timestamp) : Event(timestamp),
                                       _subtype(subtype),
                                       _processor_id(processor_id),
                                       _midi_data(midi_data) {}

    bool is_keyboard_event() override  {return true;}

    bool maps_to_rt_event() override {return true;}

    RtEvent to_rt_event(int sample_offset) override;

    Subtype         subtype() {return _subtype;}
    ObjectId        processor_id() {return _processor_id;}
    int             note() {return _note;}
    float           velocity() {return _velocity;}
    float           value() {return _velocity;}
    MidiDataByte    midi_data() {return _midi_data;}

protected:
    Subtype         _subtype;
    ObjectId        _processor_id;
    int             _note;
    float           _velocity;
    MidiDataByte    _midi_data;
};

class ParameterChangeEvent : public Event
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
                         ObjectId processor_id,
                         ObjectId parameter_id,
                         float value,
                         int64_t timestamp) : Event(timestamp),
                                              _subtype(subtype),
                                              _processor_id(processor_id),
                                              _parameter_id(parameter_id),
                                              _value(value) {}

    virtual bool is_parameter_change_event() override {return true;}

    virtual bool maps_to_rt_event() override {return true;}

    virtual RtEvent to_rt_event(int sample_offset) override;

    Subtype             subtype() {return _subtype;}
    ObjectId            processor_id() {return _processor_id;}
    ObjectId            parameter_id() {return _parameter_id;}
    float               float_value() {return _value;}
    int                 int_value() {return static_cast<int>(_value);}
    bool                bool_value() {return _value > 0.5f;}

protected:
    Subtype             _subtype;
    ObjectId            _processor_id;
    ObjectId            _parameter_id;
    float               _value;
};

class StringPropertyChangeEvent : public ParameterChangeEvent
{
public:
    StringPropertyChangeEvent(ObjectId processor_id,
                              ObjectId property_id,
                              const std::string& string_value,
                              int64_t timestamp) : ParameterChangeEvent(Subtype::STRING_PROPERTY_CHANGE,
                                                                        processor_id,
                                                                        property_id,
                                                                        0.0f,
                                                                        timestamp),
                                                   _string_value(string_value) {}

    RtEvent to_rt_event(int sample_offset) override;
    ObjectId property_id() {return _parameter_id;}

protected:
    std::string _string_value;
};

class DataPropertyChangeEvent : public ParameterChangeEvent
{
public:
    DataPropertyChangeEvent(ObjectId processor_id,
                            ObjectId property_id,
                            BlobData blob_value,
                            int64_t timestamp) : ParameterChangeEvent(Subtype::BLOB_PROPERTY_CHANGE,
                                                                      processor_id,
                                                                      property_id,
                                                                      0.0f,
                                                                      timestamp),
                                                 _blob_value(blob_value) {}


    RtEvent to_rt_event(int sample_offset) override;
    ObjectId property_id() {return _parameter_id;}
    BlobData blob_value() {return _blob_value;}

protected:
    BlobData _blob_value;
};

// TODO - Maybe notifications are just ParameterChangeEvents
/*class ParameterChangeNotificationEvent : public ProcessorEvent
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
};*/

// TODO how to handle strings and blobs here?

namespace engine {class BaseEngine;}

class EngineEvent : public Event
{
public:
    virtual bool process_asynchronously() override {return true;}

    virtual bool is_engine_event() override {return true;}

    virtual int execute(engine::BaseEngine*engine) = 0;

protected:
    explicit EngineEvent(int64_t timestamp) : Event(timestamp) {}
};

class AddTrackEvent : public EngineEvent
{
public:
    enum Status : int
    {
        INVALID_NAME = EventStatus::EVENT_SPECIFIC
    };
    AddTrackEvent(const std::string& name, int channels, int64_t timestamp) : EngineEvent(timestamp),
                                                                              _name(name),
                                                                              _channels(channels) {}
    int execute(engine::BaseEngine*engine) override;

private:
    std::string _name;
    int _channels;
};

class RemoveTrackEvent : public EngineEvent
{
public:
    enum Status : int
    {
        INVALID_TRACK = EventStatus::EVENT_SPECIFIC
    };
    RemoveTrackEvent(const std::string& name, int64_t timestamp) : EngineEvent(timestamp),
                                                                   _name(name) {}
    int execute(engine::BaseEngine*engine) override;

private:
    std::string _name;
};

class AddProcessorEvent : public EngineEvent
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
                      ProcessorType processor_type, int64_t timestamp) : EngineEvent(timestamp),
                                                                         _track(track),
                                                                         _uid(uid),
                                                                         _name(name),
                                                                         _file(file),
                                                                         _processor_type(processor_type) {}
    int execute(engine::BaseEngine*engine) override;

private:
    std::string     _track;
    std::string     _uid;
    std::string     _name;
    std::string     _file;
    ProcessorType   _processor_type;

};

class RemoveProcessorEvent : public EngineEvent
{
public:
    enum Status : int
    {
        INVALID_NAME = EventStatus::EVENT_SPECIFIC,
        INVALID_CHAIN,
    };
    RemoveProcessorEvent(const std::string& name, const std::string& track,
                         int64_t timestamp) : EngineEvent(timestamp),
                                              _name(name),
                                              _track(track) {}

    int execute(engine::BaseEngine*engine) override;

private:
    std::string _name;
    std::string _track;
};

typedef int (*AsynchronousWorkCallback)(void* data, EventId id);

class AsynchronousWorkEvent : public Event
{
public:
    AsynchronousWorkEvent(AsynchronousWorkCallback callback,
                          void* data,
                          ObjectId processor,
                          EventId rt_event_id,
                          int64_t timestamp) : Event(timestamp),
                                               _work_callback(callback),
                                               _data(data),
                                               _rt_processor(processor),
                                               _rt_event_id(rt_event_id)
    {}

    virtual bool process_asynchronously() {return true;}
    virtual bool is_async_work_event() {return true;}

    virtual Event* execute();

protected:
    AsynchronousWorkCallback _work_callback;
    void*                    _data;
    ObjectId                 _rt_processor;
    EventId                  _rt_event_id;
};

class AsynchronousWorkCompletionEvent : public Event
{
public:
    AsynchronousWorkCompletionEvent(int return_value,
                                           ObjectId processor,
                                           EventId rt_event_id,
                                           int64_t timestamp) : Event(timestamp),
                                                                _return_value(return_value),
                                                                _rt_processor(processor),
                                                                _rt_event_id(rt_event_id) {}

    bool maps_to_rt_event() override {return true;}
    RtEvent to_rt_event(int sample_offset) override;

private:
    int         _return_value;
    ObjectId    _rt_processor;
    EventId     _rt_event_id;
};


} // end namespace sushi

#endif //SUSHI_CONTROL_EVENT_H
