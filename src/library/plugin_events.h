/**
 * @brief Definitions of events sent to process function.
 * @copyright MIND Music Labs AB, Stockholm
 */

#ifndef SUSHI_PLUGIN_EVENTS_H
#define SUSHI_PLUGIN_EVENTS_H

#include <cassert>
#include <vector>
#include "id_generator.h"

namespace sushi {


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
    uint8_t midi_byte_0() const {return _midi_data[0];}
    uint8_t midi_byte_1() const {return _midi_data[1];}
    uint8_t midi_byte_2() const {return _midi_data[2];}

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
    DataPayloadEvent(EventType type, ObjectId processor, int offset, void* data) : BaseEvent(type, processor, offset),
                                                                                   _data(data) {}

    void* data() const {return _data;}

    /**
     * @brief Call to mark the event as handled. Data can not be accessed after this.
     * TODO - Maybe data can be a shared pointer instead, a refcounter that automatically
     * counts down when it goes out of scope?
     */
    void set_handled() {_handled = true;}

    /**
     * Returns true if the event is handled and the data is ok to delete.
     * @return
     */
    bool handled() const {return _handled;}

protected:
    void* _data;
    bool _handled{false};
};

/**
 * @brief Class for string parameter changes
 */
class StringParameterChangeEvent : public DataPayloadEvent
{
public:
    StringParameterChangeEvent(ObjectId processor,
                               int offset,
                               ObjectId param_id,
                               std::string* value) : DataPayloadEvent(EventType::STRING_PARAMETER_CHANGE,
                                                                      processor,
                                                                      offset,
                                                                      static_cast<void*>(value)),
                                                     _param_id(param_id) {}

    ObjectId param_id() const {return _param_id;}

    std::string* value() const {return static_cast<std::string*>(_data);}

protected:
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
                             void* value) : DataPayloadEvent(EventType::DATA_PARAMETER_CHANGE,
                                                             processor,
                                                             offset,
                                                             static_cast<void*>(value)),
                                            _param_id(param_id) {}

    ObjectId param_id() const {return _param_id;}

    char* value() const {return static_cast<char*>(_data);}

protected:
    ObjectId _param_id;
};


/**
 * @brief Container class for events. Functionally this take the role of a
 *        baseclass for event from which you can access the derived event
 *        classes.
 */
class Event
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

    static Event make_data_parameter_change_event(ObjectId target, int offset, ObjectId param_id, void* data)
    {
        DataParameterChangeEvent typed_event(target, offset, param_id, data);
        return Event(typed_event);
    }

private:
    Event(const KeyboardEvent& e) : _keyboard_event(e) {}
    Event(const ParameterChangeEvent& e) : _parameter_change_event(e) {}
    Event(const WrappedMidiEvent& e) : _wrapped_midi_event(e) {}
    Event(const StringParameterChangeEvent& e) : _string_parameter_change_event(e) {}
    Event(const DataParameterChangeEvent& e) : _data_parameter_change_event(e) {}
    union
    {
        BaseEvent                   _base_event;
        KeyboardEvent               _keyboard_event;
        WrappedMidiEvent            _wrapped_midi_event;
        ParameterChangeEvent        _parameter_change_event;
        StringParameterChangeEvent  _string_parameter_change_event;
        DataParameterChangeEvent    _data_parameter_change_event;
    };
} __attribute__ ((aligned (32)));

/* Currently limiting the size of an event to 32 bytes and forcing it to align
 * to 32 byte boundaries. We could possibly extend this to 64 bytes if neccesary,
 * but likely not further */

static_assert(sizeof(Event) == 32, "");
static_assert(std::is_trivially_copyable<Event>::value, "");



} // namespace sushi

#endif //SUSHI_PLUGIN_EVENTS_H
