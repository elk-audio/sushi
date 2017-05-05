
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


bool MidiDispatcher::connect_cc_to_parameter(int midi_input,
                                             const std::string &processor_name,
                                             const std::string &parameter_name,
                                             int cc_no,
                                             float min_range,
                                             float max_range,
                                             int channel)
{
    ObjectId processor_id;
    ObjectId parameter_id;
    EngineReturnStatus status;
    std::tie(status, processor_id) = _engine->processor_id_from_name(processor_name);
    if (status != EngineReturnStatus::OK)
    {
        return false;
    }
    std::tie(status, parameter_id) = _engine->parameter_id_from_name(processor_name, parameter_name);
    if (status != EngineReturnStatus::OK)
    {
        return false;
    }
    Connection connection;
    connection.target = processor_id;
    connection.parameter = parameter_id;
    connection.min_range = min_range;
    connection.max_range = max_range;
    _cc_routes[midi_input][cc_no][channel].push_back(connection);
    return true;
}


bool MidiDispatcher::connect_kb_to_track(int midi_input,
                                         const std::string &chain_name,
                                         int channel)
{
    ObjectId id;
    EngineReturnStatus status;
    std::tie(status, id) = _engine->processor_id_from_name(chain_name);
    if (status != EngineReturnStatus::OK)
    {
        return false;
    }
    Connection connection;
    connection.target = id;
    connection.parameter = 0;
    connection.min_range = 0;
    connection.max_range = 0;
    _kb_routes[midi_input][channel].push_back(connection);
    return true;
}


void MidiDispatcher::clear_connections()
{
    _cc_routes.clear();
    _kb_routes.clear();
}


void MidiDispatcher::process_midi(int input, int offset, const uint8_t* data, size_t size)
{
    midi::MessageType type = midi::decode_message_type(data, size);
    switch (type)
    {
        case midi::MessageType::CONTROL_CHANGE:
        {
            midi::ControlChangeMessage decoded_msg = midi::decode_control_change(data);
            const auto& cons = _cc_routes.find(input);
            if (cons != _cc_routes.end())
            {
                for (auto c : cons->second[decoded_msg.controller][midi::MidiChannel::OMNI])
                {
                    _engine->send_rt_event(make_param_change_event(c, decoded_msg, offset));
                }
                for (auto c : cons->second[decoded_msg.controller][decoded_msg.channel])
                {
                    _engine->send_rt_event(make_param_change_event(c, decoded_msg, offset));
                }
            }
            break;
        }

        case midi::MessageType::NOTE_ON:
        {
            midi::NoteOnMessage decoded_msg = midi::decode_note_on(data);
            const auto& cons = _kb_routes.find(input);
            if (cons != _kb_routes.end())
            {
                for (auto c : cons->second[midi::MidiChannel::OMNI])
                {
                    _engine->send_rt_event(make_note_on_event(c, decoded_msg, offset));
                }
                for (auto c : cons->second[decoded_msg.channel])
                {
                    _engine->send_rt_event(make_note_on_event(c, decoded_msg, offset));
                }
            }
            break;
        }

        case midi::MessageType::NOTE_OFF:
        {
            midi::NoteOffMessage decoded_msg = midi::decode_note_off(data);
            const auto& cons = _kb_routes.find(input);
            if (cons != _kb_routes.end())
            {
                for (auto c : cons->second[midi::MidiChannel::OMNI])
                {
                    _engine->send_rt_event(make_note_off_event(c, decoded_msg, offset));
                }
                for (auto c : cons->second[decoded_msg.channel])
                {
                    _engine->send_rt_event(make_note_off_event(c, decoded_msg, offset));
                }
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