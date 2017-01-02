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
enum class MindEventType
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

class BaseMindEvent
{
public:
    MindEventType type() { return _type;};
    virtual bool is_real_time();
    int sample_offset() {return _sample_offset;}

protected:
    BaseMindEvent(MindEventType type, int offset) : _type(type), _sample_offset(offset) {}
    MindEventType _type;
    int _sample_offset;
};

/**
 * @brief Event class for all keyboard events.
 */
class KeyboardEvent : BaseMindEvent
{
public:
    KeyboardEvent(MindEventType type, int offset, int note, float velocity) : KeyboardEvent::BaseMindEvent(_type, offset),
                                                                              _note(note),
                                                                              _velocity(velocity)
    {
        assert(type == MindEventType::NOTE_ON ||
               type == MindEventType::NOTE_OFF ||
               type == MindEventType::NOTE_AFTERTOUCH);
    }
    bool is_real_time() override { return true;}

protected:
    int _note;
    float _velocity;
};

/**
 * @brief This provides a way of "tunneling" raw midi for plugins that
 * handle midi natively. Could come in handy, or me might duplicate the entire
 * midi functionality in our own events.
 */
class WrappedMidiEvent : public BaseMindEvent
{
public:
    WrappedMidiEvent(int offset, uint8_t byte_0, uint8_t byte_1, uint8_t byte_2) : WrappedMidiEvent::BaseMindEvent(MindEventType::WRAPPED_MIDI_EVENT, offset),
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

/* TODO replace this with our own iterable container class or wrapper.*/

typedef std::vector<BaseMindEvent*> EventList;

} // namespace sushi

#endif //SUSHI_PLUGIN_EVENTS_H
