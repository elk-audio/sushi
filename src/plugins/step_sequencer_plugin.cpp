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
 * @brief Simple 8-step sequencer example.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <algorithm>

#include "plugins/step_sequencer_plugin.h"

namespace sushi {
namespace step_sequencer_plugin {

constexpr auto PLUGIN_UID = "sushi.testing.step_sequencer";
constexpr auto DEFAULT_LABEL = "Step Sequencer";

constexpr float SECONDS_IN_MINUTE = 60.0f;
constexpr float MULTIPLIER_8TH_NOTE = 2.0f;
constexpr int   OCTAVE = 12;
constexpr std::array<int, 12> MINOR_SCALE = {0,0,2,3,3,5,5,7,8,8,10,10};

StepSequencerPlugin::StepSequencerPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);
    for (int i = 0; i < SEQUENCER_STEPS; ++i)
    {
        std::string str_nr = std::to_string(i);
        _pitch_parameters[i] = register_int_parameter("pitch_" + str_nr, "Pitch " + str_nr, "semitone",
                                                      0, -24, 24,
                                                      Direction::AUTOMATABLE,
                                                      new IntParameterPreProcessor(-24, 24));

        _step_parameters[i] = register_bool_parameter("step_" + str_nr, "Step " + str_nr, "",
                                                      true, Direction::AUTOMATABLE);

        _step_indicator_parameters[i] = register_bool_parameter("step_ind_" + str_nr, "Step Indication " + str_nr, "",
                                                                true, Direction::AUTOMATABLE);

        assert(_pitch_parameters[i] && _step_indicator_parameters[i] && _step_indicator_parameters[i]);
    }
    for (auto& s : _sequence)
    {
        s = START_NOTE;
    }
}

ProcessorReturnCode StepSequencerPlugin::init(float sample_rate)
{
    _sample_rate = sample_rate;
    return ProcessorReturnCode::OK;
}

void StepSequencerPlugin::configure(float sample_rate)
{
    _sample_rate = sample_rate;
}

void StepSequencerPlugin::set_bypassed(bool bypassed)
{
    Processor::set_bypassed(bypassed);
}

void StepSequencerPlugin::process_event(const RtEvent& event)
{
    switch (event.type())
    {
        case RtEventType::NOTE_ON:
        {
            auto typed_event = event.keyboard_event();
            _transpose = typed_event->note() - START_NOTE;
            _event_queue.push(event);
            break;
        }

        case RtEventType::NOTE_OFF:
        case RtEventType::MODULATION:
        case RtEventType::PITCH_BEND:
        case RtEventType::AFTERTOUCH:
        case RtEventType::WRAPPED_MIDI_EVENT:
        {
            _event_queue.push(event);
            break;
        }
        case RtEventType::FLOAT_PARAMETER_CHANGE:
        case RtEventType::INT_PARAMETER_CHANGE:
        case RtEventType::BOOL_PARAMETER_CHANGE:
        {
            auto typed_event = event.parameter_change_event();
            /* Ugly way of identifying a step parameter change and outputing an indicator param update*/
            if (typed_event->param_id() % 3 == 1)
            {
                set_parameter_and_notify(_step_indicator_parameters[typed_event->param_id()/3], typed_event->value() > 0.5f);
            }
            [[fallthrough]];
        }
        default:
            InternalPlugin::process_event(event);
    }
}

void StepSequencerPlugin::process_audio(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer)
{
    bypass_process(in_buffer, out_buffer);
    if (_host_control.transport()->playing_mode() == PlayingMode::STOPPED)
    {
        // If not playing, we pass keyboard events through unchanged.
        while (_event_queue.empty() == false)
        {
            output_event(_event_queue.pop());
        }
        // If stopping, we kill the current note playing.
        if (_host_control.transport()->current_state_change() == PlayStateChange::STOPPING)
        {
            output_event(RtEvent::make_note_off_event(this->id(), 0, 0, _current_note, 1.0f));
        }
        return;
    }

    float start_beat = _host_control.transport()->current_bar_beats() * MULTIPLIER_8TH_NOTE;
    float end_beat = _host_control.transport()->current_bar_beats(AUDIO_CHUNK_SIZE) * MULTIPLIER_8TH_NOTE;

    /* New 8th note during this chunk */
    if (static_cast<int>(end_beat) - static_cast<int>(start_beat) != 0)
    {
        int step = static_cast<int>(end_beat);
        if (step >= SEQUENCER_STEPS)
        {
            return;
        }
        int offset = static_cast<int>((end_beat - std::floor(end_beat)) /
                samples_per_qn(_host_control.transport()->current_tempo(), _sample_rate));
        if (_current_step_active)
        {
            RtEvent note_off = RtEvent::make_note_off_event(this->id(), offset, 0, _current_note, 1.0f);
            output_event(note_off);
        }

        /* Indicator for the current step is turned off if the current step is active and
         * vice versa in order to provide visual feedback when the sequencer is running */
        set_parameter_and_notify(_step_indicator_parameters[_current_step], _current_step_active);
        _current_step = step;
        _current_step_active = _step_parameters[step]->processed_value();
        set_parameter_and_notify(_step_indicator_parameters[step], !_current_step_active);

        if (_current_step_active)
        {
            _current_note = snap_to_scale(_pitch_parameters[step]->processed_value() + START_NOTE, MINOR_SCALE) + _transpose;
            RtEvent note_on = RtEvent::make_note_on_event(this->id(), offset, 0, _current_note, 1.0f);
            output_event(note_on);
        }
    }

    _event_queue.clear();
}

std::string_view StepSequencerPlugin::static_uid()
{
    return PLUGIN_UID;
}

float samples_per_qn(float tempo, float samplerate)
{
    return 8.0f * samplerate / tempo * SECONDS_IN_MINUTE;
}

int snap_to_scale(int note, const std::array<int, 12>& scale)
{
    int octave = note / OCTAVE;
    return scale[note % OCTAVE] + octave * OCTAVE;
}
}// namespace step_sequencer_plugin
}// namespace sushi
