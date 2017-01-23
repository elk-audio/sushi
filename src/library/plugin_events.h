/**
 * @brief Definitions of events sent to process function.
 * @copyright MIND Music Labs AB, Stockholm
 */

#ifndef SUSHI_PLUGIN_EVENTS_H
#define SUSHI_PLUGIN_EVENTS_H

#include <cassert>
#include <vector>

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
    virtual ~BaseEvent() {}
    EventType type() { return _type;};
    const std::string& target_id() {return _target_id;}
    virtual bool is_real_time() = 0;
    int sample_offset() {return _sample_offset;}

protected:
    BaseEvent(EventType type, const std::string& target, int offset) : _type(type),
                                                                _target_id(target),
                                                                _sample_offset(offset) {}
    EventType _type;
    std::string _target_id;
    int _sample_offset;
};

/**
 * @brief Event class for all keyboard events.
 */
class KeyboardEvent : public BaseEvent
{
public:
    KeyboardEvent(EventType type, const std::string& target, int offset, int note, float velocity) : BaseEvent(type, target, offset),
                                                                                                     _note(note),
                                                                                                     _velocity(velocity)
    {
        assert(type == EventType::NOTE_ON ||
               type == EventType::NOTE_OFF ||
               type == EventType::NOTE_AFTERTOUCH);
    }
    bool is_real_time() override { return true;}
    int note() {return _note;}
    float velocity() {return _velocity;}

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
    WrappedMidiEvent(int offset, std::string& target, uint8_t byte_0, uint8_t byte_1, uint8_t byte_2) : WrappedMidiEvent::BaseEvent(EventType::WRAPPED_MIDI_EVENT, target, offset),
                                                                                                        _midi_byte_0(byte_0),
                                                                                                        _midi_byte_1(byte_1),
                                                                                                        _midi_byte_2(byte_2) {}

    uint8_t midi_byte_0() {return _midi_byte_0;}
    uint8_t midi_byte_1() {return _midi_byte_1;}
    uint8_t midi_byte_2() {return _midi_byte_2;}

    bool is_real_time() override { return true;}

protected:
    uint8_t _midi_byte_0;
    uint8_t _midi_byte_1;
    uint8_t _midi_byte_2;
};

class ParameterChangeEvent : public BaseEvent
{
public:
    ParameterChangeEvent(EventType type, const std::string& target, int offset, const std::string& param_id, float value) : BaseEvent(type, target, offset),
                                                                                                                            _param_id(param_id),
                                                                                                                            _value(value)
    {
        assert(type == EventType::FLOAT_PARAMETER_CHANGE ||
               type == EventType::INT_PARAMETER_CHANGE ||
               type == EventType::BOOL_PARAMETER_CHANGE);
    }
    const std::string& param_id() { return _param_id;}

    float value(){return _value;}

    bool is_real_time() override { return true;}

protected:
    std::string _param_id;
    float _value;
};

/* TODO replace this with our own iterable container class or wrapper.*/

typedef std::vector<BaseEvent*> EventList;

} // namespace sushi

#endif //SUSHI_PLUGIN_EVENTS_H
