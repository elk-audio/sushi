/*
 * Copyright 2017-2023 Elk Audio AB
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI. If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Main event class used for communication across modules outside the rt part
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef SUSHI_CONTROL_EVENT_H
#define SUSHI_CONTROL_EVENT_H

#include <string>
#include <memory>

#include "sushi/sushi_time.h"
#include "sushi/types.h"

#include "id_generator.h"
#include "library/rt_event.h"
#include "base_performance_timer.h"

namespace sushi::internal {

namespace engine {class BaseEngine;}

namespace dispatcher
{
    class EventDispatcher;
    class Worker;
}

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
    friend class sushi::internal::dispatcher::EventDispatcher;
    friend class sushi::internal::dispatcher::Worker;

public:
    virtual ~Event() = default;

    /**
     * @brief Creates an Event from its RtEvent counterpart if possible
     * @param rt_event The RtEvent to convert from
     * @param timestamp the timestamp to assign to the Event
     * @return pointer to an Event if successful, nullptr if there is no possible conversion
     */
    static std::unique_ptr<Event> from_rt_event(const RtEvent& rt_event, Time timestamp);

    [[nodiscard]] Time        time() const {return _timestamp;}
    [[nodiscard]] int         receiver() const {return _receiver;}
    [[nodiscard]] EventId     id() const {return _id;}

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

    /**
     * @brief Whether the events should be processes asynchronously in a low priority thread or not
     * @return true if the Event should be processed asynchronously, false otherwise.
     */
    [[nodiscard]] virtual bool process_asynchronously() const {return false;}

    /**
     * Events that are directly convertible to an RtEvent.
     */
    [[nodiscard]] virtual bool maps_to_rt_event() const {return false;}

    /**
     * Return the RtEvent counterpart of the Event
     */
    [[nodiscard]] virtual RtEvent to_rt_event(int /*sample_offset*/) const {return {};}

    /**
     * The below should all be overridden only by one single sub-class.
     * They are only used for reflection:
     *  to statically cast to a particular sub-class of Event.
     */

    /* Convertible to KeyboardEvent */
    [[nodiscard]] virtual bool is_keyboard_event() const {return false;}

    /* Convertible to ParameterChangeNotification */
    [[nodiscard]] virtual bool is_parameter_change_event() const {return false;}

    /* Convertible to ParameterChangeNotification */
    [[nodiscard]] virtual bool is_parameter_change_notification() const {return false;}

    /* Convertible to PropertyChangeNotification */
    [[nodiscard]] virtual bool is_property_change_notification() const {return false;}

    /* Convertible to EngineEvent */
    [[nodiscard]] virtual bool is_engine_event() const {return false;}

    /* Convertible to EngineNotificationEvent */
    [[nodiscard]] virtual bool is_engine_notification() const {return false;}

    /* Convertible to AsynchronousWorkEvent */
    [[nodiscard]] virtual bool is_async_work_event() const {return false;}

protected:
    explicit Event(Time timestamp) : _timestamp(timestamp) {}

    /* Only the dispatcher can set the receiver and call the completion callback */

    [[nodiscard]] EventCompletionCallback completion_cb() const {return _completion_cb;}
    [[nodiscard]] void*                   callback_arg() const {return _callback_arg;}

private:
    int                     _receiver{0};
    Time                    _timestamp;
    EventCompletionCallback _completion_cb{nullptr};
    void*                   _callback_arg{nullptr};
    EventId                 _id{EventIdGenerator::new_id()};
};

/**
 * The subsequent events all map to real-time events:
 */

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

    bool is_keyboard_event() const override {return true;}

    bool maps_to_rt_event() const override {return true;}

    [[nodiscard]] RtEvent to_rt_event(int sample_offset) const override;

    Subtype       subtype() const {return _subtype;}
    [[nodiscard]] ObjectId processor_id() const {return _processor_id;}
    [[nodiscard]] int      channel() const {return _channel;}
    [[nodiscard]] int      note() const {return _note;}
    [[nodiscard]] float    velocity() const {return _velocity;}
    [[nodiscard]] float    value() const {return _velocity;}
    MidiDataByte midi_data() {return _midi_data;}

private:
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
        FLOAT_PARAMETER_CHANGE
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

    [[nodiscard]] bool maps_to_rt_event() const override {return true;}

    [[nodiscard]] RtEvent to_rt_event(int sample_offset) const override;

    [[nodiscard]] bool is_parameter_change_event() const override {return true;}

    [[nodiscard]] Subtype             subtype() const {return _subtype;}
    [[nodiscard]] ObjectId            processor_id() const {return _processor_id;}
    [[nodiscard]] ObjectId            parameter_id() const {return _parameter_id;}
    [[nodiscard]] float               float_value() const {return _value;}
    [[nodiscard]] int                 int_value() const {return static_cast<int>(_value);}
    [[nodiscard]] bool                bool_value() const {return _value > 0.5f;}

private:
    Subtype             _subtype;

protected:
    ObjectId            _processor_id;
    ObjectId            _parameter_id;
    float               _value;
};

class DataPropertyEvent : public Event
{
public:
    DataPropertyEvent(ObjectId processor_id,
                      int property_id,
                      BlobData blob_value,
                      Time timestamp) : Event(timestamp),
                                        _processor_id(processor_id),
                                        _property_id(property_id),
                                        _blob_value(blob_value) {}

    [[nodiscard]] bool maps_to_rt_event() const override {return true;}

    [[nodiscard]] RtEvent to_rt_event(int sample_offset) const override;

private:
    ObjectId _processor_id;
    int      _property_id;
    BlobData _blob_value;
};

class StringPropertyEvent : public Event
{
public:
    StringPropertyEvent(ObjectId processor_id,
                       int property_id,
                       const std::string& string_value,
                       Time timestamp) : Event(timestamp),
                                        _processor_id(processor_id),
                                        _property_id(property_id),
                                        _string_value(string_value) {}

    [[nodiscard]] bool maps_to_rt_event() const override {return true;}

    [[nodiscard]] RtEvent to_rt_event(int sample_offset) const override;

private:
    ObjectId    _processor_id;
    ObjectId    _property_id;
    std::string _string_value;
};

class RtStateEvent : public Event
{
public:
    RtStateEvent(ObjectId processor_id,
                 std::unique_ptr<RtState> state,
                 Time timestamp);

    ~RtStateEvent() override;

    bool maps_to_rt_event() const override {return true;}

    RtEvent to_rt_event(int sample_offset) const override;

private:
    ObjectId _processor_id;
    mutable std::unique_ptr<RtState> _state;
};

class SetProcessorBypassEvent : public Event
{
public:
    SetProcessorBypassEvent(ObjectId processor_id, bool bypass_enabled, Time timestamp) : Event(timestamp),
                                                                                          _processor_id(processor_id),
                                                                                          _bypass_enabled(bypass_enabled)
    {}

    [[nodiscard]] bool maps_to_rt_event() const override {return true;}

    [[nodiscard]] RtEvent to_rt_event(int sample_offset) const override;

    [[nodiscard]] ObjectId processor_id() const {return _processor_id;}
    [[nodiscard]] bool bypass_enabled() const {return _bypass_enabled;}

private:
    ObjectId _processor_id;
    bool     _bypass_enabled;
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

    [[nodiscard]] bool maps_to_rt_event() const override {return true;}
    [[nodiscard]] RtEvent to_rt_event(int sample_offset) const override;

private:
    int         _return_value;
    ObjectId    _rt_processor;
    EventId     _rt_event_id;
};

/**
 * The following events are all for Asynchronous processing:
 */

class EngineEvent : public Event
{
public:
    [[nodiscard]] bool process_asynchronously() const override {return true;}

    [[nodiscard]] bool is_engine_event() const override {return true;}

    [[nodiscard]] virtual int execute(engine::BaseEngine* engine) const = 0;

protected:
    explicit EngineEvent(Time timestamp) : Event(timestamp) {}
};

template <typename LambdaType>
class LambdaEvent : public EngineEvent
{
public:
    LambdaEvent(const LambdaType& work_lambda,
                Time timestamp) : EngineEvent(timestamp),
                                  _work_lambda(work_lambda) {}

    LambdaEvent(LambdaType&& work_lambda,
                Time timestamp) : EngineEvent(timestamp),
                                  _work_lambda(std::move(work_lambda)) {}

    int execute(engine::BaseEngine*) const override
    {
        return _work_lambda();
    }

protected:
    LambdaType _work_lambda;
};

class ProgramChangeEvent : public EngineEvent
{
public:
    ProgramChangeEvent(ObjectId processor_id,
                       int program_no,
                       Time timestamp) : EngineEvent(timestamp),
                                         _processor_id(processor_id),
                                         _program_no(program_no) {}

    int execute(engine::BaseEngine* engine) const override;

    [[nodiscard]] ObjectId processor_id() const {return _processor_id;}
    [[nodiscard]] int      program_no() const {return _program_no;}

private:
    ObjectId            _processor_id;
    int                 _program_no;
};

class PropertyChangeEvent : public EngineEvent
{
public:
    PropertyChangeEvent(ObjectId processor_id,
                        ObjectId property_id,
                        std::string  string_value,
                        Time timestamp) : EngineEvent(timestamp),
                                          _processor_id(processor_id),
                                          _property_id(property_id),
                                          _string_value(std::move(string_value)) {}

    int execute(engine::BaseEngine* engine) const override;

private:
    ObjectId    _processor_id;
    ObjectId    _property_id;
    std::string _string_value;
};

class SetEngineTempoEvent : public EngineEvent
{
public:
    SetEngineTempoEvent(float tempo, Time timestamp) : EngineEvent(timestamp),
                                                       _tempo(tempo) {}

    int execute(engine::BaseEngine* engine) const override;

private:
    float _tempo;
};

class SetEngineTimeSignatureEvent : public EngineEvent
{
public:
    SetEngineTimeSignatureEvent(TimeSignature signature, Time timestamp) : EngineEvent(timestamp),
                                                                           _signature(signature) {}

    int execute(engine::BaseEngine* engine) const override;

private:
    TimeSignature _signature;
};

class SetEnginePlayingModeStateEvent : public EngineEvent
{
public:
    SetEnginePlayingModeStateEvent(PlayingMode mode, Time timestamp) : EngineEvent(timestamp),
                                                                       _mode(mode) {}

    int execute(engine::BaseEngine* engine) const override;

private:
    PlayingMode _mode;
};

class SetEngineSyncModeEvent : public EngineEvent
{
public:
    SetEngineSyncModeEvent(SyncMode mode, Time timestamp) : EngineEvent(timestamp),
                                                            _mode(mode) {}

    int execute(engine::BaseEngine* engine) const override;

private:
    SyncMode _mode;
};

class AsynchronousWorkEvent : public Event
{
public:
    [[nodiscard]] bool process_asynchronously() const override {return true;}
    [[nodiscard]] bool is_async_work_event() const override {return true;}
    virtual std::unique_ptr<Event> execute() = 0;

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

    std::unique_ptr<Event> execute() override;

private:
    AsynchronousWorkCallback _work_callback;
    void*                    _data;
    ObjectId                 _rt_processor;
    EventId                  _rt_event_id;
};

class AsynchronousBlobDeleteEvent : public AsynchronousWorkEvent
{
public:
    AsynchronousBlobDeleteEvent(BlobData data,
                                Time timestamp) : AsynchronousWorkEvent(timestamp),
                                                  _data(data) {}
    std::unique_ptr<Event> execute() override;

private:
    BlobData _data;
};

class AsynchronousDeleteEvent : public AsynchronousWorkEvent
{
public:
    AsynchronousDeleteEvent(RtDeletable* data,
                            Time timestamp) : AsynchronousWorkEvent(timestamp),
                                              _data(data) {}
    std::unique_ptr<Event> execute() override;

private:
    RtDeletable* _data;
};

/**
 * The subsequent events are processed immediately, in the non-real-time thread.
 * They do not map to real-time events, nor are they processed asynchronously.
 */

class ParameterChangeNotificationEvent : public Event
{
public:
    ParameterChangeNotificationEvent(ObjectId processor_id,
                                     ObjectId parameter_id,
                                     float normalized_value,
                                     float domain_value,
                                     std::string formatted_value,
                                     Time timestamp) : Event(timestamp),
                                                       _processor_id(processor_id),
                                                       _parameter_id(parameter_id),
                                                       _normalized_value(normalized_value),
                                                       _domain_value(domain_value),
                                                       _formatted_value(std::move(formatted_value)) {}

    [[nodiscard]] bool is_parameter_change_notification() const override {return true;}

    [[nodiscard]] ObjectId processor_id() const {return _processor_id;}

    [[nodiscard]] ObjectId parameter_id() const {return _parameter_id;}

    [[nodiscard]] float normalized_value() const {return _normalized_value;}

    [[nodiscard]] float domain_value() const {return _domain_value;}

    [[nodiscard]] const std::string& formatted_value() const {return _formatted_value;}

private:
    ObjectId    _processor_id;
    ObjectId    _parameter_id;
    float       _normalized_value;
    float       _domain_value;
    std::string _formatted_value;
};

class PropertyChangeNotificationEvent : public Event
{
public:
    PropertyChangeNotificationEvent(ObjectId processor_id,
                                    ObjectId property_id,
                                    std::string value,
                                    Time timestamp) : Event(timestamp),
                                                      _processor_id(processor_id),
                                                      _property_id(property_id),
                                                      _value(std::move(value)) {}

    [[nodiscard]] bool is_property_change_notification() const override {return true;}

    [[nodiscard]] ObjectId processor_id() const {return _processor_id;}

    [[nodiscard]] ObjectId property_id() const {return _property_id;}

    [[nodiscard]] const std::string& value() const {return _value;}

private:
    ObjectId    _processor_id;
    ObjectId    _property_id;
    std::string _value;
};

class EngineNotificationEvent : public Event
{
public:
    [[nodiscard]] bool is_engine_notification() const override {return true;}

    /* Convertible to ClippingNotification */
    [[nodiscard]] virtual bool is_clipping_notification() const {return false;}

    /* Convertible to AudioGraphNotification */
    [[nodiscard]] virtual bool is_audio_graph_notification() const {return false;}

    /* Convertible to TempoNotification */
    [[nodiscard]] virtual bool is_tempo_notification() const {return false;}

    /* Convertible to TimeSignatureNotification */
    [[nodiscard]] virtual bool is_time_sign_notification() const {return false;}

    /* Convertible to PlayingModeNotification */
    [[nodiscard]] virtual bool is_playing_mode_notification() const {return false;}

    /* Convertible to SyncModeNotification */
    [[nodiscard]] virtual bool is_sync_mode_notification() const {return false;}

    /* Convertible to TimingNotification */
    [[nodiscard]] virtual bool is_timing_notification() const {return false;}

    /* Convertible to TimingTickNotification */
    [[nodiscard]] virtual bool is_timing_tick_notification() const {return false;}

protected:
    EngineNotificationEvent(Time timestamp) : Event(timestamp) {}
};

class ClippingNotificationEvent : public EngineNotificationEvent
{
public:
    enum class ClipChannelType
    {
        INPUT,
        OUTPUT,
    };
    ClippingNotificationEvent(int channel,
                              ClipChannelType channel_type,
                              Time timestamp) : EngineNotificationEvent(timestamp),
                                                                                           _channel(channel),
                                                                                           _channel_type(channel_type) {}
    bool            is_clipping_notification() const override {return true;}
    int             channel() const {return _channel;}
    ClipChannelType channel_type() const {return _channel_type;}

private:
    int             _channel;
    ClipChannelType _channel_type;
};

class AudioGraphNotificationEvent : public EngineNotificationEvent
{
public:
    enum class Action
    {
        PROCESSOR_CREATED,
        PROCESSOR_DELETED,
        PROCESSOR_ADDED_TO_TRACK,
        PROCESSOR_REMOVED_FROM_TRACK,
        PROCESSOR_UPDATED,
        TRACK_CREATED,
        TRACK_DELETED
    };

    AudioGraphNotificationEvent(Action action,
                                ObjectId processor_id,
                                ObjectId track_id,
                                Time timestamp) : EngineNotificationEvent(timestamp),
                                                  _action(action),
                                                  _processor(processor_id),
                                                  _track(track_id) {}

    [[nodiscard]] bool     is_audio_graph_notification() const override {return true;}
    [[nodiscard]] Action   action() const {return _action;}
    [[nodiscard]] ObjectId processor() const {return _processor;}
    [[nodiscard]] ObjectId track() const {return _track;}

private:
    Action   _action;
    ObjectId _processor;
    ObjectId _track;
};

class TempoNotificationEvent : public EngineNotificationEvent
{
public: TempoNotificationEvent(float tempo, Time timestamp) : EngineNotificationEvent(timestamp),
                                                              _tempo(tempo) {}

    [[nodiscard]] bool is_tempo_notification() const override {return true;}
    [[nodiscard]] float tempo() const {return _tempo;}

private:
    float _tempo;
};

class TimeSignatureNotificationEvent : public EngineNotificationEvent
{
public: TimeSignatureNotificationEvent(TimeSignature signature,
                                       Time timestamp) : EngineNotificationEvent(timestamp),
                                                         _signature(signature) {}

    [[nodiscard]] bool is_time_sign_notification() const override {return true;}
    [[nodiscard]] TimeSignature time_signature() const {return _signature;}

private:
    TimeSignature _signature;
};

class PlayingModeNotificationEvent : public EngineNotificationEvent
{
public: PlayingModeNotificationEvent(PlayingMode mode, Time timestamp) : EngineNotificationEvent(timestamp),
                                                                         _mode(mode) {}

    [[nodiscard]] bool is_playing_mode_notification() const override {return true;}
    [[nodiscard]] PlayingMode mode() const {return _mode;}

private:
    PlayingMode _mode;
};

class SyncModeNotificationEvent : public EngineNotificationEvent
{
public: SyncModeNotificationEvent(SyncMode mode, Time timestamp) : EngineNotificationEvent(timestamp),
                                                                   _mode(mode) {}

    [[nodiscard]] bool is_sync_mode_notification() const override {return true;}
    [[nodiscard]] SyncMode mode() const {return _mode;}

private:
    SyncMode _mode;
};

class EngineTimingNotificationEvent : public EngineNotificationEvent
{
public:
    EngineTimingNotificationEvent(const performance::ProcessTimings& timings,
                                  Time timestamp) : EngineNotificationEvent(timestamp),
                                                    _timings(timings){}

    [[nodiscard]] bool is_timing_notification() const override {return true;}
    [[nodiscard]] const performance::ProcessTimings& timings() const {return _timings;}

private:
    performance::ProcessTimings _timings;
};

class EngineTimingTickNotificationEvent : public EngineNotificationEvent
{
public:
    EngineTimingTickNotificationEvent(int tick_count, Time timestamp) : EngineNotificationEvent(timestamp),
                                                                        _tick_count(tick_count) {}

    [[nodiscard]] bool is_timing_tick_notification() const override {return true;}
    [[nodiscard]] int tick_count() const {return _tick_count;}

private:
    int _tick_count;
};

} // end namespace sushi::internal

#endif // SUSHI_CONTROL_EVENT_H
