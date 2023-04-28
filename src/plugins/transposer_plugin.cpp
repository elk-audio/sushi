/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI.  If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Midi i/o plugin example
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <algorithm>

#include "plugins/transposer_plugin.h"
#include "library/midi_decoder.h"
#include "library/midi_encoder.h"

namespace sushi {
namespace transposer_plugin {


constexpr auto PLUGIN_UID = "sushi.testing.transposer";
constexpr auto DEFAULT_LABEL = "Transposer";

constexpr int MAX_NOTE = 127;
constexpr int MIN_NOTE = 0;

TransposerPlugin::TransposerPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);
    _transpose_parameter = register_float_parameter("transpose",
                                                    "Transpose",
                                                    "semitones",
                                                    0.0f,
                                                    -24.0f,
                                                    24.0f,
                                                    Direction::AUTOMATABLE,
                                                    new FloatParameterPreProcessor(-24.0f, 24.0f) );
    assert(_transpose_parameter);
    _max_input_channels = 0;
    _max_output_channels = 0;
}

ProcessorReturnCode TransposerPlugin::init(float /*sample_rate*/)
{
    return ProcessorReturnCode::OK;
}

void TransposerPlugin::process_event(const RtEvent& event)
{
    switch (event.type())
    {
        case RtEventType::NOTE_ON:
        {
            auto typed_event = event.keyboard_event();
            output_event(RtEvent::make_note_on_event(typed_event->processor_id(),
                                                     typed_event->sample_offset(),
                                                     typed_event->channel(),
                                                     _transpose_note(typed_event->note()),
                                                     typed_event->velocity()));
            break;
        }

        case RtEventType::NOTE_OFF:
        {
            auto typed_event = event.keyboard_event();
            output_event(RtEvent::make_note_off_event(typed_event->processor_id(),
                                                      typed_event->sample_offset(),
                                                      typed_event->channel(),
                                                      _transpose_note(typed_event->note()),
                                                      typed_event->velocity()));
            break;
        }

        case RtEventType::WRAPPED_MIDI_EVENT:
        {
            auto typed_event = event.wrapped_midi_event();
            output_event(RtEvent::make_wrapped_midi_event(typed_event->processor_id(),
                                                          typed_event->sample_offset(),
                                                          _transpose_midi(typed_event->midi_data())));
            break;
        }

        default:
            // Parameter changes are handled by the default implementation
            InternalPlugin::process_event(event);
            break;
    }
}

int TransposerPlugin::_transpose_note(int note)
{
    int steps = _transpose_parameter->processed_value();
    return std::clamp(note + steps, MIN_NOTE, MAX_NOTE);
}

MidiDataByte TransposerPlugin::_transpose_midi(MidiDataByte midi_msg)
{
    auto type = midi::decode_message_type(midi_msg);
    switch (type)
    {
        case midi::MessageType::NOTE_ON:
        {
            auto note_on_msg = midi::decode_note_on(midi_msg);
            return midi::encode_note_on(note_on_msg.channel, _transpose_note(note_on_msg.note), note_on_msg.velocity);
        }
        case midi::MessageType::NOTE_OFF:
        {
            auto note_off_msg = midi::decode_note_off(midi_msg);
            return midi::encode_note_off(note_off_msg.channel, _transpose_note(note_off_msg.note), note_off_msg.velocity);
        }
        default:
            return midi_msg;
    }
}

void TransposerPlugin::process_audio(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer)
{
    bypass_process(in_buffer, out_buffer);
}

std::string_view TransposerPlugin::static_uid()
{
    return PLUGIN_UID;
}

}// namespace transposer_plugin
}// namespace sushi
