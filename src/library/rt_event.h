/**
 * @brief Definitions of events sent to process function.
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
    ADD_PROCESSOR_TO_CHAIN,
    REMOVE_PROCESSOR_FROM_CHAIN,
    ADD_PLUGIN_CHAIN,
    REMOVE_PLUGIN_CHAIN,
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
class KeyboardEvent : public BaseRtEvent
{
public:
    KeyboardEvent(RtEventType type, ObjectId target, int offset, int note, float velocity) : BaseRtEvent(type, target, offset),
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

/**
 * @brief This provides a way of "tunneling" raw midi for plugins that
 * handle midi natively. Could come in handy, or me might duplicate the entire
 * midi functionality in our own events.
 */
class WrappedMidiEvent : public BaseRtEvent
{
public:
    WrappedMidiEvent(int offset, ObjectId target, uint8_t byte_0, uint8_t byte_1, uint8_t byte_2) : BaseRtEvent(RtEventType::WRAPPED_MIDI_EVENT, target, offset),
                                                                                                    _midi_data{byte_0, byte_1, byte_2} {}

    const uint8_t* midi_data() const {return _midi_data;}

protected:
    uint8_t _midi_data[3];
};
/**
 * @brief Baseclass for simple parameter changes
 */
class ParameterChangeEvent : public BaseRtEvent
{
public:
    ParameterChangeEvent(RtEventType type, ObjectId target, int offset, ObjectId param_id, float value) : BaseRtEvent(type, target, offset),
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
class DataPayloadEvent : public BaseRtEvent
{
public:
    DataPayloadEvent(RtEventType type, ObjectId processor, int offset, BlobData data) : BaseRtEvent(type, processor, offset),
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
class StringParameterChangeEvent : public BaseRtEvent
{
public:
    StringParameterChangeEvent(ObjectId processor,
                               int offset,
                               ObjectId param_id,
                               std::string* value) : BaseRtEvent(RtEventType::STRING_PARAMETER_CHANGE,
                                                               processor,
                                                               offset),
                                                     _data(value),
                                                     _param_id(param_id) {}

    ObjectId param_id() const {return _param_id;}

    std::string* value() const {return static_cast<std::string*>(_data);}

protected:
    std::string* _data;
    ObjectId _param_id;
};


/**
 * @brief Class for binarydata parameter changes
 */
class DataParameterChangeEvent : public DataPayloadEvent
{
public:
    DataParameterChangeEvent(ObjectId processor,
                             int offset,
                             ObjectId param_id,
                             BlobData value) : DataPayloadEvent(RtEventType::DATA_PARAMETER_CHANGE,
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
class ProcessorCommandEvent : public BaseRtEvent
{
public:
    ProcessorCommandEvent(RtEventType type,
                          ObjectId processor,
                          bool value) : BaseRtEvent(type, processor, 0),
                                        _value(value)
    {
        assert(type == RtEventType::SET_BYPASS);
    }
    bool value() const {return _value;}
private:
    bool _value;
};
/**
 * @brief Baseclass for events that can be returned with a status code.
 */
class ReturnableEvent : public BaseRtEvent
{
public:
    enum class EventStatus : uint8_t
    {
        UNHANDLED,
        HANDLED_OK,
        HANDLED_ERROR
    };
    ReturnableEvent(RtEventType type) : BaseRtEvent(type, 0, 0),
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

class ProcessorOperationEvent : public ReturnableEvent
{
public:
    ProcessorOperationEvent(RtEventType type,
                            Processor* instance) : ReturnableEvent(type),
                                                   _instance{instance} {}
    Processor* instance() {return _instance;}
private:
    Processor* _instance;
};

class ProcessorReorderEvent : public ReturnableEvent
{
public:
    ProcessorReorderEvent(RtEventType type, ObjectId processor, ObjectId chain) : ReturnableEvent(type),
                                                                                _processor{processor},
                                                                                _chain{chain} {}
    ObjectId processor() {return _processor;}
    ObjectId chain() {return _chain;}
private:
    ObjectId _processor;
    ObjectId _chain;
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
    const KeyboardEvent* keyboard_event()
    {
        assert(_keyboard_event.type() == RtEventType::NOTE_ON ||
               _keyboard_event.type() == RtEventType::NOTE_OFF ||
               _keyboard_event.type() == RtEventType::NOTE_AFTERTOUCH);
        return &_keyboard_event;
    }

    const WrappedMidiEvent* wrapper_midi_event() const
    {
        assert(_wrapped_midi_event.type() == RtEventType::WRAPPED_MIDI_EVENT);
        return &_wrapped_midi_event;
    }

    const ParameterChangeEvent* parameter_change_event() const
    {
        assert(_keyboard_event.type() == RtEventType::FLOAT_PARAMETER_CHANGE);
        return &_parameter_change_event;
    }
    const StringParameterChangeEvent* string_parameter_change_event() const
    {
        assert(_string_parameter_change_event.type() == RtEventType::STRING_PARAMETER_CHANGE);
        return &_string_parameter_change_event;
    }
    const DataParameterChangeEvent* data_parameter_change_event() const
    {
        assert(_data_parameter_change_event.type() == RtEventType::DATA_PARAMETER_CHANGE);
        return &_data_parameter_change_event;
    }

    const ProcessorCommandEvent* processor_command_event() const
    {
        assert(_processor_command_event.type() == RtEventType::SET_BYPASS);
        return &_processor_command_event;
    }

    ReturnableEvent* returnable_event()
    {
        assert(_returnable_event.type() >= RtEventType::STOP_ENGINE);
        return &_returnable_event;
    }

    ProcessorOperationEvent* processor_operation_event()
    {
        assert(_processor_operation_event.type() == RtEventType::INSERT_PROCESSOR);
        return &_processor_operation_event;
    }

    ProcessorReorderEvent* processor_reorder_event()
    {
        assert(_processor_reorder_event.type() == RtEventType::REMOVE_PROCESSOR ||
               _processor_reorder_event.type() == RtEventType::ADD_PROCESSOR_TO_CHAIN ||
               _processor_reorder_event.type() == RtEventType::REMOVE_PROCESSOR_FROM_CHAIN ||
               _processor_reorder_event.type() == RtEventType::ADD_PLUGIN_CHAIN ||
               _processor_reorder_event.type() == RtEventType::REMOVE_PLUGIN_CHAIN);
        ;
        return &_processor_reorder_event;
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
        KeyboardEvent typed_event(type, target, offset, note, velocity);
        return RtEvent(typed_event);
    }

    static RtEvent make_parameter_change_event(ObjectId target, int offset, ObjectId param_id, float value)
    {
        ParameterChangeEvent typed_event(RtEventType::FLOAT_PARAMETER_CHANGE, target, offset, param_id, value);
        return RtEvent(typed_event);
    }

    static RtEvent make_wrapped_midi_event(ObjectId target, int offset,  uint8_t byte_0, uint8_t byte_1, uint8_t byte_2)
    {
        WrappedMidiEvent typed_event(offset, target, byte_0, byte_1, byte_2);
        return RtEvent(typed_event);
    }

    static RtEvent make_string_parameter_change_event(ObjectId target, int offset, ObjectId param_id, std::string* value)
    {
        StringParameterChangeEvent typed_event(target, offset, param_id, value);
        return RtEvent(typed_event);
    }

    static RtEvent make_data_parameter_change_event(ObjectId target, int offset, ObjectId param_id, BlobData data)
    {
        DataParameterChangeEvent typed_event(target, offset, param_id, data);
        return RtEvent(typed_event);
    }

    static RtEvent make_bypass_processor_event(ObjectId target, bool value)
    {
        ProcessorCommandEvent typed_event(RtEventType::SET_BYPASS, target, value);
        return RtEvent(typed_event);
    }

    static RtEvent make_stop_engine_event()
    {
        ReturnableEvent typed_event(RtEventType::STOP_ENGINE);
        return RtEvent(typed_event);
    }

    static RtEvent make_insert_processor_event(Processor* instance)
    {
        ProcessorOperationEvent typed_event(RtEventType::INSERT_PROCESSOR, instance);
        return RtEvent(typed_event);
    }

    static RtEvent make_remove_processor_event(ObjectId processor)
    {
        ProcessorReorderEvent typed_event(RtEventType::REMOVE_PROCESSOR, processor, 0);
        return RtEvent(typed_event);
    }

    static RtEvent make_add_processor_to_chain_event(ObjectId processor, ObjectId chain)
    {
        ProcessorReorderEvent typed_event(RtEventType::ADD_PROCESSOR_TO_CHAIN, processor, chain);
        return RtEvent(typed_event);
    }

    static RtEvent make_remove_processor_from_chain_event(ObjectId processor, ObjectId chain)
    {
        ProcessorReorderEvent typed_event(RtEventType::REMOVE_PROCESSOR_FROM_CHAIN, processor, chain);
        return RtEvent(typed_event);
    }

    static RtEvent make_add_plugin_chain_event(ObjectId chain)
    {
        ProcessorReorderEvent typed_event(RtEventType::ADD_PLUGIN_CHAIN, 0, chain);
        return RtEvent(typed_event);
    }

    static RtEvent make_remove_plugin_chain_event(ObjectId chain)
    {
        ProcessorReorderEvent typed_event(RtEventType::REMOVE_PLUGIN_CHAIN, 0, chain);
        return RtEvent(typed_event);
    }


private:
    RtEvent(const KeyboardEvent& e) : _keyboard_event(e) {}
    RtEvent(const ParameterChangeEvent& e) : _parameter_change_event(e) {}
    RtEvent(const WrappedMidiEvent& e) : _wrapped_midi_event(e) {}
    RtEvent(const StringParameterChangeEvent& e) : _string_parameter_change_event(e) {}
    RtEvent(const DataParameterChangeEvent& e) : _data_parameter_change_event(e) {}
    RtEvent(const ProcessorCommandEvent& e) : _processor_command_event(e) {}
    RtEvent(const ReturnableEvent& e) : _returnable_event(e) {}
    RtEvent(const ProcessorOperationEvent& e) : _processor_operation_event(e) {}
    RtEvent(const ProcessorReorderEvent& e) : _processor_reorder_event(e) {}
    union
    {
        BaseRtEvent                   _base_event;
        KeyboardEvent               _keyboard_event;
        WrappedMidiEvent            _wrapped_midi_event;
        ParameterChangeEvent        _parameter_change_event;
        StringParameterChangeEvent  _string_parameter_change_event;
        DataParameterChangeEvent    _data_parameter_change_event;
        ProcessorCommandEvent       _processor_command_event;
        ReturnableEvent             _returnable_event;
        ProcessorOperationEvent     _processor_operation_event;
        ProcessorReorderEvent       _processor_reorder_event;
    };
};

static_assert(sizeof(RtEvent) == MIND_EVENT_CACHE_ALIGNMENT, "");
static_assert(std::is_trivially_copyable<RtEvent>::value, "");



} // namespace sushi

#endif //SUSHI_RT_EVENTS_H
