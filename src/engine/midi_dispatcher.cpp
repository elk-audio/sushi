
#include "midi_dispatcher.h"
#include "engine/engine.h"
#include <iostream>

namespace sushi {
namespace engine {
namespace midi_dispatcher {


inline BaseEvent* make_note_on_event(const Connection &c,
                                     const midi::NoteOnMessage &msg,
                                     int offset)
{
    float velocity = msg.velocity / static_cast<float>(midi::MAX_VALUE);
    return new KeyboardEvent(EventType::NOTE_ON, c.target, offset, msg.note, velocity);
}


inline BaseEvent* make_note_off_event(const Connection &c,
                                      const midi::NoteOffMessage &msg,
                                      int offset)
{
    float velocity = msg.velocity / static_cast<float>(midi::MAX_VALUE);
    return new KeyboardEvent(EventType::NOTE_OFF, c.target, offset, msg.note, velocity);
}


inline BaseEvent* make_param_change_event(const Connection &c,
                                          const midi::ControlChangeMessage &msg,
                                          int offset)
{
    float value = static_cast<float>(msg.value) / midi::MAX_VALUE * (c.max_range - c.min_range) + c.min_range;
    return new ParameterChangeEvent(EventType::FLOAT_PARAMETER_CHANGE, c.target, offset, c.parameter, value);
}


inline const std::pair<ConnectionIter, ConnectionIter> get_route_iters_from_nested_map(int key_1,
                                                                                       int key_2,
                                                                                       const std::map<int, std::unordered_multimap<int, Connection>> &map)
{
    static const std::unordered_multimap<int, Connection> empty_map;
    const auto& by_key_1 = map.find(key_1);
    if (by_key_1 != map.end())
    {
        return by_key_1->second.equal_range(key_2);
    }
    return std::pair<ConnectionIter, ConnectionIter>(empty_map.end(), empty_map.end());
}


inline const std::pair<ConnectionIter, ConnectionIter> get_route_iters_from_double_nested_map(int key_1,
                                                                                              int key_2,
                                                                                              int key_3,
                                                                                              const std::map<int, std::map<int, std::unordered_multimap<int, Connection>>> &map)
{
    static const std::unordered_multimap<int, Connection> empty_map;
    const auto& by_key_1 = map.find(key_1);
    if (by_key_1 != map.end())
    {
        const auto& map_2 = by_key_1->second;
        const auto& by_key_2 = map_2.find(key_2);
        if (by_key_2 != map_2.end())
        {
            return by_key_2->second.equal_range(key_3);
        }
    }
    return std::pair<ConnectionIter, ConnectionIter>(empty_map.end(), empty_map.end());
}

inline const std::pair<ConnectionIter, ConnectionIter>& get_connection_iterators(int key_1, int key_2,
                                                                          const std::map<int, std::map<int, std::unordered_multimap<int, Connection>>>& map)
{

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
                                         const std::string &processor_id,
                                         int channel)
{
    Connection connection;
    connection.target = processor_id;
    connection.parameter = "";
    connection.min_range = 0;
    connection.max_range = 0;
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
            const auto routes = get_route_iters_from_nested_map(input, decoded_msg.controller, _cc_routes);
            for (auto route = routes.first; route != routes.second; ++route)
            {
                _engine->send_rt_event(make_param_change_event(route->second, decoded_msg, offset));
            }
            const auto routes_ch = get_route_iters_from_double_nested_map(input,
                                                                          decoded_msg.channel,
                                                                          decoded_msg.controller,
                                                                          _cc_routes_by_channel);
            for (auto route = routes_ch.first; route != routes_ch.second; ++route)
            {
                _engine->send_rt_event(make_param_change_event(route->second, decoded_msg, offset));
            }
            break;
        }

        case midi::MessageType::NOTE_ON:
        {
            midi::NoteOnMessage decoded_msg = midi::decode_note_on(data);
            auto routes = _kb_routes.equal_range(input);
            for (auto route = routes.first; route != routes.second; ++route)
            {
                _engine->send_rt_event(make_note_on_event(route->second, decoded_msg, offset));
            }
            const auto routes_ch = get_route_iters_from_nested_map(input, decoded_msg.channel, _kb_routes_by_channel);
            for (auto route = routes_ch.first; route != routes_ch.second; ++route)
            {
                _engine->send_rt_event(make_note_on_event(route->second, decoded_msg, offset));
            }
            break;
        }

        case midi::MessageType::NOTE_OFF:
        {
            midi::NoteOffMessage decoded_msg = midi::decode_note_off(data);
            const auto routes = _kb_routes.equal_range(input);
            for (auto route = routes.first; route != routes.second; ++route)
            {
                _engine->send_rt_event(make_note_off_event(route->second, decoded_msg, offset));
            }
            const auto routes_ch = get_route_iters_from_nested_map(input, decoded_msg.channel, _kb_routes_by_channel);
            for (auto route = routes_ch.first; route != routes_ch.second; ++route)
            {
                _engine->send_rt_event(make_note_off_event(route->second, decoded_msg, offset));
            }
            break;
        }

        case midi::MessageType::PITCH_BEND:
        case midi::MessageType::POLY_KEY_PRESSURE:
        case midi::MessageType::CHANNEL_PRESSURE:
        {}

        default:
            break;
    }

}


} // end namespace midi_dispatcher
}
} // end namespace sushi