/*
 * Copyright 2017-2023 Elk Audio AB, Stockholm
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
 * @brief Simple monophonic synthesizer from Brickworks library
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include <cassert>

#include <bw_buf.h>

#include "simple_synth_plugin.h"

namespace sushi {
namespace simple_synth_plugin {

constexpr auto PLUGIN_UID = "sushi.brickworks.simple_synth";
constexpr auto DEFAULT_LABEL = "Simple synthesizer";

constexpr float A4_FREQUENCY = 440.0f;
constexpr int A4_NOTENUM = 69;
constexpr float NOTE2FREQ_SCALE = 5.0f / 60.0f;

SimpleSynthPlugin::SimpleSynthPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);

    _volume = register_float_parameter("volume", "Volume", "dB",
                                       0.0f, -60.0f, 12.0f,
                                       Direction::AUTOMATABLE,
                                       new dBToLinPreProcessor(-60.0f, 36.0f));
    _portamento = register_float_parameter("portamento", "Portamento time", "sec",
                                           0.0f, 0.0f, 1.0f,
                                           Direction::AUTOMATABLE,
                                           new FloatParameterPreProcessor(0.0f, 1.0f));
    _pulse_width = register_float_parameter("pulse_width", "Pulse width", "",
                                            0.5f, 0.0f, 1.0f,
                                            Direction::AUTOMATABLE,
                                            new FloatParameterPreProcessor(0.0f, 1.0f));
    _filter_cutoff = register_float_parameter("filter_cutoff", "Filter cutoff", "Hz",
                                              4'000.0f, 20.0f, 20'000.0f,
                                              Direction::AUTOMATABLE,
                                              new CubicWarpPreProcessor(20.0f, 20'000.0f));
    _filter_Q = register_float_parameter("filter_Q", "Filter Q", "",
                                         1.0f, 0.5f, 10.0f,
                                         Direction::AUTOMATABLE,
                                         new FloatParameterPreProcessor(0.5f, 10.0f));
    _attack = register_float_parameter("attack", "Attack time", "sec",
                                       0.01f, 0.0f, 1.0f,
                                       Direction::AUTOMATABLE,
                                       new FloatParameterPreProcessor(0.0f, 1.0f));
    _decay = register_float_parameter("decay", "Decay time", "sec",
                                      0.01f, 0.0f, 1.0f,
                                      Direction::AUTOMATABLE,
                                      new FloatParameterPreProcessor(0.0f, 1.0f));
    _sustain = register_float_parameter("sustain", "Sustain level", "",
                                        1.0f, 0.0f, 1.0f,
                                        Direction::AUTOMATABLE,
                                        new FloatParameterPreProcessor(0.0f, 1.0f));
    _release = register_float_parameter("release", "Release time", "sec",
                                        0.01f, 0.0f, 1.0f,
                                        Direction::AUTOMATABLE,
                                        new FloatParameterPreProcessor(0.0f, 1.0f));

    _max_input_channels = 0;
};

ProcessorReturnCode SimpleSynthPlugin::init(float sample_rate)
{
	bw_phase_gen_init(&_phase_gen_coeffs);
	bw_osc_pulse_init(&_osc_pulse_coeffs);
	bw_svf_init(&_svf_coeffs);
	bw_env_gen_init(&_env_gen_coeffs);

	bw_osc_pulse_set_antialiasing(&_osc_pulse_coeffs, 1);

    configure(sample_rate);

    return ProcessorReturnCode::OK;
}

void SimpleSynthPlugin::configure(float sample_rate)
{
	bw_phase_gen_set_sample_rate(&_phase_gen_coeffs, sample_rate);
	bw_osc_pulse_set_sample_rate(&_osc_pulse_coeffs, sample_rate);
	bw_svf_set_sample_rate(&_svf_coeffs, sample_rate);
	bw_env_gen_set_sample_rate(&_env_gen_coeffs, sample_rate);

    return;
}

void SimpleSynthPlugin::set_enabled(bool enabled)
{
    Processor::set_enabled(enabled);
	bw_phase_gen_reset_coeffs(&_phase_gen_coeffs);
	bw_phase_gen_reset_state(&_phase_gen_coeffs, &_phase_gen_state, 0.0f);
	bw_osc_pulse_reset_coeffs(&_osc_pulse_coeffs);
	bw_osc_filt_reset_state(&_osc_filt_state);
	bw_svf_reset_coeffs(&_svf_coeffs);
	bw_svf_reset_state(&_svf_coeffs, &_svf_state, 0.0f);
	bw_env_gen_reset_coeffs(&_env_gen_coeffs);
	bw_env_gen_reset_state(&_env_gen_coeffs, &_env_gen_state);
    _active_note = -1;
    _start_offset = 0;
    _stop_offset = AUDIO_CHUNK_SIZE;
}


void SimpleSynthPlugin::process_event(const RtEvent& event)
{
    switch (event.type())
    {
        case RtEventType::NOTE_ON:
        {
            if (_bypassed)
            {
                break;
            }

            auto key_event = event.keyboard_event();
            _active_note = key_event->note();
            _gate = true;
            _start_offset = event.sample_offset();
            break;
        }
        case RtEventType::NOTE_OFF:
        {
            if (_bypassed)
            {
                break;
            }
            auto key_event = event.keyboard_event();
            if (key_event->note() == _active_note)
            {
                _stop_offset = event.sample_offset();
            }
            break;
        }

        case RtEventType::NOTE_AFTERTOUCH:
        case RtEventType::PITCH_BEND:
        case RtEventType::AFTERTOUCH:
        case RtEventType::MODULATION:
        case RtEventType::WRAPPED_MIDI_EVENT:
            // Consume these events so they are not propagated
            break;

        default:
            InternalPlugin::process_event(event);
            break;
    }
}


void SimpleSynthPlugin::process_audio(const ChunkSampleBuffer& /* in_buffer */, ChunkSampleBuffer& out_buffer)
{
    out_buffer.clear();
    bw_phase_gen_set_portamento_tau(&_phase_gen_coeffs, _portamento->processed_value());
    bw_osc_pulse_set_pulse_width(&_osc_pulse_coeffs, _pulse_width->processed_value());
    bw_svf_set_cutoff(&_svf_coeffs, _filter_cutoff->processed_value());
    bw_svf_set_Q(&_svf_coeffs, _filter_Q->processed_value());
    bw_env_gen_set_attack(&_env_gen_coeffs, _attack->processed_value());
    bw_env_gen_set_decay(&_env_gen_coeffs, _decay->processed_value());
    bw_env_gen_set_sustain(&_env_gen_coeffs, _sustain->processed_value());
    bw_env_gen_set_release(&_env_gen_coeffs, _release->processed_value());

    _render_loop(0, _start_offset);
    if (_gate)
    {
        bw_env_gen_set_gate(&_env_gen_coeffs, 1);
        // the Brickworks example also had a master tuning parameter to change the 440.0Hz, skipped here
        float note_freq = A4_FREQUENCY * bw_pow2f_3(NOTE2FREQ_SCALE * (_active_note - A4_NOTENUM));
		bw_phase_gen_set_frequency(&_phase_gen_coeffs, note_freq);
    }
    else
    {
        bw_env_gen_set_gate(&_env_gen_coeffs, 0);
    }
    _render_loop(_start_offset, (_stop_offset - _start_offset));
    // the following is only met if we received a NoteOFF event
    if (_stop_offset < AUDIO_CHUNK_SIZE)
    {
        bw_env_gen_set_gate(&_env_gen_coeffs, 0);
        _gate = false;
        _render_loop(_stop_offset, (AUDIO_CHUNK_SIZE - _stop_offset));
        _stop_offset = AUDIO_CHUNK_SIZE;
    }

    if (!_bypassed)
    {
        float gain = _volume->processed_value();
        out_buffer.add_with_gain(_render_buffer, gain);
    }
}

void SimpleSynthPlugin::_render_loop(int offset, int n)
{
    float* out = &_render_buffer.channel(0)[offset];
    float* buf = &_aux_buffer.channel(0)[offset];

    bw_phase_gen_process(&_phase_gen_coeffs, &_phase_gen_state, nullptr, out, buf, n);
    bw_osc_pulse_process(&_osc_pulse_coeffs, out, buf, out, n);
    bw_osc_filt_process(&_osc_filt_state, out, out, n);
    bw_svf_process(&_svf_coeffs, &_svf_state, out, out, nullptr, nullptr, n);
    bw_env_gen_process(&_env_gen_coeffs, &_env_gen_state, buf, n);
    bw_buf_mul(out, out, buf, n);
}

std::string_view SimpleSynthPlugin::static_uid()
{
    return PLUGIN_UID;
}

}// namespace simple_synth_plugin
}// namespace sushi
