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

#ifndef SUSHI_SIMPLE_SYNTH_PLUGIN_H
#define SUSHI_SIMPLE_SYNTH_PLUGIN_H

#include <bw_math.h>
#include <bw_phase_gen.h>
#include <bw_osc_pulse.h>
#include <bw_osc_filt.h>
#include <bw_svf.h>
#include <bw_env_gen.h>
#include <bw_gain.h>

#include "library/internal_plugin.h"

namespace sushi {
namespace simple_synth_plugin {


class SimpleSynthPlugin : public InternalPlugin, public UidHelper<SimpleSynthPlugin>
{
public:
    SimpleSynthPlugin(HostControl host_control);

    ~SimpleSynthPlugin() = default;

    virtual ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void set_enabled(bool enabled) override;

    void process_event(const RtEvent& event) override ;

    void process_audio(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer) override;

    static std::string_view static_uid();

private:
    void _render_loop(int offset, int n);

    ChunkSampleBuffer _render_buffer{1};
    ChunkSampleBuffer _aux_buffer{1};

    FloatParameterValue* _volume;
    FloatParameterValue* _portamento;
    FloatParameterValue* _pulse_width;
    FloatParameterValue* _filter_cutoff;
    FloatParameterValue* _filter_Q;
    FloatParameterValue* _attack;
    FloatParameterValue* _decay;
    FloatParameterValue* _sustain;
    FloatParameterValue* _release;

    bool _gate;
    int _active_note{0};
    int _start_offset{0};
    int _stop_offset{AUDIO_CHUNK_SIZE};

	bw_phase_gen_coeffs	_phase_gen_coeffs;
	bw_phase_gen_state	_phase_gen_state;
	bw_osc_pulse_coeffs	_osc_pulse_coeffs;
	bw_osc_filt_state	_osc_filt_state;
	bw_svf_coeffs		_svf_coeffs;
	bw_svf_state		_svf_state;
	bw_env_gen_coeffs	_env_gen_coeffs;
	bw_env_gen_state	_env_gen_state;
};

}// namespace simple_synth_plugin
}// namespace sushi

#endif //SUSHI_SIMPLE_SYNTH_PLUGIN_H
