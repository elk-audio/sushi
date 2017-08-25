/**
 * @brief Definitions of events sent to process function.
 * @copyright MIND Music Labs AB, Stockholm
 */

#ifndef SUSHI_PLUGIN_EVENTS_H
#define SUSHI_PLUGIN_EVENTS_H

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
enum class EventType
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

class BaseEvent
{
public:
    /**
     * @brief Type of event.
     * @return
     */
    EventType type() const {return _type;};

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
    BaseEvent(EventType type, ObjectId target, int offset) : _type(type),
                                                             _processor_id(target),
                                                             _sample_offset(offset) {}
    EventType _type;
    ObjectId _processor_id;
    int _sample_offset;
};

/**
 * @brief Event class for all keyboard events.
 */
class KeyboardEvent : public BaseEvent
{
public:
    KeyboardEvent(EventType type, ObjectId target, int offset, int note, float velocity) : BaseEvent(type, target, offset),
                                                                                           _note(note),
                                                                                           _velocity(velocity)
    {
        assert(type == EventType::NOTE_ON ||
               type == EventType::NOTE_OFF ||
               type == EventType::NOTE_AFTERTOUCH);
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
class WrappedMidiEvent : public BaseEvent
{
public:
    WrappedMidiEvent(int offset, ObjectId target, uint8_t byte_0, uint8_t byte_1, uint8_t byte_2) : BaseEvent(EventType::WRAPPED_MIDI_EVENT, target, offset),
                                                                                                    _midi_data{byte_0, byte_1, byte_2} {}

    const uint8_t* midi_data() const {return _midi_data;}

protected:
    uint8_t _midi_data[3];
};
/**
 * @brief Baseclass for simple parameter changes
 */
class ParameterChangeEvent : public BaseEvent
{
public:
    ParameterChangeEvent(EventType type, ObjectId target, int offset, ObjectId param_id, float value) : BaseEvent(type, target, offset),
                                                                                                        _param_id(param_id),
                                                                                                        _value(value)
    {
        assert(type == EventType::FLOAT_PARAMETER_CHANGE ||
               type == EventType::INT_PARAMETER_CHANGE ||
               type == EventType::BOOL_PARAMETER_CHANGE);
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
class DataPayloadEvent : public BaseEvent
{
public:
    DataPayloadEvent(EventType type, ObjectId processor, int offset, BlobData data) : BaseEvent(type, processor, offset),
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
class StringParameterChangeEvent : public BaseEvent
{
public:
    StringParameterChangeEvent(ObjectId processor,
                               int offset,
                               ObjectId param_id,
                               std::string* value) : BaseEvent(EventType::STRING_PARAMETER_CHANGE,
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
                             BlobData value) : DataPayloadEvent(EventType::DATA_PARAMETER_CHANGE,
                                                             processor,
                                                             offset,
                                                             value),
                                            _param_id(param_id) {}

    ObjectId param_id() const {return _param_id;}

protected:
    ObjectId _param_id;
};

/**
 * @brief Baseclass for events that can be returned with a status code.
 */
class ReturnableEvent : public BaseEvent
{
public:
    enum class EventStatus : uint8_t
    {
        UNHANDLED,
        HANDLED_OK,
        HANDLED_ERROR
    };
    ReturnableEvent(EventType type) : BaseEvent(type, 0, 0),
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
    ProcessorOperationEvent(EventType type,
                            Processor* instance) : ReturnableEvent(type),
                                                   _instance{instance} {}
    Processor* instance() {return _instance;}
private:
    Processor* _instance;
};

class ProcessorReorderEvent : public ReturnableEvent
{
public:
    ProcessorReorderEvent(EventType type, ObjectId processor, ObjectId chain) : ReturnableEvent(type),
                                                                                _processor{processor},
                                                                                _chain{chain} {}
    ObjectId processor() {return _processor;}
    ObjectId chain() {return _chain;}
private:
    ObjectId _processor;
    ObjectId _chain;
};

/**
 * @brief Container class for events. Functionally this take the role of a
 *        baseclass for event from which you can access the derived event
 *        classes.
 */
class alignas(MIND_EVENT_CACHE_ALIGNMENT) Event
{
public:
    Event() {}

    EventType type() const {return _base_event.type();}

    ObjectId processor_id() const {return _base_event.processor_id();}

    int sample_offset() const {return _base_event.sample_offset();}

    /* Access functions protected by asserts */
    const KeyboardEvent* keyboard_event()
    {
        assert(_keyboard_event.type() == EventType::NOTE_ON ||
               _keyboard_event.type() == EventType::NOTE_OFF ||
               _keyboard_event.type() == EventType::NOTE_AFTERTOUCH);
        return &_keyboard_event;
    }

    const WrappedMidiEvent* wrapper_midi_event() const
    {
        assert(_wrapped_midi_event.type() == EventType::WRAPPED_MIDI_EVENT);
        return &_wrapped_midi_event;
    }

    const ParameterChangeEvent* parameter_change_event() const
    {
        assert(_keyboard_event.type() == EventType::FLOAT_PARAMETER_CHANGE);
        return &_parameter_change_event;
    }
    const StringParameterChangeEvent* string_parameter_change_event() const
    {
        assert(_string_parameter_change_event.type() == EventType::STRING_PARAMETER_CHANGE);
        return &_string_parameter_change_event;
    }
    const DataParameterChangeEvent* data_parameter_change_event() const
    {
        assert(_data_parameter_change_event.type() == EventType::DATA_PARAMETER_CHANGE);
        return &_data_parameter_change_event;
    }

    ReturnableEvent* returnable_event()
    {
        assert(_returnable_event.type() >= EventType::STOP_ENGINE);
        return &_returnable_event;
    }

    ProcessorOperationEvent* processor_operation_event()
    {
        assert(_processor_operation_event.type() == EventType::INSERT_PROCESSOR);
        return &_processor_operation_event;
    }

    ProcessorReorderEvent* processor_reorder_event()
    {
        assert(_processor_reorder_event.type() == EventType::REMOVE_PROCESSOR ||
               _processor_reorder_event.type() == EventType::ADD_PROCESSOR_TO_CHAIN ||
               _processor_reorder_event.type() == EventType::REMOVE_PROCESSOR_FROM_CHAIN ||
               _processor_reorder_event.type() == EventType::ADD_PLUGIN_CHAIN ||
               _processor_reorder_event.type() == EventType::REMOVE_PLUGIN_CHAIN);
        ;
        return &_processor_reorder_event;
    }

    /* Factory functions for constructing events */
    static Event make_note_on_event(ObjectId target, int offset, int note, float velocity)
    {
        return make_keyboard_event(EventType::NOTE_ON, target, offset, note, velocity);
    }

    static Event make_note_off_event(ObjectId target, int offset, int note, float velocity)
    {
        return make_keyboard_event(EventType::NOTE_OFF, target, offset, note, velocity);
    }

    static Event make_note_aftertouch_event(ObjectId target, int offset, int note, float velocity)
    {
        return make_keyboard_event(EventType::NOTE_AFTERTOUCH, target, offset, note, velocity);
    }

    static Event make_keyboard_event(EventType type, ObjectId target, int offset, int note, float velocity)
    {
        KeyboardEvent typed_event(type, target, offset, note, velocity);
        return Event(typed_event);
    }

    static Event make_parameter_change_event(ObjectId target, int offset, ObjectId param_id, float value)
    {
        ParameterChangeEvent typed_event(EventType::FLOAT_PARAMETER_CHANGE, target, offset, param_id, value);
        return Event(typed_event);
    }

    static Event make_wrapped_midi_event(ObjectId target, int offset,  uint8_t byte_0, uint8_t byte_1, uint8_t byte_2)
    {
        WrappedMidiEvent typed_event(offset, target, byte_0, byte_1, byte_2);
        return Event(typed_event);
    }

    static Event make_string_parameter_change_event(ObjectId target, int offset, ObjectId param_id, std::string* value)
    {
        StringParameterChangeEvent typed_event(target, offset, param_id, value);
        return Event(typed_event);
    }

    static Event make_data_parameter_change_event(ObjectId target, int offset, ObjectId param_id, BlobData data)
    {
        DataParameterChangeEvent typed_event(target, offset, param_id, data);
        return Event(typed_event);
    }

    static Event make_stop_engine_event()
    {
        ReturnableEvent typed_event(EventType::STOP_ENGINE);
        return Event(typed_event);
    }

    static Event make_insert_processor_event(Processor* instance)
    {
        ProcessorOperationEvent typed_event(EventType::INSERT_PROCESSOR, instance);
        return Event(typed_event);
    }

    static Event make_remove_processor_event(ObjectId processor)
    {
        ProcessorReorderEvent typed_event(EventType::REMOVE_PROCESSOR, processor, 0);
        return Event(typed_event);
    }

    static Event make_add_processor_to_chain_event(ObjectId processor, ObjectId chain)
    {
        ProcessorReorderEvent typed_event(EventType::ADD_PROCESSOR_TO_CHAIN, processor, chain);
        return Event(typed_event);
    }

    static Event make_remove_processor_from_chain_event(ObjectId processor, ObjectId chain)
    {
        ProcessorReorderEvent typed_event(EventType::REMOVE_PROCESSOR_FROM_CHAIN, processor, chain);
        return Event(typed_event);
    }

    static Event make_add_plugin_chain_event(ObjectId chain)
    {
        ProcessorReorderEvent typed_event(EventType::ADD_PLUGIN_CHAIN, 0, chain);
        return Event(typed_event);
    }

    static Event make_remove_plugin_chain_event(ObjectId chain)
    {
        ProcessorReorderEvent typed_event(EventType::REMOVE_PLUGIN_CHAIN, 0, chain);
        return Event(typed_event);
    }


private:
    Event(const KeyboardEvent& e) : _keyboard_event(e) {}
    Event(const ParameterChangeEvent& e) : _parameter_change_event(e) {}
    Event(const WrappedMidiEvent& e) : _wrapped_midi_event(e) {}
    Event(const StringParameterChangeEvent& e) : _string_parameter_change_event(e) {}
    Event(const DataParameterChangeEvent& e) : _data_parameter_change_event(e) {}
    Event(const ReturnableEvent& e) : _returnable_event(e) {}
    Event(const ProcessorOperationEvent& e) : _processor_operation_event(e) {}
    Event(const ProcessorReorderEvent& e) : _processor_reorder_event(e) {}
    union
    {
        BaseEvent                   _base_event;
        KeyboardEvent               _keyboard_event;
        WrappedMidiEvent            _wrapped_midi_event;
        ParameterChangeEvent        _parameter_change_event;
        StringParameterChangeEvent  _string_parameter_change_event;
        DataParameterChangeEvent    _data_parameter_change_event;
        ReturnableEvent             _returnable_event;
        ProcessorOperationEvent     _processor_operation_event;
        ProcessorReorderEvent       _processor_reorder_event;
    };
};

static_assert(sizeof(Event) == MIND_EVENT_CACHE_ALIGNMENT, "");
static_assert(std::is_trivially_copyable<Event>::value, "");



} // namespace sushi

#endif //SUSHI_PLUGIN_EVENTS_H
