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

    /**
     * @brief Type of event.
     * @return
     */
    EventType type() { return _type;};

    /**
     * @brief The processor id of the target for this message.
     * @return
     */
    const std::string& processor_id() {return _processor_id;}

    /**
     * @brief If the message should be handled in the realtime context or not
     *        Maybe not neccesary.
     * @return
     */
    virtual bool is_real_time() = 0;

    /**
     * @brief For real time events that need sample accurate timing, how many
     *        samples into the current chunk the event should take effect.
     * @return
     */
    int sample_offset() {return _sample_offset;}

protected:
    BaseEvent(EventType type, const std::string& target, int offset) : _type(type),
                                                                _processor_id(target),
                                                                _sample_offset(offset) {}
    EventType _type;
    std::string _processor_id;
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
/**
 * @brief Baseclass for simple parameter changes
 */
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

/**
 * @brief Baseclass for events that need to carry a larger payload of data.
 */
class DataPayloadEvent : public BaseEvent
{
public:
    DataPayloadEvent(EventType type, const std::string& processor, int offset, void* data) : BaseEvent(type, processor, offset),
                                                                                             _data(data) {}


    void* data() {return _data;}

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
    bool handled() {return _handled;}

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
    StringParameterChangeEvent(const std::string& processor,
                               int offset,
                               const std::string& param_id,
                               std::string* value) : DataPayloadEvent(EventType::STRING_PARAMETER_CHANGE,
                                                                      processor,
                                                                      offset,
                                                                      static_cast<void*>(value)),
                                                     _param_id(param_id) {}

    const std::string& param_id() { return _param_id;}

    std::string* value(){return static_cast<std::string*>(_data);}

    bool is_real_time() override { return true;}

protected:
    std::string _param_id;
};

/**
 * @brief Class for binarydata parameter changes
 */
class DataParameterChangeEvent : public DataPayloadEvent
{
public:
    DataParameterChangeEvent(const std::string& processor,
                             int offset,
                             const std::string& param_id,
                             char* value) : DataPayloadEvent(EventType::DATA_PARAMETER_CHANGE,
                                                             processor,
                                                             offset,
                                                             static_cast<void*>(value)),
                                            _param_id(param_id) {}

    const std::string& param_id() { return _param_id;}

    char* value(){return static_cast<char*>(_data);}

    bool is_real_time() override { return true;}

protected:
    std::string _param_id;
};

/* TODO replace this with our own iterable container class or wrapper.*/

typedef std::vector<BaseEvent*> EventList;

} // namespace sushi

#endif //SUSHI_PLUGIN_EVENTS_H
