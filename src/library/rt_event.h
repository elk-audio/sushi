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
 * @brief Set of compact and performance oriented events for use in the realtime part.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_RT_EVENTS_H
#define SUSHI_RT_EVENTS_H

#include <string>
#include <cassert>
#include <optional>

#include "id_generator.h"
#include "library/types.h"
#include "library/time.h"
#include "library/connection_types.h"

namespace sushi {

/* Currently limiting the size of an event to 32 bytes and forcing it to align
 * to 32 byte boundaries. We could possibly extend this to 64 bytes if neccesary,
 * but likely not further */
#ifndef SUSHI_EVENT_CACHE_ALIGNMENT
#define SUSHI_EVENT_CACHE_ALIGNMENT 32
#endif

class RtEvent;
inline bool is_keyboard_event(const RtEvent& event);
inline bool is_engine_control_event(const RtEvent& event);
inline bool is_returnable_event(const RtEvent& event);


/**
 * List of realtime message types
 */
enum class RtEventType
{
    /* Processor commands */
    NOTE_ON,
    NOTE_OFF,
    NOTE_AFTERTOUCH,
    PITCH_BEND,
    AFTERTOUCH,
    MODULATION,
    WRAPPED_MIDI_EVENT,
    GATE_EVENT,
    CV_EVENT,
    INT_PARAMETER_CHANGE,
    FLOAT_PARAMETER_CHANGE,
    BOOL_PARAMETER_CHANGE,
    DATA_PROPERTY_CHANGE,
    STRING_PROPERTY_CHANGE,
    SET_BYPASS,
    SET_STATE,
    DELETE,
    NOTIFY,
    /* Engine commands */
    TEMPO,
    TIME_SIGNATURE,
    PLAYING_MODE,
    SYNC_MODE,
    /* Processor add/delete/reorder events */
    INSERT_PROCESSOR,
    REMOVE_PROCESSOR,
    ADD_PROCESSOR_TO_TRACK,
    REMOVE_PROCESSOR_FROM_TRACK,
    ADD_TRACK,
    REMOVE_TRACK,
    ASYNC_WORK,
    ASYNC_WORK_NOTIFICATION,
    /* Routing events */
    ADD_AUDIO_CONNECTION,
    REMOVE_AUDIO_CONNECTION,
    ADD_CV_CONNECTION,
    REMOVE_CV_CONNECTION,
    ADD_GATE_CONNECTION,
    REMOVE_GATE_CONNECTION,
    /* Delete object event */
    BLOB_DELETE,
    /* Synchronisation events */
    SYNC,
    TIMING_TICK,
    /* Engine notification events */
    CLIP_NOTIFICATION,
};

class BaseRtEvent
{
public:
    /**
     * @brief Type of event.
     * @return
     */
    RtEventType type() const {return _type;};

    /**
     * @brief The processor id of the target for this message.
     * @return
     */
    ObjectId processor_id() const {return _processor_id;}

    /**
     * @brief For real time events that need sample accurate timing, how many
     *        samples into the current chunk the event should take effect.
     * @return
     */
    int sample_offset() const {return _sample_offset;}

protected:
    BaseRtEvent(RtEventType type, ObjectId target, int offset) : _type(type),
                                                                 _processor_id(target),
                                                                 _sample_offset(offset) {}
    RtEventType _type;
    ObjectId _processor_id;
    int _sample_offset;
};

/**
 * @brief Event class for all keyboard events.
 */
class KeyboardRtEvent : public BaseRtEvent
{
public:
    KeyboardRtEvent(RtEventType type,
                    ObjectId target,
                    int offset,
                    int channel,
                    int note,
                    float velocity) : BaseRtEvent(type, target, offset),
                                      _channel(channel),
                                      _note(note),
                                      _velocity(velocity)
    {
        assert(type == RtEventType::NOTE_ON ||
               type == RtEventType::NOTE_OFF ||
               type == RtEventType::NOTE_AFTERTOUCH);
    }
    int channel() const {return _channel;}
    int note() const {return _note;}
    float velocity() const {return _velocity;}

protected:
    int _channel;
    int _note;
    float _velocity;
};

class KeyboardCommonRtEvent : public BaseRtEvent
{
public:
    KeyboardCommonRtEvent(RtEventType type,
                          ObjectId target,
                          int offset,
                          int channel,
                          float value) : BaseRtEvent(type, target, offset),
                                         _channel(channel),
                                         _value(value)
    {
        assert(type == RtEventType::AFTERTOUCH ||
               type == RtEventType::PITCH_BEND ||
               type == RtEventType::MODULATION);
    }
    int channel() const {return _channel;}
    float value() const {return _value;}

protected:
    int _channel;
    float _value;
};

/**
 * @brief This provides a way of "tunneling" raw midi for plugins that
 * handle midi natively. Could come in handy, or me might duplicate the entire
 * midi functionality in our own events.
 */
class WrappedMidiRtEvent : public BaseRtEvent
{
public:
    WrappedMidiRtEvent(int offset,
                       ObjectId target,
                       MidiDataByte data) : BaseRtEvent(RtEventType::WRAPPED_MIDI_EVENT, target, offset),
                                            _midi_data{data} {}

    MidiDataByte midi_data() const {return _midi_data;}

protected:
    MidiDataByte _midi_data;
};

class GateRtEvent : public BaseRtEvent
{
public:
    GateRtEvent(ObjectId target,
                int offset,
                int gate_id,
                bool value) : BaseRtEvent(RtEventType::GATE_EVENT, target, offset),
                                          _gate_id(gate_id),
                                          _value(value) {}

    int gate_no() const {return _gate_id;}
    bool value() const {return _value;}

protected:
    int _gate_id;
    bool _value;
};

class CvRtEvent : public BaseRtEvent
{
public:
    CvRtEvent(ObjectId target,
                int offset,
                int cv_id,
                float value) : BaseRtEvent(RtEventType::CV_EVENT, target, offset),
                               _cv_id(cv_id),
                               _value(value) {}

    int cv_id() const {return _cv_id;}
    float value() const {return _value;}

protected:
    int _cv_id;
    float _value;
};


/**
 * @brief Baseclass for simple parameter changes
 */
class ParameterChangeRtEvent : public BaseRtEvent
{
public:
    ParameterChangeRtEvent(RtEventType type,
                           ObjectId target,
                           int offset,
                           ObjectId param_id,
                           float value) : BaseRtEvent(type, target, offset),
                                          _param_id(param_id),
                                          _value(value)
    {
        assert(type == RtEventType::FLOAT_PARAMETER_CHANGE ||
               type == RtEventType::INT_PARAMETER_CHANGE ||
               type == RtEventType::BOOL_PARAMETER_CHANGE);
    }
    ObjectId param_id() const {return _param_id;}

    float value() const {return _value;}

protected:
    ObjectId _param_id;
    float _value;
};


/**
 * @brief Baseclass for events that need to carry a larger payload of data.
 */
class DataPayloadRtEvent : public BaseRtEvent
{
public:
    DataPayloadRtEvent(RtEventType type,
                       ObjectId processor,
                       int offset,
                       BlobData data) : BaseRtEvent(type, processor, offset),
                                        _data_size(data.size),
                                        _data(data.data) {}

    BlobData value() const
    {
        return BlobData{_data_size, _data};
    }

protected:
    /* The members of BlobData are pulled apart and stored separately
     * here because this enables us to pack the members efficiently using only
     * self-aligning without having to resort to non-portable #pragmas or
     * __attributes__  to keep the size of the event small */
    int _data_size;
    uint8_t* _data;
};

/**
 * @brief Class for string parameter changes
 */
class PropertyChangeRtEvent : public BaseRtEvent
{
public:
    PropertyChangeRtEvent(ObjectId processor,
                          int offset,
                          ObjectId param_id,
                          RtDeletableWrapper<std::string>* value) : BaseRtEvent(RtEventType::STRING_PROPERTY_CHANGE,
                                                                                processor,
                                                                                offset),
                                                                   _data(value),
                                                                   _param_id(param_id) {}

    ObjectId param_id() const {return _param_id;}

    std::string* value() const {return &_data->data();}
    RtDeletable* deletable_value() const {return _data;}

protected:
    RtDeletableWrapper<std::string>* _data;
    ObjectId _param_id;
};


/**
 * @brief Class for binarydata parameter changes
 */
class DataPropertyChangeRtEvent : public DataPayloadRtEvent
{
public:
    DataPropertyChangeRtEvent(ObjectId processor,
                               int offset,
                               ObjectId param_id,
                               BlobData value) : DataPayloadRtEvent(RtEventType::DATA_PROPERTY_CHANGE,
                                                                    processor,
                                                                    offset,
                                                                    value),
                                                 _param_id(param_id) {}

    ObjectId param_id() const {return _param_id;}

protected:
    ObjectId _param_id;
};

/**
 * @brief Class for sending commands to processors.
 */
class ProcessorCommandRtEvent : public BaseRtEvent
{
public:
    ProcessorCommandRtEvent(RtEventType type,
                            ObjectId processor,
                            int value) : BaseRtEvent(type, processor, 0),
                                         _value(value)
    {
        assert(type == RtEventType::SET_BYPASS ||
               type == RtEventType::ASYNC_WORK_NOTIFICATION );
    }
    int value() const {return _value;}
private:
    int _value;
};

class RtState;
/**
 * @brief Class for binarydata parameter changes
 */
class ProcessorStateRtEvent : public BaseRtEvent
{
public:
    ProcessorStateRtEvent(ObjectId processor,
                          RtState* state) : BaseRtEvent(RtEventType::SET_STATE, processor, 0),
                                            _state(state) {}

    RtState* state() const {return _state;}

protected:
    RtState* _state;
};

/**
 * @brief Class for sending notifications from the rt thread of a processor
 */
class ProcessorNotifyRtEvent : public BaseRtEvent
{
public:
    enum class Action
    {
        PARAMETER_UPDATE
    };
    ProcessorNotifyRtEvent(ObjectId processor,
                           Action action) : BaseRtEvent(RtEventType::NOTIFY, processor, 0),
                                            _action(action) {}

    Action action() const {return _action;}

protected:
    Action _action;
};


/**
 * @brief Baseclass for events that can be returned with a status code.
 */
class ReturnableRtEvent : public BaseRtEvent
{
public:
    enum class EventStatus : uint8_t
    {
        UNHANDLED,
        HANDLED_OK,
        HANDLED_ERROR
    };
    ReturnableRtEvent(RtEventType type, ObjectId processor) : BaseRtEvent(type, processor, 0),
                                                              _status{EventStatus::UNHANDLED},
                                                              _event_id{EventIdGenerator::new_id()} {}

    EventStatus status() const {return _status;}
    uint16_t event_id() const {return _event_id;}
    void set_handled(bool ok) {_status = ok? EventStatus::HANDLED_OK : EventStatus::HANDLED_ERROR;}

protected:
    EventStatus _status;
    uint16_t _event_id;
};

class Processor;

class ProcessorOperationRtEvent : public ReturnableRtEvent
{
public:
    ProcessorOperationRtEvent(RtEventType type,
                              Processor* instance) : ReturnableRtEvent(type, 0),
                                                     _instance{instance} {}
    Processor* instance() const {return _instance;}
private:
    Processor* _instance;
};

class ProcessorReorderRtEvent : public ReturnableRtEvent
{
public:
    ProcessorReorderRtEvent(RtEventType type,
                            ObjectId processor,
                            ObjectId track,
                            std::optional<ObjectId> before_processor) : ReturnableRtEvent(type, 0),
                                                                        _processor{processor},
                                                                        _track{track},
                                                                        _before_processor{before_processor} {}

    ObjectId processor() const {return _processor;}
    ObjectId track() const {return _track;}
    const std::optional<ObjectId>& before_processor() const {return _before_processor;}

private:
    ObjectId _processor;
    ObjectId _track;
    std::optional<ObjectId> _before_processor;
};

typedef int (*AsyncWorkCallback)(void* data, EventId id);

class AsyncWorkRtEvent: public ReturnableRtEvent
{
public:
    AsyncWorkRtEvent(AsyncWorkCallback callback, ObjectId processor, void* data) : ReturnableRtEvent(RtEventType::ASYNC_WORK,
                                                                                                     processor),
                                                                                   _callback{callback},
                                                                                   _data{data} {}
    AsyncWorkCallback callback() const {return _callback;}
    void*             callback_data() const {return _data;}
private:
    AsyncWorkCallback _callback;
    void*             _data;
};

class AsyncWorkRtCompletionEvent : public ProcessorCommandRtEvent
{
public:
    AsyncWorkRtCompletionEvent(ObjectId processor,
                               uint16_t event_id,
                               int return_status) : ProcessorCommandRtEvent(RtEventType::ASYNC_WORK_NOTIFICATION,
                                                                            processor,
                                                                            return_status),
                                                    _event_id{event_id} {}

    uint16_t    sending_event_id() const {return _event_id;}
    int         return_status() const {return value();}

private:
    uint16_t  _event_id;
};

/* Base class for passing audio, cv and gate connections */
/* slightly hackish to repurpose the processor_id field for storing a
 * bool, but it allows us to keep the size down to 32 bytes.
 * Otherwise GateConnection wouldn't fit in the event */
template <typename ConnectionType>
class ConnectionRtEvent : public ReturnableRtEvent
{
public:
    ConnectionRtEvent(ConnectionType connection,
                      RtEventType type,
                      bool is_input_connection) : ReturnableRtEvent(type,
                                                                    is_input_connection ? 1 : 0),
                                                  _connection(connection) {}

    const ConnectionType& connection() const {return _connection;}
    bool  input_connection() const {return _processor_id == 1;}
    bool  output_connection() const {return _processor_id == 0;}

private:
    ConnectionType _connection;
};

/* RtEvents for passing and deleting audio, cv and gate mappings */
using AudioConnectionRtEvent = ConnectionRtEvent<AudioConnection>;
using CvConnectionRtEvent = ConnectionRtEvent<CvConnection>;
using GateConnectionRtEvent = ConnectionRtEvent<GateConnection>;


/* RtEvent for passing timestamps synced to sample offsets */
class SynchronisationRtEvent : public BaseRtEvent
{
public:
    SynchronisationRtEvent(Time timestamp) : BaseRtEvent(RtEventType::SYNC, 0, 0),
                                             _timestamp(timestamp) {}

    Time timestamp() const {return _timestamp;}

protected:
    Time _timestamp;
};

/* RtEvent for passing tempo information */
class TempoRtEvent : public BaseRtEvent
{
public:
    TempoRtEvent(int offset, float tempo) : BaseRtEvent(RtEventType::TEMPO, 0, offset),
                                            _tempo(tempo) {}

    float tempo() const {return _tempo;}

protected:
    float _tempo;
};

/* RtEvent for passing time signature information */
class TimeSignatureRtEvent : public BaseRtEvent
{
public:
    TimeSignatureRtEvent(int offset, TimeSignature signature) : BaseRtEvent(RtEventType::TIME_SIGNATURE, 0, offset),
                                                                _signature(signature) {}

    TimeSignature time_signature() const {return _signature;}

protected:
    TimeSignature _signature;
};

enum class PlayingMode
{
    STOPPED,
    PLAYING,
    RECORDING
};

/* RtEvent for passing transport commands */
class PlayingModeRtEvent : public BaseRtEvent
{
public:
    PlayingModeRtEvent(int offset, PlayingMode mode) : BaseRtEvent(RtEventType::PLAYING_MODE, 0, offset),
                                                       _mode(mode) {}

    PlayingMode mode() const {return _mode;}

protected:
    PlayingMode _mode;
};

enum class SyncMode
{
    INTERNAL,
    MIDI,
    GATE_INPUT,
    ABLETON_LINK
};

/* RtEvent for setting the mode of external tempo sync */
class SyncModeRtEvent : public BaseRtEvent
{
public:
    SyncModeRtEvent(int offset, SyncMode mode) : BaseRtEvent(RtEventType::SYNC_MODE, 0, offset),
                                                 _mode(mode) {}

    SyncMode mode() const {return _mode;}

protected:
    SyncMode _mode;
};

/* RtEvent for sending transport timing ticks for tempo sync */
class TimingTickRtEvent : public BaseRtEvent
{
public:
    TimingTickRtEvent(int offset, int tick_count) : BaseRtEvent(RtEventType::TIMING_TICK, 0, offset),
                                                    _tick_count(tick_count) {}

    int tick_count() const {return _tick_count;}
protected:
    int _tick_count;
};


/* RtEvent for notifing the engine of audio clipping in the realtime */
class ClipNotificationRtEvent : public BaseRtEvent
{
public:
    enum class ClipChannelType
    {
        INPUT,
        OUTPUT,
    };
    ClipNotificationRtEvent(int offset, int channel, ClipChannelType channel_type) : BaseRtEvent(RtEventType::CLIP_NOTIFICATION,
                                                                                                 0,
                                                                                                 offset),
                                                                                     _channel(channel),
                                                                                     _channel_type(channel_type) {}

    int channel() const {return _channel;}
    ClipChannelType channel_type() const {return _channel_type;}

private:
    int _channel;
    ClipChannelType _channel_type;
};

/**
 * @brief Class for passing deletable data out from the rt domain
 */
class DeleteDataRtEvent : public BaseRtEvent
{
public:
    DeleteDataRtEvent(RtDeletable* data) : BaseRtEvent(RtEventType::DELETE, 0, 0),
                                           _data(data)
    {}

    RtDeletable* data() const {return _data;}

private:
    RtDeletable* _data;
};

/**
 * @brief Container class for rt events. Functionally this takes the role of a
 *        baseclass for events, from which you can access the derived event
 *        classes via function calls that essentially casts the event to the
 *        given rt event type.
 */
class alignas(SUSHI_EVENT_CACHE_ALIGNMENT) RtEvent
{
public:
    RtEvent() {};

    RtEventType type() const {return _base_event.type();}

    ObjectId processor_id() const {return _base_event.processor_id();}

    int sample_offset() const {return _base_event.sample_offset();}

    /* Access functions protected by asserts */
    const KeyboardRtEvent* keyboard_event() const
    {
        assert(_keyboard_event.type() == RtEventType::NOTE_ON ||
               _keyboard_event.type() == RtEventType::NOTE_OFF ||
               _keyboard_event.type() == RtEventType::NOTE_AFTERTOUCH);
        return &_keyboard_event;
    }

    const KeyboardCommonRtEvent* keyboard_common_event() const
    {
        assert(_keyboard_event.type() == RtEventType::AFTERTOUCH ||
               _keyboard_event.type() == RtEventType::PITCH_BEND ||
               _keyboard_event.type() == RtEventType::MODULATION);
        return &_keyboard_common_event;
    }

    const WrappedMidiRtEvent* wrapped_midi_event() const
    {
        assert(_wrapped_midi_event.type() == RtEventType::WRAPPED_MIDI_EVENT);
        return &_wrapped_midi_event;
    }

    const GateRtEvent* gate_event() const
    {
        assert(_gate_event.type() == RtEventType::GATE_EVENT);
        return &_gate_event;
    }

    const CvRtEvent* cv_event() const
    {
        assert(_cv_event.type() == RtEventType::CV_EVENT);
        return &_cv_event;
    }

    const ParameterChangeRtEvent* parameter_change_event() const
    {
        assert(_keyboard_event.type() == RtEventType::FLOAT_PARAMETER_CHANGE);
        return &_parameter_change_event;
    }
    const PropertyChangeRtEvent* property_change_event() const
    {
        assert(_property_change_event.type() == RtEventType::STRING_PROPERTY_CHANGE);
        return &_property_change_event;
    }
    const DataPropertyChangeRtEvent* data_parameter_change_event() const
    {
        assert(_data_property_change_event.type() == RtEventType::DATA_PROPERTY_CHANGE);
        return &_data_property_change_event;
    }

    const ProcessorCommandRtEvent* processor_command_event() const
    {
        assert(_processor_command_event.type() == RtEventType::SET_BYPASS);
        return &_processor_command_event;
    }

    const ProcessorStateRtEvent* processor_state_event() const
    {
        assert(_processor_state_event.type() == RtEventType::SET_STATE);
        return &_processor_state_event;
    }

    const ProcessorNotifyRtEvent* processor_notify_event() const
    {
        assert(_processor_notify_event.type() == RtEventType::NOTIFY);
        return &_processor_notify_event;
    }

    // ReturnableEvent and every event type that inherits from it
    // needs a non-const accessor function as well

    const ReturnableRtEvent* returnable_event() const
    {
        assert(is_returnable_event(*this));
        return &_returnable_event;
    }

    ReturnableRtEvent* returnable_event()
    {
        assert(is_returnable_event(*this));
        return &_returnable_event;
    }

    const ProcessorOperationRtEvent* processor_operation_event() const
    {
        assert(_processor_operation_event.type() == RtEventType::INSERT_PROCESSOR);
        return &_processor_operation_event;
    }

    ProcessorOperationRtEvent* processor_operation_event()
    {
        assert(_processor_operation_event.type() == RtEventType::INSERT_PROCESSOR);
        return &_processor_operation_event;
    }

    const ProcessorReorderRtEvent* processor_reorder_event() const
    {
        assert(_processor_reorder_event.type() == RtEventType::REMOVE_PROCESSOR ||
               _processor_reorder_event.type() == RtEventType::ADD_PROCESSOR_TO_TRACK ||
               _processor_reorder_event.type() == RtEventType::REMOVE_PROCESSOR_FROM_TRACK ||
               _processor_reorder_event.type() == RtEventType::ADD_TRACK ||
               _processor_reorder_event.type() == RtEventType::REMOVE_TRACK);
        ;
        return &_processor_reorder_event;
    }

    ProcessorReorderRtEvent* processor_reorder_event()
    {
        assert(_processor_reorder_event.type() == RtEventType::REMOVE_PROCESSOR ||
               _processor_reorder_event.type() == RtEventType::ADD_PROCESSOR_TO_TRACK ||
               _processor_reorder_event.type() == RtEventType::REMOVE_PROCESSOR_FROM_TRACK ||
               _processor_reorder_event.type() == RtEventType::ADD_TRACK ||
               _processor_reorder_event.type() == RtEventType::REMOVE_TRACK);
        ;
        return &_processor_reorder_event;
    }

    const AsyncWorkRtEvent* async_work_event() const
    {
        assert(_async_work_event.type() == RtEventType::ASYNC_WORK);
        return &_async_work_event;
    }

    AsyncWorkRtEvent* async_work_event()
    {
        assert(_async_work_event.type() == RtEventType::ASYNC_WORK);
        return &_async_work_event;
    }

    const AsyncWorkRtCompletionEvent* async_work_completion_event() const
    {
        assert(_async_work_completion_event.type() == RtEventType::ASYNC_WORK_NOTIFICATION);
        return &_async_work_completion_event;
    }

    const AudioConnectionRtEvent* audio_connection_event() const
    {
        assert(_audio_connection_event.type() == RtEventType::ADD_AUDIO_CONNECTION ||
               _audio_connection_event.type() == RtEventType::REMOVE_AUDIO_CONNECTION);
        return &_audio_connection_event;
    }

    AudioConnectionRtEvent* audio_connection_event()
    {
        assert(_audio_connection_event.type() == RtEventType::ADD_AUDIO_CONNECTION ||
               _audio_connection_event.type() == RtEventType::REMOVE_AUDIO_CONNECTION);
        return &_audio_connection_event;
    }

    const CvConnectionRtEvent* cv_connection_event() const
    {
        assert(_cv_connection_event.type() == RtEventType::ADD_CV_CONNECTION ||
               _cv_connection_event.type() == RtEventType::REMOVE_CV_CONNECTION);
        return &_cv_connection_event;
    }

    const GateConnectionRtEvent* gate_connection_event() const
    {
        assert(_gate_connection_event.type() == RtEventType::ADD_GATE_CONNECTION ||
               _gate_connection_event.type() == RtEventType::REMOVE_GATE_CONNECTION);
        return &_gate_connection_event;
    }

    const DataPayloadRtEvent* data_payload_event() const
    {
        assert(_data_payload_event.type() == RtEventType::BLOB_DELETE);
        return &_data_payload_event;
    }

    const SynchronisationRtEvent* syncronisation_event() const
    {
        assert(_synchronisation_event.type() == RtEventType::SYNC);
        return &_synchronisation_event;
    }

    const TempoRtEvent* tempo_event() const
    {
        assert(_tempo_event.type() == RtEventType::TEMPO);
        return &_tempo_event;
    }

    const TimeSignatureRtEvent* time_signature_event() const
    {
        assert(_time_signature_event.type() == RtEventType::TIME_SIGNATURE);
        return &_time_signature_event;
    }

    const PlayingModeRtEvent* playing_mode_event() const
    {
        assert(_playing_mode_event.type() == RtEventType::PLAYING_MODE);
        return &_playing_mode_event;
    }

    const SyncModeRtEvent* sync_mode_event() const
    {
        assert(_sync_mode_event.type() == RtEventType::SYNC_MODE);
        return &_sync_mode_event;
    }

    const TimingTickRtEvent* timing_tick_event() const
    {
        assert(_timing_tick_event.type() == RtEventType::TIMING_TICK);
        return &_timing_tick_event;
    }

    const ClipNotificationRtEvent* clip_notification_event() const
    {
        assert(_clip_notification_event.type() == RtEventType::CLIP_NOTIFICATION);
        return &_clip_notification_event;
    }

    const DeleteDataRtEvent* delete_data_event() const
    {
        assert(_delete_data_event.type() == RtEventType::DELETE);
        return &_delete_data_event;
    }


    /* Factory functions for constructing events */
    static RtEvent make_note_on_event(ObjectId target, int offset, int channel, int note, float velocity)
    {
        return make_keyboard_event(RtEventType::NOTE_ON, target, offset, channel, note, velocity);
    }

    static RtEvent make_note_off_event(ObjectId target, int offset, int channel, int note, float velocity)
    {
        return make_keyboard_event(RtEventType::NOTE_OFF, target, offset, channel, note, velocity);
    }

    static RtEvent make_note_aftertouch_event(ObjectId target, int offset, int channel, int note, float velocity)
    {
        return make_keyboard_event(RtEventType::NOTE_AFTERTOUCH, target, offset, channel, note, velocity);
    }

    static RtEvent make_keyboard_event(RtEventType type, ObjectId target, int offset, int channel, int note, float velocity)
    {
        KeyboardRtEvent typed_event(type, target, offset, channel, note, velocity);
        return RtEvent(typed_event);
    }

    static RtEvent make_aftertouch_event(ObjectId target, int offset, int channel, float value)
    {
        return make_keyboard_common_event(RtEventType::AFTERTOUCH, target, offset, channel, value);
    }

    static RtEvent make_pitch_bend_event(ObjectId target, int offset, int channel, float value)
    {
        return make_keyboard_common_event(RtEventType::PITCH_BEND, target, offset, channel, value);
    }

    static RtEvent make_kb_modulation_event(ObjectId target, int offset, int channel, float value)
    {
        return make_keyboard_common_event(RtEventType::MODULATION, target, offset, channel, value);
    }

    static RtEvent make_keyboard_common_event(RtEventType type, ObjectId target, int offset, int channel, float value)
    {
        KeyboardCommonRtEvent typed_event(type, target, offset, channel, value);
        return RtEvent(typed_event);
    }

    static RtEvent make_gate_event(ObjectId target, int offset, int gate_id, bool value)
    {
        GateRtEvent typed_event(target, offset, gate_id, value);
        return RtEvent(typed_event);
    }

    static RtEvent make_cv_event(ObjectId target, int offset, int cv_id, float value)
    {
        CvRtEvent typed_event(target, offset, cv_id, value);
        return RtEvent(typed_event);
    }

    static RtEvent make_parameter_change_event(ObjectId target, int offset, ObjectId param_id, float value)
    {
        ParameterChangeRtEvent typed_event(RtEventType::FLOAT_PARAMETER_CHANGE, target, offset, param_id, value);
        return RtEvent(typed_event);
    }

    static RtEvent make_wrapped_midi_event(ObjectId target, int offset, MidiDataByte data)
    {
        WrappedMidiRtEvent typed_event(offset, target, data);
        return RtEvent(typed_event);
    }

    static RtEvent make_string_property_change_event(ObjectId target, int offset, ObjectId param_id, RtDeletableWrapper<std::string>* value)
    {
        PropertyChangeRtEvent typed_event(target, offset, param_id, value);
        return RtEvent(typed_event);
    }

    static RtEvent make_data_property_change_event(ObjectId target, int offset, ObjectId param_id, BlobData data)
    {
        DataPropertyChangeRtEvent typed_event(target, offset, param_id, data);
        return RtEvent(typed_event);
    }

    static RtEvent make_bypass_processor_event(ObjectId target, bool value)
    {
        ProcessorCommandRtEvent typed_event(RtEventType::SET_BYPASS, target, value);
        return RtEvent(typed_event);
    }

    static RtEvent make_set_rt_state_event(ObjectId target, RtState* state)
    {
        ProcessorStateRtEvent typed_event(target, state);
        return RtEvent(typed_event);
    }

    static RtEvent make_processor_notify_event(ObjectId target, ProcessorNotifyRtEvent::Action action)
    {
        ProcessorNotifyRtEvent typed_event(target, action);
        return RtEvent(typed_event);
    }

    static RtEvent make_insert_processor_event(Processor* instance)
    {
        ProcessorOperationRtEvent typed_event(RtEventType::INSERT_PROCESSOR, instance);
        return RtEvent(typed_event);
    }

    static RtEvent make_remove_processor_event(ObjectId processor)
    {
        ProcessorReorderRtEvent typed_event(RtEventType::REMOVE_PROCESSOR, processor, 0, std::nullopt);
        return RtEvent(typed_event);
    }

    static RtEvent make_add_processor_to_track_event(ObjectId processor,
                                                     ObjectId track,
                                                     std::optional<ObjectId> before_processor = std::nullopt)
    {
        ProcessorReorderRtEvent typed_event(RtEventType::ADD_PROCESSOR_TO_TRACK, processor, track, before_processor);
        return RtEvent(typed_event);
    }

    static RtEvent make_remove_processor_from_track_event(ObjectId processor, ObjectId track)
    {
        ProcessorReorderRtEvent typed_event(RtEventType::REMOVE_PROCESSOR_FROM_TRACK, processor, track, std::nullopt);
        return RtEvent(typed_event);
    }

    static RtEvent make_add_track_event(ObjectId track)
    {
        ProcessorReorderRtEvent typed_event(RtEventType::ADD_TRACK, 0, track, std::nullopt);
        return RtEvent(typed_event);
    }

    static RtEvent make_remove_track_event(ObjectId track)
    {
        ProcessorReorderRtEvent typed_event(RtEventType::REMOVE_TRACK, 0, track, std::nullopt);
        return RtEvent(typed_event);
    }

    static RtEvent make_async_work_event(AsyncWorkCallback callback, ObjectId processor, void* data)
    {
        AsyncWorkRtEvent typed_event(callback, processor, data);
        return typed_event;
    }

    static RtEvent make_async_work_completion_event(ObjectId processor, uint16_t event_id, int return_status)
    {
        AsyncWorkRtCompletionEvent typed_event(processor, event_id, return_status);
        return typed_event;
    }

    static RtEvent make_add_audio_input_connection_event(const AudioConnection& connection)
    {
        AudioConnectionRtEvent typed_event(connection, RtEventType::ADD_AUDIO_CONNECTION, true);
        return typed_event;
    }

    static RtEvent make_add_audio_output_connection_event(const AudioConnection& connection)
    {
        AudioConnectionRtEvent typed_event(connection, RtEventType::ADD_AUDIO_CONNECTION, false);
        return typed_event;
    }

    static RtEvent make_remove_audio_input_connection_event(const AudioConnection& connection)
    {
        AudioConnectionRtEvent typed_event(connection, RtEventType::REMOVE_AUDIO_CONNECTION, true);
        return typed_event;
    }

    static RtEvent make_remove_audio_output_connection_event(const AudioConnection& connection)
    {
        AudioConnectionRtEvent typed_event(connection, RtEventType::REMOVE_AUDIO_CONNECTION, false);
        return typed_event;
    }

    static RtEvent make_add_cv_input_connection_event(const CvConnection& connection)
    {
        CvConnectionRtEvent typed_event(connection, RtEventType::ADD_CV_CONNECTION, true);
        return typed_event;
    }

    static RtEvent make_add_cv_output_connection_event(const CvConnection& connection)
    {
        CvConnectionRtEvent typed_event(connection, RtEventType::ADD_CV_CONNECTION, false);
        return typed_event;
    }

    static RtEvent make_remove_cv_input_connection_event(const CvConnection& connection)
    {
        CvConnectionRtEvent typed_event(connection, RtEventType::REMOVE_CV_CONNECTION, true);
        return typed_event;
    }

    static RtEvent make_remove_cv_output_connection_event(const CvConnection& connection)
    {
        CvConnectionRtEvent typed_event(connection, RtEventType::REMOVE_CV_CONNECTION, false);
        return typed_event;
    }

    static RtEvent make_add_gate_input_connection_event(const GateConnection& connection)
    {
        GateConnectionRtEvent typed_event(connection, RtEventType::ADD_GATE_CONNECTION, true);
        return typed_event;
    }

    static RtEvent make_add_gate_output_connection_event(const GateConnection& connection)
    {
        GateConnectionRtEvent typed_event(connection, RtEventType::ADD_GATE_CONNECTION, false);
        return typed_event;
    }

    static RtEvent make_remove_gate_input_connection_event(const GateConnection& connection)
    {
        GateConnectionRtEvent typed_event(connection, RtEventType::REMOVE_GATE_CONNECTION, true);
        return typed_event;
    }

    static RtEvent make_remove_gate_output_connection_event(const GateConnection& connection)
    {
        GateConnectionRtEvent typed_event(connection, RtEventType::REMOVE_GATE_CONNECTION, false);
        return typed_event;
    }

    static RtEvent make_delete_blob_event(BlobData data)
    {
        DataPayloadRtEvent typed_event(RtEventType::BLOB_DELETE, 0, 0, data);
        return typed_event;
    }

    static RtEvent make_synchronisation_event(Time timestamp)
    {
        SynchronisationRtEvent typed_event(timestamp);
        return typed_event;
    }

    static RtEvent make_tempo_event(int offset, float tempo)
    {
        TempoRtEvent typed_event(offset, tempo);
        return typed_event;
    }

    static RtEvent make_time_signature_event(int offset, TimeSignature signature)
    {
        TimeSignatureRtEvent typed_event(offset, signature);
        return typed_event;
    }

    static RtEvent make_playing_mode_event(int offset, PlayingMode mode)
    {
        PlayingModeRtEvent typed_event(offset, mode);
        return typed_event;
    }

    static RtEvent make_sync_mode_event(int offset, SyncMode mode)
    {
        SyncModeRtEvent typed_event(offset, mode);
        return typed_event;
    }

    static RtEvent make_timing_tick_event(int offset, int tick_count)
    {
        TimingTickRtEvent typed_event(offset, tick_count);
        return typed_event;
    }

    static RtEvent make_clip_notification_event(int offset, int channel, ClipNotificationRtEvent::ClipChannelType type)
    {
        ClipNotificationRtEvent typed_event(offset, channel, type);
        return typed_event;
    }

    static RtEvent make_delete_data_event(RtDeletable* data)
    {
        DeleteDataRtEvent typed_event(data);
        return typed_event;
    }

private:
    /* Private constructors that are invoked automatically when using the make_xxx_event functions */
    RtEvent(const KeyboardRtEvent& e)                   : _keyboard_event(e) {}
    RtEvent(const KeyboardCommonRtEvent& e)             : _keyboard_common_event(e) {}
    RtEvent(const WrappedMidiRtEvent& e)                : _wrapped_midi_event(e) {}
    RtEvent(const GateRtEvent& e)                       : _gate_event(e) {}
    RtEvent(const CvRtEvent& e)                         : _cv_event(e) {}
    RtEvent(const ParameterChangeRtEvent& e)            : _parameter_change_event(e) {}
    RtEvent(const PropertyChangeRtEvent& e)             : _property_change_event(e) {}
    RtEvent(const DataPropertyChangeRtEvent& e)         : _data_property_change_event(e) {}
    RtEvent(const ProcessorCommandRtEvent& e)           : _processor_command_event(e) {}
    RtEvent(const ProcessorStateRtEvent& e)             : _processor_state_event(e) {}
    RtEvent(const ProcessorNotifyRtEvent& e)            : _processor_notify_event(e) {}
    RtEvent(const ReturnableRtEvent& e)                 : _returnable_event(e) {}
    RtEvent(const ProcessorOperationRtEvent& e)         : _processor_operation_event(e) {}
    RtEvent(const ProcessorReorderRtEvent& e)           : _processor_reorder_event(e) {}
    RtEvent(const AsyncWorkRtEvent& e)                  : _async_work_event(e) {}
    RtEvent(const AsyncWorkRtCompletionEvent& e)        : _async_work_completion_event(e) {}
    RtEvent(const AudioConnectionRtEvent& e)            : _audio_connection_event(e) {}
    RtEvent(const CvConnectionRtEvent& e)               : _cv_connection_event(e) {}
    RtEvent(const GateConnectionRtEvent& e)             : _gate_connection_event(e) {}
    RtEvent(const DataPayloadRtEvent& e)                : _data_payload_event(e) {}
    RtEvent(const SynchronisationRtEvent& e)            : _synchronisation_event(e) {}
    RtEvent(const TempoRtEvent& e)                      : _tempo_event(e) {}
    RtEvent(const TimeSignatureRtEvent& e)              : _time_signature_event(e) {}
    RtEvent(const PlayingModeRtEvent& e)                : _playing_mode_event(e) {}
    RtEvent(const SyncModeRtEvent& e)                   : _sync_mode_event(e) {}
    RtEvent(const ClipNotificationRtEvent& e)           : _clip_notification_event(e) {}
    RtEvent(const DeleteDataRtEvent& e)                 : _delete_data_event(e) {}
    RtEvent(const TimingTickRtEvent& e)                 : _timing_tick_event(e) {}
    /* Data storage */
    union
    {
        BaseRtEvent                   _base_event;
        KeyboardRtEvent               _keyboard_event;
        KeyboardCommonRtEvent         _keyboard_common_event;
        WrappedMidiRtEvent            _wrapped_midi_event;
        GateRtEvent                   _gate_event;
        CvRtEvent                     _cv_event;
        ParameterChangeRtEvent        _parameter_change_event;
        PropertyChangeRtEvent         _property_change_event;
        DataPropertyChangeRtEvent     _data_property_change_event;
        ProcessorCommandRtEvent       _processor_command_event;
        ProcessorStateRtEvent         _processor_state_event;
        ProcessorNotifyRtEvent        _processor_notify_event;
        ReturnableRtEvent             _returnable_event;
        ProcessorOperationRtEvent     _processor_operation_event;
        ProcessorReorderRtEvent       _processor_reorder_event;
        AsyncWorkRtEvent              _async_work_event;
        AsyncWorkRtCompletionEvent    _async_work_completion_event;
        AudioConnectionRtEvent        _audio_connection_event;
        CvConnectionRtEvent           _cv_connection_event;
        GateConnectionRtEvent         _gate_connection_event;
        DataPayloadRtEvent            _data_payload_event;
        SynchronisationRtEvent        _synchronisation_event;
        TempoRtEvent                  _tempo_event;
        TimeSignatureRtEvent          _time_signature_event;
        PlayingModeRtEvent            _playing_mode_event;
        SyncModeRtEvent               _sync_mode_event;
        ClipNotificationRtEvent       _clip_notification_event;
        DeleteDataRtEvent             _delete_data_event;
        TimingTickRtEvent             _timing_tick_event;
    };
};

/* Compile time check that the event container fits withing given memory constraints and
 * they can be safely copied without side effects. Important for real-time performance*/
static_assert(sizeof(RtEvent) == SUSHI_EVENT_CACHE_ALIGNMENT);
static_assert(std::is_trivially_copyable<RtEvent>::value);

/**
 * @brief Convenience function to encapsulate the logic to determine if it is a keyboard event
 *        and hence should be passed on to the next processor, or sent upwards.
 * @param event The event to test
 * @return true if the event is a keyboard event, false otherwise.
 */
inline bool is_keyboard_event(const RtEvent& event)
{
    return event.type() >= RtEventType::NOTE_ON && event.type() <= RtEventType::WRAPPED_MIDI_EVENT;
}

/**
 * @brief Convenience function to encapsulate the logic to determine if an event is only for
 *        internal engine control or if it can be passed to processor instances
 * @param event The event to test
 * @return true if the event is only for internal engine use.
 */
inline bool is_engine_control_event(const RtEvent& event)
{
    return event.type() >= RtEventType::TEMPO;
}

/**
 * @brief Convenience function to encapsulate the logic to determine if the event is
 *        returnable with a status code
 * @param event The event to test
 * @return true if the event can be converted to ReturnableRtEvent
 */
inline bool is_returnable_event(const RtEvent& event)
{
    return event.type() >= RtEventType::INSERT_PROCESSOR && event.type() <= RtEventType::REMOVE_GATE_CONNECTION;
}

} // namespace sushi

#endif //SUSHI_RT_EVENTS_H
