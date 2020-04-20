/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI.  If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Main event class used for communication across modules outside the rt part
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_CONTROL_EVENT_H
#define SUSHI_CONTROL_EVENT_H

#include <string>

#include "types.h"
#include "id_generator.h"
#include "library/rt_event.h"
#include "library/time.h"
#include "library/types.h"

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
    ERROR,
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
    static Event* from_rt_event(RtEvent& rt_event, Time timestamp);

    Time        time() const {return _timestamp;}
    int         receiver() const {return _receiver;}
    EventId     id() const {return _id;}

    /**
     * @brief Whether the event should be processes asynchronously in a low priority thread or not
     * @return true if the Event should be processed asynchronously, false otherwise.
     */
    virtual bool process_asynchronously() {return false;}

    /* Convertible to KeyboardEvent */
    virtual bool is_keyboard_event() {return false;}

    /* Convertible to ParameterChangeEvent */
    virtual bool is_parameter_change_event() {return false;}

    /* Convertible to ParameterChangeNotification */
    virtual bool is_parameter_change_notification() {return false;}

    /* Convertible to EngineEvent */
    virtual bool is_engine_event() {return false;}

    /* Is either ClippingNotification or AudioGraphNotification */
    virtual bool is_engine_notification() {return false;}

    /* Convertible to ClippingNotification */
    virtual bool is_clipping_notification() {return false;}

    /* Convertible to AudioGraphNotification */
    virtual bool is_audio_graph_notification() {return false;}

    /* Convertible to AsynchronousWorkEvent */
    virtual bool is_async_work_event() {return false;}

    /* Event is directly convertible to an RtEvent */
    virtual bool maps_to_rt_event() {return false;}

    /* Return the RtEvent counterpart of the Event */
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
    explicit Event(Time timestamp) : _timestamp(timestamp) {}

    /* Only the dispatcher can set the receiver */
    void set_receiver(int receiver) {_receiver = receiver;}

    int         _receiver{0};
    Time        _timestamp;
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
                  int channel,
                  float value,
                  Time timestamp) : Event(timestamp),
                                    _subtype(subtype),
                                    _processor_id(processor_id),
                                    _channel(channel),
                                    _note(0),
                                    _velocity(value)
    {
        assert(_subtype == Subtype::AFTERTOUCH ||
               _subtype == Subtype::PITCH_BEND ||
               _subtype == Subtype::MODULATION);
    }

    KeyboardEvent(Subtype subtype,
                  ObjectId processor_id,
                  int channel,
                  int note,
                  float velocity,
                  Time timestamp) : Event(timestamp),
                                    _subtype(subtype),
                                    _processor_id(processor_id),
                                    _channel(channel),
                                    _note(note),
                                    _velocity(velocity) {}

    KeyboardEvent(Subtype subtype,
                  ObjectId processor_id,
                  MidiDataByte midi_data,
                  Time timestamp) : Event(timestamp),
                                    _subtype(subtype),
                                    _processor_id(processor_id),
                                    _midi_data(midi_data) {}

    bool is_keyboard_event() override  {return true;}

    bool maps_to_rt_event() override {return true;}

    RtEvent to_rt_event(int sample_offset) override;

    Subtype         subtype() {return _subtype;}
    ObjectId        processor_id() {return _processor_id;}
    int             channel() {return _channel;}
    int             note() {return _note;}
    float           velocity() {return _velocity;}
    float           value() {return _velocity;}
    MidiDataByte    midi_data() {return _midi_data;}

protected:
    Subtype         _subtype;
    ObjectId        _processor_id;
    int             _channel;
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
                         Time timestamp) : Event(timestamp),
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

private:
    Subtype             _subtype;

protected:
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
                              Time timestamp) : ParameterChangeEvent(Subtype::STRING_PROPERTY_CHANGE,
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
                            Time timestamp) : ParameterChangeEvent(Subtype::BLOB_PROPERTY_CHANGE,
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

// Inheriting from ParameterChangeEvent because they share the same data members but have
// different behaviour
class ParameterChangeNotificationEvent : public ParameterChangeEvent
{
public:
    enum class Subtype
    {
        BOOL_PARAMETER_CHANGE_NOT,
        INT_PARAMETER_CHANGE_NOT,
        FLOAT_PARAMETER_CHANGE_NOT
    };
    ParameterChangeNotificationEvent(Subtype subtype,
                                     ObjectId processor_id,
                                     ObjectId parameter_id,
                                     float value,
                                     Time timestamp) : ParameterChangeEvent(ParameterChangeEvent::Subtype::FLOAT_PARAMETER_CHANGE,
                                                                            processor_id,
                                                                            parameter_id,
                                                                            value,
                                                                            timestamp),
                                                       _subtype(subtype) {}

    bool is_parameter_change_notification() override {return true;}

    bool is_parameter_change_event() override {return false;}

    bool maps_to_rt_event() override {return false;}

    Subtype subtype() {return _subtype;}

private:
    Subtype _subtype;
};

class SetProcessorBypassEvent : public Event
{
public:
    SetProcessorBypassEvent(ObjectId processor_id, bool bypass_enabled, Time timestamp) : Event(timestamp),
                                                                                          _processor_id(processor_id),
                                                                                          _bypass_enabled(bypass_enabled)
    {}

    bool maps_to_rt_event() override {return true;}

    RtEvent to_rt_event(int sample_offset) override;

    ObjectId processor_id() {return _processor_id;}
    bool bypass_enabled() {return _bypass_enabled;}

private:
    ObjectId _processor_id;
    bool     _bypass_enabled;
};

// TODO how to handle strings and blobs here?

namespace engine {class BaseEngine;}

class EngineEvent : public Event
{
public:
    virtual bool process_asynchronously() override {return true;}

    virtual bool is_engine_event() override {return true;}

    virtual int execute(engine::BaseEngine* engine) = 0;

protected:
    explicit EngineEvent(Time timestamp) : Event(timestamp) {}
};

class AddTrackEvent : public EngineEvent
{
public:
    enum Status : int
    {
        INVALID_NAME = EventStatus::EVENT_SPECIFIC
    };

    AddTrackEvent(const std::string& name,
                  int channels,
                  std::optional<int> input,
                  int output,
                  Time timestamp) : EngineEvent(timestamp),
                                    _name(name),
                                    _channels(channels),
                                    _input_bus_channel(input),
                                    _output_bus_channel(output) {}

    int execute(engine::BaseEngine* engine) override;

private:
    std::string          _name;
    int                  _channels;
    std::optional<int>   _input_bus_channel;
    int                  _output_bus_channel;
};

class RemoveTrackEvent : public EngineEvent
{
public:
    enum Status : int
    {
        INVALID_TRACK = EventStatus::EVENT_SPECIFIC
    };
    RemoveTrackEvent(ObjectId track_id, Time timestamp) : EngineEvent(timestamp),
                                                          _track_id(track_id) {}

    int execute(engine::BaseEngine* engine) override;

private:
    ObjectId _track_id;
};

class AddProcessorToTrackEvent : public EngineEvent
{
public:
    enum Status : int
    {
        INVALID_NAME = EventStatus::EVENT_SPECIFIC,
        INVALID_TRACK,
        INVALID_UID
    };
    enum class ProcessorType
    {
        INTERNAL,
        VST2X,
        VST3X,
        LV2,
    };

    AddProcessorToTrackEvent(const std::string& name,
                             const std::string& uid,
                             const std::string& file,
                             ProcessorType processor_type,
                             ObjectId track,
                             std::optional<ObjectId> before_processor,
                             Time timestamp) : EngineEvent(timestamp) ,
                                               _name(name),
                                               _uid(uid),
                                               _file(file),
                                               _processor_type(processor_type),
                                               _track(track),
                                               _before_processor(before_processor) {}

    int execute(engine::BaseEngine* engine) override;

private:
    std::string     _name;
    std::string     _uid;
    std::string     _file;
    ProcessorType   _processor_type;
    ObjectId        _track;
    std::optional<ObjectId>  _before_processor;
};

class MoveProcessorEvent : public EngineEvent
{
public:
    enum Status : int
    {
        INVALID_NAME = EventStatus::EVENT_SPECIFIC,
        INVALID_SOURCE_TRACK,
        INVALID_DEST_TRACK
    };
    MoveProcessorEvent(ObjectId processor,
                       ObjectId source_track,
                       ObjectId dest_track,
                       std::optional<ObjectId> before_processor,
                       Time timestamp) : EngineEvent(timestamp),
                                         _processor(processor),
                                         _source_track(source_track),
                                         _dest_track(dest_track),
                                         _before_processor(before_processor) {}

    int execute(engine::BaseEngine* engine) override;

private:
    ObjectId _processor;
    ObjectId _source_track;
    ObjectId _dest_track;
    std::optional<ObjectId> _before_processor;
};

class RemoveProcessorEvent : public EngineEvent
{
public:
    enum Status : int
    {
        INVALID_NAME = EventStatus::EVENT_SPECIFIC,
        INVALID_TRACK,
    };
    RemoveProcessorEvent(ObjectId name, ObjectId track,
                         Time timestamp) : EngineEvent(timestamp),
                                           _name(name),
                                           _track(track) {}

    int execute(engine::BaseEngine* engine) override;

private:
    ObjectId _name;
    ObjectId _track;
};

class ProgramChangeEvent : public EngineEvent
{
public:

    ProgramChangeEvent(ObjectId processor_id,
                       int program_no,
                       Time timestamp) : EngineEvent(timestamp),
                                         _processor_id(processor_id),
                                         _program_no(program_no) {}

    int execute(engine::BaseEngine* engine) override;

    ObjectId            processor_id() {return _processor_id;}
    int                 program_no() {return _program_no;}

protected:
    ObjectId            _processor_id;
    int                 _program_no;
};

class ClippingNotificationEvent : public Event
{
public:
    enum class ClipChannelType
    {
        INPUT,
        OUTPUT,
    };
    ClippingNotificationEvent(int channel, ClipChannelType channel_type, Time timestamp) : Event(timestamp),
                                                                                           _channel(channel),
                                                                                           _channel_type(channel_type) {}
    bool            is_clipping_notification() override {return true;}
    bool            is_engine_notification() override {return true;}
    int             channel() {return _channel;}
    ClipChannelType channel_type() {return _channel_type;}

private:
    int _channel;
    ClipChannelType _channel_type;
};

class AudioGraphNotificationEvent : public Event
{
public:
    enum class Subtype
    {
        PROCESSOR_ADDED,
        PROCESSOR_DELETED,
        PROCESSOR_MOVED,
        TRACK_ADDED,
        TRACK_DELETED,
    };

    AudioGraphNotificationEvent(Subtype subtype,
                                ObjectId processor_id,
                                ObjectId track_id,
                                Time timestamp) : Event(timestamp),
                                                  _subtype(subtype),
                                                  _processor(processor_id),
                                                  _track(track_id) {}

    bool     is_audio_graph_notification() override {return true;}
    bool     is_engine_notification() override {return true;}
    Subtype  subtype() const {return _subtype;}
    ObjectId processor() const {return _processor;}
    ObjectId track() const {return _track;}

private:
    Subtype  _subtype;
    ObjectId _processor;
    ObjectId _track;
};

class AsynchronousWorkEvent : public Event
{
public:
    virtual bool process_asynchronously() override {return true;}
    virtual bool is_async_work_event() override {return true;}
    virtual Event* execute() = 0;

protected:
    explicit AsynchronousWorkEvent(Time timestamp) : Event(timestamp) {}
};

typedef int (*AsynchronousWorkCallback)(void* data, EventId id);

class AsynchronousProcessorWorkEvent : public AsynchronousWorkEvent
{
public:
    AsynchronousProcessorWorkEvent(AsynchronousWorkCallback callback,
                                   void* data,
                                   ObjectId processor,
                                   EventId rt_event_id,
                                   Time timestamp) : AsynchronousWorkEvent(timestamp),
                                                     _work_callback(callback),
                                                     _data(data),
                                                     _rt_processor(processor),
                                                      _rt_event_id(rt_event_id)
    {}

    virtual Event* execute() override;

protected:
    AsynchronousWorkCallback _work_callback;
    void*                    _data;
    ObjectId                 _rt_processor;
    EventId                  _rt_event_id;
};

class AsynchronousProcessorWorkCompletionEvent : public Event
{
public:
    AsynchronousProcessorWorkCompletionEvent(int return_value,
                                             ObjectId processor,
                                             EventId rt_event_id,
                                             Time timestamp) : Event(timestamp),
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

class AsynchronousBlobDeleteEvent : public AsynchronousWorkEvent
{
public:
    AsynchronousBlobDeleteEvent(BlobData data,
                                Time timestamp) : AsynchronousWorkEvent(timestamp),
                                                     _data(data) {}
    virtual Event* execute() override ;

private:
    BlobData _data;
};

class SetEngineTempoEvent : public EngineEvent
{
public:
    SetEngineTempoEvent(float tempo, Time timestamp) : EngineEvent(timestamp),
                                                       _tempo(tempo) {}

    int execute(engine::BaseEngine* engine) override;

private:
    float _tempo;
};

class SetEngineTimeSignatureEvent : public EngineEvent
{
public:
    SetEngineTimeSignatureEvent(TimeSignature signature, Time timestamp) : EngineEvent(timestamp),
                                                                           _signature(signature) {}

    int execute(engine::BaseEngine* engine) override;

private:
    TimeSignature _signature;
};

class SetEnginePlayingModeStateEvent : public EngineEvent
{
public:
    SetEnginePlayingModeStateEvent(PlayingMode mode, Time timestamp) : EngineEvent(timestamp),
                                                                       _mode(mode) {}

    int execute(engine::BaseEngine* engine) override;

private:
    PlayingMode _mode;
};

class SetEngineSyncModeEvent : public EngineEvent
{
public:
    SetEngineSyncModeEvent(SyncMode mode, Time timestamp) : EngineEvent(timestamp),
                                                            _mode(mode) {}

    int execute(engine::BaseEngine* engine) override;

private:
    SyncMode _mode;
};
} // end namespace sushi

#endif //SUSHI_CONTROL_EVENT_H
