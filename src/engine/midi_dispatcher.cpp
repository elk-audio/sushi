
#include "engine/midi_dispatcher.h"
#include "library/midi_encoder.h"
#include "logging.h"

namespace sushi {
namespace midi_dispatcher {

MIND_GET_LOGGER_WITH_MODULE_NAME("midi dispatcher");

inline Event* make_note_on_event(const InputConnection &c,
                                 const midi::NoteOnMessage &msg,
                                 Time timestamp)
{
    float velocity = msg.velocity / static_cast<float>(midi::MAX_VALUE);
    return new KeyboardEvent(KeyboardEvent::Subtype::NOTE_ON, c.target, msg.channel, msg.note, velocity, timestamp);
}

inline Event* make_note_off_event(const InputConnection &c,
                                  const midi::NoteOffMessage &msg,
                                  Time timestamp)
{
    float velocity = msg.velocity / static_cast<float>(midi::MAX_VALUE);
    return new KeyboardEvent(KeyboardEvent::Subtype::NOTE_OFF, c.target, msg.channel, msg.note, velocity, timestamp);
}

inline Event* make_note_aftertouch_event(const InputConnection &c,
                                         const midi::PolyKeyPressureMessage &msg,
                                         Time timestamp)
{
    float pressure = msg.pressure / static_cast<float>(midi::MAX_VALUE);
    return new KeyboardEvent(KeyboardEvent::Subtype::NOTE_AFTERTOUCH, c.target, msg.channel, msg.note, pressure, timestamp);
}

inline Event* make_aftertouch_event(const InputConnection &c,
                                    const midi::ChannelPressureMessage &msg,
                                    Time timestamp)
{
    float pressure = msg.pressure / static_cast<float>(midi::MAX_VALUE);
    return new KeyboardEvent(KeyboardEvent::Subtype::AFTERTOUCH, c.target, msg.channel, pressure, timestamp);
}

inline Event* make_modulation_event(const InputConnection &c,
                                    const midi::ControlChangeMessage &msg,
                                    Time timestamp)
{
    float value = msg.value / static_cast<float>(midi::MAX_VALUE);
    return new KeyboardEvent(KeyboardEvent::Subtype::MODULATION, c.target, msg.channel, value, timestamp);
}

inline Event* make_pitch_bend_event(const InputConnection &c,
                                    const midi::PitchBendMessage &msg,
                                    Time timestamp)
{
    float value = (msg.value / static_cast<float>(midi::PITCH_BEND_MIDDLE)) - 1.0f;
    return new KeyboardEvent(KeyboardEvent::Subtype::PITCH_BEND, c.target, msg.channel, value, timestamp);
}

inline Event* make_wrapped_midi_event(const InputConnection &c,
                                      const uint8_t* data,
                                      size_t size,
                                      Time timestamp)
{
    MidiDataByte midi_data{0};
    std::copy(data, data + size, midi_data.data());
    return new KeyboardEvent(KeyboardEvent::Subtype::WRAPPED_MIDI, c.target, midi_data, timestamp);
}

inline Event* make_param_change_event(const InputConnection &c,
                                      const midi::ControlChangeMessage &msg,
                                      Time timestamp)
{
    float value = static_cast<float>(msg.value) / midi::MAX_VALUE * (c.max_range - c.min_range) + c.min_range;
    return new ParameterChangeEvent(ParameterChangeEvent::Subtype::FLOAT_PARAMETER_CHANGE, c.target, c.parameter, value, timestamp);
}

inline Event* make_program_change_event(const InputConnection &c,
                                        const midi::ProgramChangeMessage &msg,
                                        Time timestamp)
{
    return new ProgramChangeEvent(c.target, msg.program, timestamp);
}


MidiDispatcher::MidiDispatcher(engine::BaseEngine* engine) : _engine(engine),
                                                             _frontend(nullptr)
{
    // TODO - eventually we can pass the event dispatcher directly and avoid the engine dependency
    _event_dispatcher = _engine->event_dispatcher();
    _event_dispatcher->register_poster(this);
    _event_dispatcher->subscribe_to_keyboard_events(this);
}

MidiDispatcher::~MidiDispatcher()
{
    _event_dispatcher->unsubscribe_from_keyboard_events(this);
    _event_dispatcher->deregister_poster(this);
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
    auto [proc_status, processor_id] = _engine->processor_id_from_name(processor_name);
    auto [param_status, parameter_id] = _engine->parameter_id_from_name(processor_name, parameter_name);
    if (proc_status != engine::EngineReturnStatus::OK)
    {
        return MidiDispatcherStatus::INVALID_PROCESSOR;
    }
    if (param_status != engine::EngineReturnStatus::OK)
    {
        return MidiDispatcherStatus::INVALID_PARAMETER;
    }

    InputConnection connection;
    connection.target = processor_id;
    connection.parameter = parameter_id;
    connection.min_range = min_range;
    connection.max_range = max_range;
    _cc_routes[midi_input][cc_no][channel].push_back(connection);
    MIND_LOG_INFO("Connected parameter \"{}\" "
                           "(cc number \"{}\") to processor \"{}\"", parameter_name, cc_no, processor_name);
    return MidiDispatcherStatus::OK;
}

MidiDispatcherStatus MidiDispatcher::connect_pc_to_processor(int midi_input,
                                                             const std::string& processor_name,
                                                             int channel)
{
    if (midi_input >= _midi_inputs || midi_input < 0 || midi_input > midi::MidiChannel::OMNI)
    {
        return MidiDispatcherStatus::INVALID_MIDI_INPUT;
    }
    auto [status, id] = _engine->processor_id_from_name(processor_name);
    if (status != engine::EngineReturnStatus::OK)
    {
        return MidiDispatcherStatus::INVALID_CHAIN_NAME;
    }
    InputConnection connection;
    connection.target = id;
    connection.parameter = 0;
    connection.min_range = 0;
    connection.max_range = 0;
    _pc_routes[midi_input][channel].push_back(connection);
    MIND_LOG_INFO("Connected program changes from MIDI port \"{}\" to processor \"{}\"", midi_input, processor_name);
    return MidiDispatcherStatus::OK;
}

MidiDispatcherStatus MidiDispatcher::connect_kb_to_track(int midi_input,
                                                         const std::string &track_name,
                                                         int channel)
{
    if (midi_input >= _midi_inputs || midi_input < 0 || midi_input > midi::MidiChannel::OMNI)
    {
        return MidiDispatcherStatus::INVALID_MIDI_INPUT;
    }
    auto [status, id] = _engine->processor_id_from_name(track_name);
    if (status != engine::EngineReturnStatus::OK)
    {
        return MidiDispatcherStatus::INVALID_CHAIN_NAME;
    }
    InputConnection connection;
    connection.target = id;
    connection.parameter = 0;
    connection.min_range = 0;
    connection.max_range = 0;
    _kb_routes_in[midi_input][channel].push_back(connection);
    MIND_LOG_INFO("Connected MIDI port \"{}\" to track \"{}\"", midi_input, track_name);
    return MidiDispatcherStatus::OK;
}

MidiDispatcherStatus MidiDispatcher::connect_raw_midi_to_track(int midi_input,
                                                               const std::string &track_name,
                                                               int channel)
{
    if (midi_input >= _midi_inputs || midi_input < 0 || midi_input > midi::MidiChannel::OMNI)
    {
        return MidiDispatcherStatus::INVALID_MIDI_INPUT;
    }
    auto [status, id] = _engine->processor_id_from_name(track_name);
    if (status != engine::EngineReturnStatus::OK)
    {
        return MidiDispatcherStatus::INVALID_CHAIN_NAME;
    }
    InputConnection connection;
    connection.target = id;
    connection.parameter = 0;
    connection.min_range = 0;
    connection.max_range = 0;
    _raw_routes_in[midi_input][channel].push_back(connection);
    MIND_LOG_INFO("Connected MIDI port \"{}\" to track \"{}\"", midi_input, track_name);
    return MidiDispatcherStatus::OK;
}

MidiDispatcherStatus MidiDispatcher::connect_track_to_output(int midi_output, const std::string &track_name, int channel)
{
    if (channel >= midi::MidiChannel::OMNI)
    {
        return MidiDispatcherStatus::INVAlID_CHANNEL;
    }
    if (midi_output >= _midi_outputs || midi_output < 0)
    {
        return MidiDispatcherStatus::INVALID_MIDI_OUTPUT;
    }
    auto[status, id] = _engine->processor_id_from_name(track_name);
    if (status != engine::EngineReturnStatus::OK)
    {
        return MidiDispatcherStatus::INVALID_CHAIN_NAME;
    }
    OutputConnection connection;
    connection.channel = channel;
    connection.output = 0;
    connection.min_range = 1.234f;
    connection.max_range = 4.5678f;
    connection.cc_number = 123;
    _kb_routes_out[id].push_back(connection);
    MIND_LOG_INFO("Connected MIDI from track \"{}\" to port \"{}\" with channel {}", midi_output, track_name, channel);
    return MidiDispatcherStatus::OK;
}

void MidiDispatcher::clear_connections()
{
    _cc_routes.clear();
    _kb_routes_in.clear();
}

void MidiDispatcher::send_midi(int input, MidiDataByte data, Time timestamp)
{
    int channel = midi::decode_channel(data);
    int size = data.size();
    /* Dispatch raw midi messages */
    {
        const auto& cons = _raw_routes_in.find(input);
        if (cons != _raw_routes_in.end())
        {
            for (auto c : cons->second[midi::MidiChannel::OMNI])
            {
                _event_dispatcher->post_event(make_wrapped_midi_event(c, data.data(), size, timestamp));
            }
            for (auto c : cons->second[channel])
            {
                _event_dispatcher->post_event(make_wrapped_midi_event(c, data.data(), size, timestamp));
            }
        }
    }

    /* Dispatch decoded midi messages */
    midi::MessageType type = midi::decode_message_type(data);
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
                    _event_dispatcher->post_event(make_param_change_event(c, decoded_msg, timestamp));
                }
                for (auto c : cons->second[decoded_msg.controller][decoded_msg.channel])
                {
                    _event_dispatcher->post_event(make_param_change_event(c, decoded_msg, timestamp));
                }
            }
            if (decoded_msg.controller == midi::MOD_WHEEL_CONTROLLER_NO)
            {
                const auto& cons = _kb_routes_in.find(input);
                if (cons != _kb_routes_in.end())
                {
                    for (auto c : cons->second[midi::MidiChannel::OMNI])
                    {
                        _event_dispatcher->post_event(make_modulation_event(c, decoded_msg, timestamp));
                    }
                    for (auto c : cons->second[decoded_msg.channel])
                    {
                        _event_dispatcher->post_event(make_modulation_event(c, decoded_msg, timestamp));
                    }
                }
            }
            break;
        }

        case midi::MessageType::NOTE_ON:
        {
            midi::NoteOnMessage decoded_msg = midi::decode_note_on(data);
            const auto& cons = _kb_routes_in.find(input);
            if (cons != _kb_routes_in.end())
            {
                for (auto c : cons->second[midi::MidiChannel::OMNI])
                {
                    _event_dispatcher->post_event(make_note_on_event(c, decoded_msg, timestamp));
                }
                for (auto c : cons->second[decoded_msg.channel])
                {
                    _event_dispatcher->post_event(make_note_on_event(c, decoded_msg, timestamp));
                }
            }
            break;
        }

        case midi::MessageType::NOTE_OFF:
        {
            midi::NoteOffMessage decoded_msg = midi::decode_note_off(data);
            const auto& cons = _kb_routes_in.find(input);
            if (cons != _kb_routes_in.end())
            {
                for (auto c : cons->second[midi::MidiChannel::OMNI])
                {
                    _event_dispatcher->post_event(make_note_off_event(c, decoded_msg, timestamp));
                }
                for (auto c : cons->second[decoded_msg.channel])
                {
                    _event_dispatcher->post_event(make_note_off_event(c, decoded_msg, timestamp));
                }
            }
            break;
        }

        case midi::MessageType::PITCH_BEND:
        {
            midi::PitchBendMessage decoded_msg = midi::decode_pitch_bend(data);
            const auto& cons = _kb_routes_in.find(input);
            if (cons != _kb_routes_in.end())
            {
                for (auto c : cons->second[midi::MidiChannel::OMNI])
                {
                    _event_dispatcher->post_event(make_pitch_bend_event(c, decoded_msg, timestamp));
                }
                for (auto c : cons->second[decoded_msg.channel])
                {
                    _event_dispatcher->post_event(make_pitch_bend_event(c, decoded_msg, timestamp));
                }
            }
            break;
        }

        case midi::MessageType::POLY_KEY_PRESSURE:
        {
            midi::PolyKeyPressureMessage decoded_msg = midi::decode_poly_key_pressure(data);
            const auto& cons = _kb_routes_in.find(input);
            if (cons != _kb_routes_in.end())
            {
                for (auto c : cons->second[midi::MidiChannel::OMNI])
                {
                    _event_dispatcher->post_event(make_note_aftertouch_event(c, decoded_msg, timestamp));
                }
                for (auto c : cons->second[decoded_msg.channel])
                {
                    _event_dispatcher->post_event(make_note_aftertouch_event(c, decoded_msg, timestamp));
                }
            }
            break;
        }

        case midi::MessageType::CHANNEL_PRESSURE:
        {
            midi::ChannelPressureMessage decoded_msg = midi::decode_channel_pressure(data);
            const auto& cons = _kb_routes_in.find(input);
            if (cons != _kb_routes_in.end())
            {
                for (auto c : cons->second[midi::MidiChannel::OMNI])
                {
                    _event_dispatcher->post_event(make_aftertouch_event(c, decoded_msg, timestamp));
                }
                for (auto c : cons->second[decoded_msg.channel])
                {
                    _event_dispatcher->post_event( make_aftertouch_event(c, decoded_msg, timestamp));
                }
            }
            break;
        }

        case midi::MessageType::PROGRAM_CHANGE:
        {
            midi::ProgramChangeMessage decoded_msg = midi::decode_program_change(data);
            const auto& cons = _pc_routes.find(input);
            if (cons != _pc_routes.end())
            {
                for (auto c : cons->second[midi::MidiChannel::OMNI])
                {
                    _event_dispatcher->post_event(make_program_change_event(c, decoded_msg, timestamp));
                }
                for (auto c : cons->second[decoded_msg.channel])
                {
                    _event_dispatcher->post_event(make_program_change_event(c, decoded_msg, timestamp));
                }
            }
            break;
        }

        default:
            break;
    }
}

int MidiDispatcher::process(Event* event)
{
    if (event->is_keyboard_event())
    {
        auto typed_event = static_cast<KeyboardEvent*>(event);
        const auto& cons = _kb_routes_out.find(typed_event->processor_id());
        if (cons != _kb_routes_out.end())
        {
            for (const OutputConnection& c : cons->second)
            {
                MidiDataByte midi_data;
                switch (typed_event->subtype())
                {
                    case KeyboardEvent::Subtype::NOTE_ON:
                        midi_data = midi::encode_note_on(c.channel, typed_event->note(), typed_event->velocity());
                        break;
                    case KeyboardEvent::Subtype::NOTE_OFF:
                        midi_data = midi::encode_note_off(c.channel, typed_event->note(), typed_event->velocity());
                        break;
                    case KeyboardEvent::Subtype::NOTE_AFTERTOUCH:
                        midi_data = midi::encode_poly_key_pressure(c.channel, typed_event->note(), typed_event->velocity());
                        break;
                    case KeyboardEvent::Subtype::AFTERTOUCH:
                        midi_data = midi::encode_channel_pressure(c.channel, typed_event->value());
                        break;
                    case KeyboardEvent::Subtype::PITCH_BEND:
                        midi_data = midi::encode_pitch_bend(c.channel, typed_event->value());
                        break;
                    case KeyboardEvent::Subtype::MODULATION:
                        midi_data = midi::encode_control_change(c.channel, midi::MOD_WHEEL_CONTROLLER_NO, typed_event->value());
                        break;
                    case KeyboardEvent::Subtype::WRAPPED_MIDI:
                        midi_data = typed_event->midi_data();
                }
                MIND_LOG_DEBUG("Dispatching midi [{:x} {:x} {:x} {:x}], timestamp: {}",
                              midi_data[0], midi_data[1], midi_data[2], midi_data[3], event->time().count());
                _frontend->send_midi(c.output, midi_data, event->time());
            }
        }
        return EventStatus::HANDLED_OK;
    }
    return EventStatus::NOT_HANDLED;
}

} // end namespace midi_dispatcher
} // end namespace sushi
