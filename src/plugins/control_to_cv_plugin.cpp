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
 * @brief Adapter plugin to convert from note on and note
 *        off messages, to cv/gate information, to enable cv/gate control from MIDI plugins.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <algorithm>

#include "plugins/control_to_cv_plugin.h"

namespace sushi {
namespace control_to_cv_plugin {

constexpr auto PLUGIN_UID = "sushi.testing.control_to_cv";
constexpr auto DEFAULT_LABEL = "Keyboard control to CV adapter";
constexpr int TUNE_RANGE = 24;
constexpr float PITCH_BEND_RANGE = 12.0f;
constexpr int SEND_CHANNEL = 0;

ControlToCvPlugin::ControlToCvPlugin(HostControl host_control) :  InternalPlugin(host_control)
{
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);

    _send_velocity_parameter = register_bool_parameter("send_velocity", "Send Velocity", "", false, Direction::AUTOMATABLE);
    _send_modulation_parameter = register_bool_parameter("send_modulation", "Send Modulation", "", false, Direction::AUTOMATABLE);
    _retrigger_mode_parameter = register_bool_parameter("retrigger_enabled", "Retrigger enabled", "", false, Direction::AUTOMATABLE);

    _coarse_tune_parameter  = register_int_parameter("tune", "Tune", "semitones",
                                                     0, -TUNE_RANGE, TUNE_RANGE,
                                                     Direction::AUTOMATABLE,
                                                     new IntParameterPreProcessor(-24, 24));

    _fine_tune_parameter  = register_float_parameter("fine_tune", "Fine Tune", "semitone",
                                                     0.0f, -1.0f, 1.0f,
                                                     Direction::AUTOMATABLE,
                                                     new FloatParameterPreProcessor(-1, 1));

    _polyphony_parameter  = register_int_parameter("polyphony", "Polyphony", "",
                                                   1, 1, MAX_CV_VOICES,
                                                   Direction::AUTOMATABLE,
                                                   new IntParameterPreProcessor(1, MAX_CV_VOICES));

    _modulation_parameter  = register_float_parameter("modulation", "Modulation", "",
                                                      0.0f, -1.0f, 1.0f,
                                                      Direction::AUTOMATABLE,
                                                      new FloatParameterPreProcessor(-1, 1));

    assert(_send_velocity_parameter && _send_modulation_parameter && _coarse_tune_parameter &&
                                             _polyphony_parameter && _modulation_parameter);

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
}

ProcessorReturnCode ControlToCvPlugin::init(float sample_rate)
{
    return Processor::init(sample_rate);
}

void ControlToCvPlugin::configure(float sample_rate)
{
    Processor::configure(sample_rate);
}

void ControlToCvPlugin::process_event(const RtEvent& event)
{
    if (is_keyboard_event(event))
    {
        _kb_events.push(event);
        return;
    }
    InternalPlugin::process_event(event);
}

void ControlToCvPlugin::process_audio(const ChunkSampleBuffer&  /*in_buffer*/, ChunkSampleBuffer& /*out_buffer*/)
{
    if (_bypassed == true)
    {
        _kb_events.clear();
        return;
    }

    bool send_velocity = _send_velocity_parameter->processed_value();
    bool send_modulation = _send_modulation_parameter->processed_value();
    bool retrigger_mode = _retrigger_mode_parameter->processed_value();
    int coarse_tune = _coarse_tune_parameter->processed_value();
    float fine_tune = _fine_tune_parameter->processed_value();
    int polyphony = _polyphony_parameter->processed_value();

    _send_deferred_events();
    _parse_events(retrigger_mode, polyphony);
    _send_cv_signals(coarse_tune + fine_tune + _pitch_bend_value, polyphony, send_velocity, send_modulation);
}

void ControlToCvPlugin::_send_deferred_events()
{
    while (_deferred_gate_highs.empty() == false)
    {
        auto gate_id = _deferred_gate_highs.pop();
        maybe_output_gate_event(SEND_CHANNEL, gate_id, true);
    }
    _deferred_gate_highs.clear();
}

void ControlToCvPlugin::_parse_events(bool retrigger, int polyphony)
{
    while (_kb_events.empty() == false)
    {
        auto event = _kb_events.pop();
        switch (event.type())
        {
            case RtEventType::NOTE_ON:
            {
                auto typed_event = event.keyboard_event();
                int voice_id = get_free_voice_id(polyphony);
                auto& voice = _voices[voice_id];
                if (retrigger && voice.active)
                {
                    // Send the gate low event now, and send the gate high event in the next buffer
                    maybe_output_gate_event(SEND_CHANNEL, voice_id, false);
                    _deferred_gate_highs.push(voice_id);
                }
                else if (voice.active == false)
                {
                    maybe_output_gate_event(SEND_CHANNEL, voice_id, true);
                }
                voice.active = true;
                voice.note = typed_event->note();
                voice.velocity = typed_event->velocity();
                break;
            }

            case RtEventType::NOTE_OFF:
            {
                auto typed_event = event.keyboard_event();
                for (int i = 0; i < polyphony; ++i)
                {
                    auto& voice = _voices[i];
                    if (voice.note == typed_event->note() && voice.active)
                    {
                        maybe_output_gate_event(SEND_CHANNEL, i, false);
                        voice.active = false; // TODO - should we handle release velocity, should it be optional?
                    }
                }
                break;
            }

            case RtEventType::PITCH_BEND:
            {
                auto typed_event = event.keyboard_common_event();
                _pitch_bend_value = typed_event->value() * PITCH_BEND_RANGE;
                break;
            }
            case RtEventType::MODULATION:
            {
                auto typed_event = event.keyboard_common_event();
                _modulation_value = typed_event->value();
                break;
            }

            default:
                break;
        }
    }
    _kb_events.clear();
}

void ControlToCvPlugin::_send_cv_signals(float tune_offset, int polyphony, bool send_velocity, bool send_modulation)
{
    // As notes have a non-zero decay, pitch matters even if gate is off, hence always send pitch on all notes
    for (int i = 0; i < polyphony; ++i)
    {
        set_parameter_and_notify(_pitch_parameters[i], pitch_to_cv(_voices[i].note + tune_offset));
    }
    if (send_velocity)
    {
        for (int i = 0; i < polyphony; ++i)
        {
            set_parameter_and_notify(_velocity_parameters[i], _voices[i].velocity);
        }
    }
    if (send_modulation)
    {
        set_parameter_and_notify(_modulation_parameter, _modulation_value);
    }
}

int ControlToCvPlugin::get_free_voice_id(int polyphony)
{
    assert(polyphony <= MAX_CV_VOICES);
    int voice_id = 0;
    if (polyphony > 1)
    {
        while (voice_id < polyphony && _voices[voice_id].active)
        {
            voice_id++;
        }
        if (voice_id >= polyphony) // We need to steal an active note, pick the last
        {
            voice_id = _last_voice;
            _last_voice = (_last_voice + 1) % polyphony;
        }
        else
        {
            _last_voice = 0;
        }
    }
    return voice_id;
}

std::string_view ControlToCvPlugin::static_uid()
{
    return PLUGIN_UID;
}

float pitch_to_cv(float value)
{
    // TODO - this need a lot of tuning, or maybe that tuning should be done someplace else in the code?
    // Currently just assuming [0, 1] covers a 10 octave linear range.
    return std::clamp(value / 120.f, 0.0f, 1.0f);
}
}// namespace control_to_cv_plugin
}// namespace sushi