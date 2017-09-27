
#include "midi_dispatcher.h"
#include "logging.h"

namespace sushi {
namespace midi_dispatcher {

MIND_GET_LOGGER;

inline Event make_note_on_event(const Connection &c,
                                const midi::NoteOnMessage &msg,
                                int offset)
{
    float velocity = msg.velocity / static_cast<float>(midi::MAX_VALUE);
    return Event::make_note_on_event(c.target, offset, msg.note, velocity);
}

inline Event make_note_off_event(const Connection &c,
                                 const midi::NoteOffMessage &msg,
                                 int offset)
{
    float velocity = msg.velocity / static_cast<float>(midi::MAX_VALUE);
    return Event::make_note_off_event(c.target, offset, msg.note, velocity);
}

inline Event make_param_change_event(const Connection &c,
                                     const midi::ControlChangeMessage &msg,
                                     int offset)
{
    float value = static_cast<float>(msg.value) / midi::MAX_VALUE * (c.max_range - c.min_range) + c.min_range;
    return Event::make_parameter_change_event(c.target, offset, c.parameter, value);
}

MidiDispatcherStatus MidiDispatcher::connect_cc_to_parameter(int midi_input,
                                                             const std::string &processor_name,
                                                             const std::string &parameter_name,
                                                             int cc_no,
                                                             float min_range,
                                                             float max_range,
                                                             int channel)
{
    if (midi_input >= _midi_inputs || midi_input < 0 || midi_input > midi::MidiChannel::OMNI)
    {
        return MidiDispatcherStatus ::INVALID_MIDI_INPUT;
    }
    ObjectId processor_id;
    ObjectId parameter_id;
    engine::EngineReturnStatus status;
    std::tie(status, processor_id) = _engine->processor_id_from_name(processor_name);
    std::tie(status, parameter_id) = _engine->parameter_id_from_name(processor_name, parameter_name);
    if (status != engine::EngineReturnStatus::OK)
    {
        if(status == engine::EngineReturnStatus::INVALID_PROCESSOR)
        {
            return MidiDispatcherStatus::INVALID_PROCESSOR;
        }
        return MidiDispatcherStatus::INVALID_PARAMETER;
    }
    Connection connection;
    connection.target = processor_id;
    connection.parameter = parameter_id;
    connection.min_range = min_range;
    connection.max_range = max_range;
    _cc_routes[midi_input][cc_no][channel].push_back(connection);
    MIND_LOG_DEBUG("Connected parameter \"{}\" "
                           "(cc number \"{}\") to processor \"{}\"", parameter_name, cc_no, processor_name);
    return MidiDispatcherStatus::OK;
}

MidiDispatcherStatus MidiDispatcher::connect_kb_to_track(int midi_input,
                                                         const std::string &chain_name,
                                                         int channel)
{
    if (midi_input >= _midi_inputs || midi_input < 0 || midi_input > midi::MidiChannel::OMNI)
    {
        return MidiDispatcherStatus::INVALID_MIDI_INPUT;
    }
    ObjectId id;
    engine::EngineReturnStatus status;
    std::tie(status, id) = _engine->processor_id_from_name(chain_name);
    if (status != engine::EngineReturnStatus::OK)
    {
        return MidiDispatcherStatus::INVALID_CHAIN_NAME;
    }
    Connection connection;
    connection.target = id;
    connection.parameter = 0;
    connection.min_range = 0;
    connection.max_range = 0;
    _kb_routes[midi_input][channel].push_back(connection);
    MIND_LOG_DEBUG("Connected MIDI port \"{}\" to chain \"{}\"", midi_input, chain_name);
    return MidiDispatcherStatus::OK;
}

void MidiDispatcher::clear_connections()
{
    _cc_routes.clear();
    _kb_routes.clear();
}

void MidiDispatcher::process_midi(int input, int offset, const uint8_t* data, size_t size, bool realtime)
{
    std::function<engine::EngineReturnStatus(engine::BaseEngine*, Event&)> dispatch_fun = realtime? &engine::BaseEngine::send_rt_event :
                                                                                                    &engine::BaseEngine::send_async_event;
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
                    auto event = make_param_change_event(c, decoded_msg, offset);
                    dispatch_fun(_engine, event);
                }
                for (auto c : cons->second[decoded_msg.controller][decoded_msg.channel])
                {
                    auto event = make_param_change_event(c, decoded_msg, offset);
                    dispatch_fun(_engine, event);
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
                    auto event = make_note_on_event(c, decoded_msg, offset);
                    dispatch_fun(_engine, event);
                }
                for (auto c : cons->second[decoded_msg.channel])
                {
                    auto event = make_note_on_event(c, decoded_msg, offset);
                    dispatch_fun(_engine, event);
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
                    auto event = make_note_off_event(c, decoded_msg, offset);
                    dispatch_fun(_engine, event);
                }
                for (auto c : cons->second[decoded_msg.channel])
                {
                    auto event = make_note_off_event(c, decoded_msg, offset);
                    dispatch_fun(_engine, event);
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
} // end namespace sushi