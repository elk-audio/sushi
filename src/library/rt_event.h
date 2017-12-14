/**
 * @brief Set of compact and performance oriented events for use in the realtime part.
 * @copyright MIND Music Labs AB, Stockholm
 */

#ifndef SUSHI_RT_EVENTS_H
#define SUSHI_RT_EVENTS_H

#include <string>
#include <cassert>
#include <vector>

#include "id_generator.h"
#include "library/types.h"

namespace sushi {

/* Currently limiting the size of an event to 32 bytes and forcing it to align
 * to 32 byte boundaries. We could possibly extend this to 64 bytes if neccesary,
 * but likely not further */
#ifndef MIND_EVENT_CACHE_ALIGNMENT
#define MIND_EVENT_CACHE_ALIGNMENT 32
#endif

/**
 * TODO - Very incomplete list of message types we might need.
 */
enum class RtEventType
{
    NOTE_ON,
    NOTE_OFF,
    NOTE_AFTERTOUCH,
    PITCH_BEND,
    AFTERTOUCH,
    MODULATION,
    WRAPPED_MIDI_EVENT,
    INT_PARAMETER_CHANGE,
    FLOAT_PARAMETER_CHANGE,
    BOOL_PARAMETER_CHANGE,
    /* Complex parameters like those below should only be updated through events
     * since a change should always be handled and could be expensive to handle */
    DATA_PARAMETER_CHANGE,
    STRING_PARAMETER_CHANGE,
    SET_BYPASS,
    /* Engine commands */
    STOP_ENGINE,
    /* Processor add/delete/reorder events */
    INSERT_PROCESSOR,
    REMOVE_PROCESSOR,
    ADD_PROCESSOR_TO_TRACK,
    REMOVE_PROCESSOR_FROM_TRACK,
    ADD_TRACK,
    REMOVE_TRACK,
    ASYNC_WORK,
    ASYNC_WORK_NOTIFICATION,
    /* Delete object event */
    STRING_DELETE,
    BLOB_DELETE,
    VOID_DELETE
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
    KeyboardRtEvent(RtEventType type, ObjectId target, int offset, int note, float velocity) : BaseRtEvent(type, target, offset),
                                                                                             _note(note),
                                                                                             _velocity(velocity)
    {
        assert(type == RtEventType::NOTE_ON ||
               type == RtEventType::NOTE_OFF ||
               type == RtEventType::NOTE_AFTERTOUCH);
    }
    int note() const {return _note;}
    float velocity() const {return _velocity;}

protected:
    int _note;
    float _velocity;
};

class KeyboardCommonRtEvent : public BaseRtEvent
{
public:
    KeyboardCommonRtEvent(RtEventType type, ObjectId target, int offset, float value) : BaseRtEvent(type, target, offset),
                                                                                         _value(value)
    {
        assert(type == RtEventType::AFTERTOUCH ||
               type == RtEventType::PITCH_BEND ||
               type == RtEventType::MODULATION);
    }
    float value() const {return _value;}

protected:
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
    WrappedMidiRtEvent(int offset, ObjectId target, MidiDataByte data) : BaseRtEvent(RtEventType::WRAPPED_MIDI_EVENT, target, offset),
                                                                         _midi_data{data} {}

    MidiDataByte midi_data() const {return _midi_data;}

protected:
    MidiDataByte _midi_data;
};
/**
 * @brief Baseclass for simple parameter changes
 */
class ParameterChangeRtEvent : public BaseRtEvent
{
public:
    ParameterChangeRtEvent(RtEventType type, ObjectId target, int offset, ObjectId param_id, float value) : BaseRtEvent(type, target, offset),
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
    DataPayloadRtEvent(RtEventType type, ObjectId processor, int offset, BlobData data) : BaseRtEvent(type, processor, offset),
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
class StringParameterChangeRtEvent : public BaseRtEvent
{
public:
    StringParameterChangeRtEvent(ObjectId processor,
                                 int offset,
                                 ObjectId param_id,
                                 std::string* value) : BaseRtEvent(RtEventType::STRING_PARAMETER_CHANGE,
                                                                   processor,
                                                                   offset),
                                                       _data(value),
                                                       _param_id(param_id) {}

    ObjectId param_id() const {return _param_id;}

    std::string* value() const {return _data;}

protected:
    std::string* _data;
    ObjectId _param_id;
};


/**
 * @brief Class for binarydata parameter changes
 */
class DataParameterChangeRtEvent : public DataPayloadRtEvent
{
public:
    DataParameterChangeRtEvent(ObjectId processor,
                               int offset,
                               ObjectId param_id,
                               BlobData value) : DataPayloadRtEvent(RtEventType::DATA_PARAMETER_CHANGE,
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
    Processor* instance() {return _instance;}
private:
    Processor* _instance;
};

class ProcessorReorderRtEvent : public ReturnableRtEvent
{
public:
    ProcessorReorderRtEvent(RtEventType type, ObjectId processor, ObjectId track) : ReturnableRtEvent(type, 0),
                                                                                    _processor{processor},
                                                                                    _track{track} {}
    ObjectId processor() {return _processor;}
    ObjectId track() {return _track;}
private:
    ObjectId _processor;
    ObjectId _track;
};

typedef int (*AsyncWorkCallback)(void* data, EventId id);

class AsyncWorkRtEvent: public ReturnableRtEvent
{
public:
    AsyncWorkRtEvent(AsyncWorkCallback callback, ObjectId processor, void* data) : ReturnableRtEvent(RtEventType::ASYNC_WORK, processor),
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
                               int return_status) : ProcessorCommandRtEvent(RtEventType::ASYNC_WORK_NOTIFICATION, processor, return_status),
                                                    _event_id{event_id} {}

    uint16_t    sending_event_id() const {return _event_id;}
    int         return_status() const {return value();}
private:
    uint16_t  _event_id;
};
/**
 * @brief Container class for rt events. Functionally this take the role of a
 *        baseclass for events, from which you can access the derived event
 *        classes via function calls that essentially casts the event to the
 *        given rt event type.
 */
class alignas(MIND_EVENT_CACHE_ALIGNMENT) RtEvent
{
public:
    RtEvent() {}

    RtEventType type() const {return _base_event.type();}

    ObjectId processor_id() const {return _base_event.processor_id();}

    int sample_offset() const {return _base_event.sample_offset();}

    /* Access functions protected by asserts */
    const KeyboardRtEvent* keyboard_event()
    {
        assert(_keyboard_event.type() == RtEventType::NOTE_ON ||
               _keyboard_event.type() == RtEventType::NOTE_OFF ||
               _keyboard_event.type() == RtEventType::NOTE_AFTERTOUCH);
        return &_keyboard_event;
    }

    const KeyboardCommonRtEvent* keyboard_common_event()
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

    const ParameterChangeRtEvent* parameter_change_event() const
    {
        assert(_keyboard_event.type() == RtEventType::FLOAT_PARAMETER_CHANGE);
        return &_parameter_change_event;
    }
    const StringParameterChangeRtEvent* string_parameter_change_event() const
    {
        assert(_string_parameter_change_event.type() == RtEventType::STRING_PARAMETER_CHANGE);
        return &_string_parameter_change_event;
    }
    const DataParameterChangeRtEvent* data_parameter_change_event() const
    {
        assert(_data_parameter_change_event.type() == RtEventType::DATA_PARAMETER_CHANGE);
        return &_data_parameter_change_event;
    }

    const ProcessorCommandRtEvent* processor_command_event() const
    {
        assert(_processor_command_event.type() == RtEventType::SET_BYPASS);
        return &_processor_command_event;
    }

    ReturnableRtEvent* returnable_event()
    {
        assert(_returnable_event.type() >= RtEventType::STOP_ENGINE);
        return &_returnable_event;
    }

    ProcessorOperationRtEvent* processor_operation_event()
    {
        assert(_processor_operation_event.type() == RtEventType::INSERT_PROCESSOR);
        return &_processor_operation_event;
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

    AsyncWorkRtEvent* async_work_event()
    {
        assert(_async_work_event.type() == RtEventType::ASYNC_WORK);
        return &_async_work_event;
    }

    AsyncWorkRtCompletionEvent* async_work_completion_event()
    {
        assert(_async_work_completion_event.type() == RtEventType::ASYNC_WORK_NOTIFICATION);
        return &_async_work_completion_event;
    }

    DataPayloadRtEvent* data_payload_event()
    {
        assert(_processor_reorder_event.type() == RtEventType::STRING_DELETE ||
               _processor_reorder_event.type() == RtEventType::BLOB_DELETE ||
               _processor_reorder_event.type() == RtEventType::VOID_DELETE );
        return &_data_payload_event;

    }

    /* Factory functions for constructing events */
    static RtEvent make_note_on_event(ObjectId target, int offset, int note, float velocity)
    {
        return make_keyboard_event(RtEventType::NOTE_ON, target, offset, note, velocity);
    }

    static RtEvent make_note_off_event(ObjectId target, int offset, int note, float velocity)
    {
        return make_keyboard_event(RtEventType::NOTE_OFF, target, offset, note, velocity);
    }

    static RtEvent make_note_aftertouch_event(ObjectId target, int offset, int note, float velocity)
    {
        return make_keyboard_event(RtEventType::NOTE_AFTERTOUCH, target, offset, note, velocity);
    }

    static RtEvent make_keyboard_event(RtEventType type, ObjectId target, int offset, int note, float velocity)
    {
        KeyboardRtEvent typed_event(type, target, offset, note, velocity);
        return RtEvent(typed_event);
    }

    static RtEvent make_aftertouch_event(ObjectId target, int offset, float value)
    {
        return make_keyboard_common_event(RtEventType::AFTERTOUCH, target, offset, value);
    }

    static RtEvent make_pitch_bend_event(ObjectId target, int offset, float value)
    {
        return make_keyboard_common_event(RtEventType::PITCH_BEND, target, offset, value);
    }

    static RtEvent make_kb_modulation_event(ObjectId target, int offset, float value)
    {
        return make_keyboard_common_event(RtEventType::MODULATION, target, offset, value);
    }

    static RtEvent make_keyboard_common_event(RtEventType type, ObjectId target, int offset, float value)
    {
        KeyboardCommonRtEvent typed_event(type, target, offset, value);
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

    static RtEvent make_string_parameter_change_event(ObjectId target, int offset, ObjectId param_id, std::string* value)
    {
        StringParameterChangeRtEvent typed_event(target, offset, param_id, value);
        return RtEvent(typed_event);
    }

    static RtEvent make_data_parameter_change_event(ObjectId target, int offset, ObjectId param_id, BlobData data)
    {
        DataParameterChangeRtEvent typed_event(target, offset, param_id, data);
        return RtEvent(typed_event);
    }

    static RtEvent make_bypass_processor_event(ObjectId target, bool value)
    {
        ProcessorCommandRtEvent typed_event(RtEventType::SET_BYPASS, target, value);
        return RtEvent(typed_event);
    }

    static RtEvent make_stop_engine_event()
    {
        ReturnableRtEvent typed_event(RtEventType::STOP_ENGINE, 0);
        return RtEvent(typed_event);
    }

    static RtEvent make_insert_processor_event(Processor* instance)
    {
        ProcessorOperationRtEvent typed_event(RtEventType::INSERT_PROCESSOR, instance);
        return RtEvent(typed_event);
    }

    static RtEvent make_remove_processor_event(ObjectId processor)
    {
        ProcessorReorderRtEvent typed_event(RtEventType::REMOVE_PROCESSOR, processor, 0);
        return RtEvent(typed_event);
    }

    static RtEvent make_add_processor_to_track_event(ObjectId processor, ObjectId track)
    {
        ProcessorReorderRtEvent typed_event(RtEventType::ADD_PROCESSOR_TO_TRACK, processor, track);
        return RtEvent(typed_event);
    }

    static RtEvent make_remove_processor_from_track_event(ObjectId processor, ObjectId track)
    {
        ProcessorReorderRtEvent typed_event(RtEventType::REMOVE_PROCESSOR_FROM_TRACK, processor, track);
        return RtEvent(typed_event);
    }

    static RtEvent make_add_track_event(ObjectId track)
    {
        ProcessorReorderRtEvent typed_event(RtEventType::ADD_TRACK, 0, track);
        return RtEvent(typed_event);
    }

    static RtEvent make_remove_track_event(ObjectId track)
    {
        ProcessorReorderRtEvent typed_event(RtEventType::REMOVE_TRACK, 0, track);
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

    static RtEvent make_delete_string_event(std::string* string)
    {
        DataPayloadRtEvent typed_event(RtEventType::STRING_DELETE, 0, 0, {0, reinterpret_cast<uint8_t*>(string)});
        return typed_event;
    }

    static RtEvent make_delete_blob_event(BlobData data)
    {
        DataPayloadRtEvent typed_event(RtEventType::BLOB_DELETE, 0, 0, data);
        return typed_event;
    }

    static RtEvent make_delete_void_event(void* data)
    {
        DataPayloadRtEvent typed_event(RtEventType::VOID_DELETE, 0, 0, {0, reinterpret_cast<uint8_t*>(data)});
        return typed_event;
    }


private:
    RtEvent(const KeyboardRtEvent& e) : _keyboard_event(e) {}
    RtEvent(const KeyboardCommonRtEvent& e) : _keyboard_common_event(e) {}
    RtEvent(const ParameterChangeRtEvent& e) : _parameter_change_event(e) {}
    RtEvent(const WrappedMidiRtEvent& e) : _wrapped_midi_event(e) {}
    RtEvent(const StringParameterChangeRtEvent& e) : _string_parameter_change_event(e) {}
    RtEvent(const DataParameterChangeRtEvent& e) : _data_parameter_change_event(e) {}
    RtEvent(const ProcessorCommandRtEvent& e) : _processor_command_event(e) {}
    RtEvent(const ReturnableRtEvent& e) : _returnable_event(e) {}
    RtEvent(const ProcessorOperationRtEvent& e) : _processor_operation_event(e) {}
    RtEvent(const ProcessorReorderRtEvent& e) : _processor_reorder_event(e) {}
    RtEvent(const AsyncWorkRtEvent& e) : _async_work_event(e) {}
    RtEvent(const AsyncWorkRtCompletionEvent& e) : _async_work_completion_event(e) {}
    RtEvent(const DataPayloadRtEvent& e) : _data_payload_event(e) {}
    union
    {
        BaseRtEvent                   _base_event;
        KeyboardRtEvent               _keyboard_event;
        KeyboardCommonRtEvent         _keyboard_common_event;
        WrappedMidiRtEvent            _wrapped_midi_event;
        ParameterChangeRtEvent        _parameter_change_event;
        StringParameterChangeRtEvent  _string_parameter_change_event;
        DataParameterChangeRtEvent    _data_parameter_change_event;
        ProcessorCommandRtEvent       _processor_command_event;
        ReturnableRtEvent             _returnable_event;
        ProcessorOperationRtEvent     _processor_operation_event;
        ProcessorReorderRtEvent       _processor_reorder_event;
        AsyncWorkRtEvent              _async_work_event;
        AsyncWorkRtCompletionEvent    _async_work_completion_event;
        DataPayloadRtEvent            _data_payload_event;
    };
};

static_assert(sizeof(RtEvent) == MIND_EVENT_CACHE_ALIGNMENT, "");
static_assert(std::is_trivially_copyable<RtEvent>::value, "");

/**
 * @brief Convenience function to encapsulate the logic to determine if it is a keyboard event
 *        and hence should be passed on to the next processor, or sent upwards.
 * @param event The event to test
 * @return true if the event is a keyboard event, false otherwise.
 */
static inline bool is_keyboard_event(const RtEvent event)
{
    if (event.type() >= RtEventType::NOTE_ON && event.type() <= RtEventType::WRAPPED_MIDI_EVENT)
    {
        return true;
    }
    return false;
}

} // namespace sushi

#endif //SUSHI_RT_EVENTS_H
