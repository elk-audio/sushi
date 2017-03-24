
#include "midi_dispatcher.h"

namespace sushi {
namespace engine {
namespace midi_dispatcher {


inline BaseEvent* make_note_on_event(const Connection& c,
                                     const midi::NoteOnMessage& msg,
                                     int offset)
{
    float velocity =  msg.velocity / static_cast<float>(midi::MAX_VALUE);
    return new KeyboardEvent(EventType::NOTE_ON, c.target, offset, msg.note, velocity);
}

inline BaseEvent* make_note_off_event(const Connection& c,
                                      const midi::NoteOffMessage& msg,
                                      int offset)
{
    float velocity =  msg.velocity / static_cast<float>(midi::MAX_VALUE);
    return new KeyboardEvent(EventType::NOTE_OFF, c.target, offset, msg.note, velocity);
}

inline BaseEvent* make_param_change_event(const Connection& c,
                                          const midi::ControlChangeMessage& msg,
                                          int offset)
{
    float value = static_cast<float>(msg.value) / midi::MAX_VALUE * (c.max_range - c.min_range) + c.min_range;
    return new ParameterChangeEvent(EventType::FLOAT_PARAMETER_CHANGE, c.target, offset, c.parameter, value);
}

bool MidiDispatcher::connect_cc_to_parameter(int midi_input,
                                             const std::string &processor_id,
                                             const std::string &parameter_id,
                                             int cc_no,
                                             int min_range,
                                             int max_range,
                                             int channel)
{
    Connection connection;
    connection.target = processor_id;
    connection.parameter = parameter_id;
    connection.min_range = min_range;
    connection.max_range = max_range;
    if (channel == midi::MidiChannel::OMNI)
    {
        _cc_routes[midi_input].insert(std::pair<int, Connection>(cc_no, connection));
    }
    else
    {
        _cc_routes_by_channel[midi_input][channel].insert(std::pair<int, Connection>(cc_no, connection));
    }
    return true;
}


bool MidiDispatcher::connect_kb_to_track(int midi_input,
                                         const std::string& processor_id,
                                         int channel)
{
    Connection connection;
    connection.target = processor_id;
    if (channel == midi::MidiChannel::OMNI)
    {
        _kb_routes.insert(std::pair<int, Connection>(midi_input, connection));
    }
    else
    {
        _kb_routes_by_channel[midi_input].insert(std::pair<int, Connection>(channel, connection));
    }
    return true;
}


void MidiDispatcher::clear_connections()
{
    _cc_routes.clear();
    _cc_routes_by_channel.clear();
    _kb_routes.clear();
    _kb_routes_by_channel.clear();
}


void MidiDispatcher::process_midi(int input, int offset, const uint8_t* data, size_t size)
{
    midi::MessageType type = midi::decode_message_type(data, size);
    switch (type)
    {
        case midi::MessageType::CONTROL_CHANGE:
        {
            midi::ControlChangeMessage decoded_msg = midi::decode_control_change(data);
            auto routes = _cc_routes[input].equal_range(decoded_msg.controller);
            for (auto &route = routes.first; route != routes.second; ++route)
            {
                Connection& c = route->second;
                _engine->send_rt_event(make_param_change_event(c, decoded_msg, offset));
            }
            routes = _cc_routes_by_channel[input][decoded_msg.channel].equal_range(decoded_msg.controller);
            for (auto &route = routes.first; route != routes.second; ++route)
            {
                Connection& c = route->second;
                _engine->send_rt_event(make_param_change_event(c, decoded_msg, offset));
            }
            break;
        }
        case midi::MessageType::NOTE_ON:
        {
            midi::NoteOnMessage decoded_msg = midi::decode_note_on(data);
            auto routes = _kb_routes.equal_range(input);
            for (auto &route = routes.first; route != routes.second; ++route)
            {
                Connection& c = route->second;
                _engine->send_rt_event(make_note_on_event(c, decoded_msg, offset));
            }
            routes = _kb_routes_by_channel[input].equal_range(decoded_msg.channel);
            for (auto &route = routes.first; route != routes.second; ++route)
            {
                Connection& c = route->second;
                _engine->send_rt_event(make_note_on_event(c, decoded_msg, offset));
            }
            break;
        }
        case midi::MessageType::NOTE_OFF:
        {
            midi::NoteOffMessage decoded_msg = midi::decode_note_off(data);
            auto routes = _kb_routes.equal_range(input);
            for (auto &route = routes.first; route != routes.second; ++route)
            {
                Connection& c = route->second;
                _engine->send_rt_event(make_note_off_event(c, decoded_msg, offset));
            }
            routes = _kb_routes_by_channel[input].equal_range(decoded_msg.channel);
            for (auto &route = routes.first; route != routes.second; ++route)
            {
                Connection& c = route->second;
                _engine->send_rt_event(make_note_off_event(c, decoded_msg, offset));
            }
            break;
        }
        case midi::MessageType::PITCH_BEND:
        case midi::MessageType::POLY_KEY_PRESSURE:
        case midi::MessageType::CHANNEL_PRESSURE:
        {

        }
        default:
            break;
    }


}



} // end namespace midi_dispatcher
} // end namespace engine
} // end namespace sushi