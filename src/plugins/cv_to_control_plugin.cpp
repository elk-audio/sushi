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
* @brief Adapter plugin to convert cv/gate information to note on and note
*        off messages to enable cv/gate control of synthesizer plugins.
* @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
*/

#include <algorithm>
#include <cmath>

#include "plugins/cv_to_control_plugin.h"

namespace sushi {
namespace cv_to_control_plugin {

constexpr auto PLUGIN_UID = "sushi.testing.cv_to_control";
constexpr auto DEFAULT_LABEL = "Cv to control adapter";
constexpr int TUNE_RANGE = 24;
constexpr float PITCH_BEND_RANGE = 12.0f;

CvToControlPlugin::CvToControlPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);
    _pitch_bend_mode_parameter = register_bool_parameter("pitch_bend_enabled", "Pitch bend enabled", "", false, Direction::AUTOMATABLE);
    _velocity_mode_parameter = register_bool_parameter("velocity_enabled", "Velocity enabled", "", false, Direction::AUTOMATABLE);

    _channel_parameter  = register_int_parameter("channel", "Channel", "",
                                                 0, 0, 16,
                                                 Direction::AUTOMATABLE,
                                                 new IntParameterPreProcessor(0, 16));

    _coarse_tune_parameter  = register_int_parameter("tune", "Tune", "semitones",
                                                     0, -TUNE_RANGE, TUNE_RANGE,
                                                     Direction::AUTOMATABLE,
                                                     new IntParameterPreProcessor(-TUNE_RANGE, TUNE_RANGE));

    _polyphony_parameter  = register_int_parameter("polyphony", "Polyphony", "",
                                                   1, 1, MAX_CV_VOICES,
                                                   Direction::AUTOMATABLE,
                                                   new IntParameterPreProcessor(1, MAX_CV_VOICES));

    assert(_pitch_bend_mode_parameter && _velocity_mode_parameter && _channel_parameter &&
                                           _coarse_tune_parameter && _polyphony_parameter);

    for (int i = 0; i < MAX_CV_VOICES; ++i)
    {
        auto i_str = std::to_string(i);
        _pitch_parameters[i] = register_float_parameter("pitch_" + i_str, "Pitch " + i_str, "semitones",
                                                        0.0f, 0.0f, 1.0f,
                                                        Direction::AUTOMATABLE,
                                                        new FloatParameterPreProcessor(0.0f, 1.0f));

        _velocity_parameters[i] = register_float_parameter("velocity_" + i_str, "Velocity " + i_str, "",
                                                           0.5f, 0.0f, 1.0f,
                                                           Direction::AUTOMATABLE,
                                                           new FloatParameterPreProcessor(0.0f, 1.0f));

        assert(_pitch_parameters[i] && _velocity_parameters[i]);
    }

    _max_input_channels = 0;
    _max_output_channels = 0;
    _deferred_note_offs.reserve(MAX_ENGINE_CV_IO_PORTS);
}

ProcessorReturnCode CvToControlPlugin::init(float sample_rate)
{
    return Processor::init(sample_rate);
}

void CvToControlPlugin::configure(float sample_rate)
{
    Processor::configure(sample_rate);
}

void CvToControlPlugin::process_event(const RtEvent& event)
{
    // Plugin listens to all channels
    if (event.type() == RtEventType::NOTE_ON || event.type() == RtEventType::NOTE_OFF)
    {
        _gate_events.push(event);
        return;
    }

    InternalPlugin::process_event(event);
}

void CvToControlPlugin::process_audio(const ChunkSampleBuffer&  /*in_buffer*/, ChunkSampleBuffer& /*out_buffer*/)
{
    if (_bypassed == true)
    {
        _gate_events.clear();
        return;
    }

    bool send_pitch_bend = _pitch_bend_mode_parameter->processed_value();
    bool send_velocity = _velocity_mode_parameter->processed_value();
    int  channel = _channel_parameter->processed_value();
    int  tune = _coarse_tune_parameter->processed_value();
    int  polyphony = _polyphony_parameter->processed_value();

    _send_deferred_events(channel);
    _process_cv_signals(polyphony, channel, tune, send_velocity, send_pitch_bend);
    _process_gate_changes(polyphony, channel, tune, send_velocity, send_pitch_bend);
}


std::string_view CvToControlPlugin::static_uid()
{
    return PLUGIN_UID;
}

void CvToControlPlugin::_send_deferred_events(int channel)
{
    // Note offs that are deferred to create overlapping notes
    for (auto note : _deferred_note_offs)
    {
        output_event(RtEvent::make_note_off_event(0, 0, channel, note, 1.0f));
    }
    _deferred_note_offs.clear();
}

void CvToControlPlugin::_process_cv_signals(int polyphony, int channel, int tune, bool send_velocity, bool send_pitch_bend)
{
    if (send_pitch_bend && polyphony == 1)
    {
        if (_voices[0].active)
        {
            /* For now, sending pitch bend only makes sense for monophonic control
               Eventually add a mode that sends every voice on a separate channel */
            auto[note, fraction] = cv_to_pitch(_pitch_parameters[0]->processed_value());
            note += tune;
            float note_diff = std::clamp((note - _voices[0].note + fraction) / PITCH_BEND_RANGE, -1.0f, 1.0f);
            output_event(RtEvent::make_pitch_bend_event(0, 0, channel, note_diff));
        }
    }
    else
    {
        for (int i = 0; i < polyphony && i < static_cast<int>(_voices.size()); ++i)
        {
            auto& voice = _voices[i];
            if (voice.active)
            {
                int new_note;
                std::tie(new_note, std::ignore) = cv_to_pitch(_pitch_parameters[i]->processed_value());
                if (voice.note != new_note)
                {
                    _deferred_note_offs.push_back(voice.note);
                    voice.note = new_note;
                    float velocity = send_velocity ? _velocity_parameters[i]->processed_value() : 1.0f;
                    output_event(RtEvent::make_note_on_event(0, 0, channel, new_note, velocity));
                }
            }
        }
    }
}

void CvToControlPlugin::_process_gate_changes(int polyphony, int channel, int tune, bool send_velocity, bool send_pitch_bend)
{
    while (_gate_events.empty() == false)
    {
        const auto& event = _gate_events.pop();
        const auto kbd_event = event.keyboard_event();
        int gate = kbd_event->note();
        bool gate_state = kbd_event->type() == RtEventType::NOTE_ON;
        if (gate < polyphony && gate >= 0)
        {
            if (gate_state) // Gate high
            {
                float velocity = send_velocity ? _velocity_parameters[gate]->processed_value() : 1.0f;
                _voices[gate].active = true;
                auto [note, fraction] = cv_to_pitch(_pitch_parameters[gate]->processed_value());
                note += tune;
                _voices[gate].note = note;
                output_event(RtEvent::make_note_on_event(0, 0, channel, note, velocity));
                if (send_pitch_bend)
                {
                    output_event(RtEvent::make_pitch_bend_event(0, 0, channel, fraction / PITCH_BEND_RANGE));
                }
            }
            else // Gate low
            {
                _voices[gate].active = false;
                output_event(RtEvent::make_note_off_event(0, 0, channel, _voices[gate].note, 1.0f));
            }
        }
    }
}

std::pair<int, float> cv_to_pitch(float value)
{
    // TODO - this need a lot of tuning, or maybe that tuning should be done someplace else in the code?
    // Currently just assuming [0, 1] covers a 10 octave linear range.
    double int_note;
    double fraction = modf(value * 120.0f , &int_note);
    return {static_cast<int>(int_note), static_cast<float>(fraction)};
}
}// namespace cv_to_control_plugin
}// namespace sushi