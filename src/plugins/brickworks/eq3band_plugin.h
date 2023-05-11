/*
 * Copyright 2017-2023 Elk Audio AB
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
 * @brief 3-band equalizer from Brickworks library
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef EQ3BAND_PLUGIN_H
#define EQ3BAND_PLUGIN_H

#include <bw_ls2.h>
#include <bw_hs2.h>
#include <bw_peak.h>

#include "library/internal_plugin.h"

namespace sushi {
namespace eq3band_plugin {

class Eq3bandPlugin : public InternalPlugin, public UidHelper<Eq3bandPlugin>
{
public:
    Eq3bandPlugin(HostControl hostControl);

    ~Eq3bandPlugin() = default;

    virtual ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void set_enabled(bool enabled) override;

    void set_bypassed(bool bypassed) override;

    void process_event(const RtEvent& event) override;


    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

    static std::string_view static_uid();

private:
    BypassManager _bypass_manager;
    float _sample_rate{0};

    FloatParameterValue* _lowshelf_freq;
    FloatParameterValue* _lowshelf_gain;
    FloatParameterValue* _lowshelf_q;
    FloatParameterValue* _peak_freq;
    FloatParameterValue* _peak_gain;
    FloatParameterValue* _peak_q;
    FloatParameterValue* _highshelf_freq;
    FloatParameterValue* _highshelf_gain;
    FloatParameterValue* _highshelf_q;

    bw_ls2_coeffs _ls2_coeffs;
    std::array<bw_ls2_state, MAX_TRACK_CHANNELS> _ls2_states;
    bw_peak_coeffs _peak_coeffs;
    std::array<bw_peak_state, MAX_TRACK_CHANNELS> _peak_states;
    bw_hs2_coeffs _hs2_coeffs;
    std::array<bw_hs2_state, MAX_TRACK_CHANNELS> _hs2_states;
};

}// namespace eq3band_plugin
}// namespace sushi
#endif // EQ3BAND_PLUGIN_H
