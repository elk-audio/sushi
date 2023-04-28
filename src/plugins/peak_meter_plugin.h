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
 * @brief Audio processor with event output example
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_PEAK_METER_PLUGIN_H
#define SUSHI_PEAK_METER_PLUGIN_H

#include "library/internal_plugin.h"
#include "dsp_library/value_smoother.h"

#include "engine/track.h"

namespace sushi {
namespace peak_meter_plugin {

constexpr int MAX_METERED_CHANNELS = MAX_TRACK_CHANNELS;

class PeakMeterPlugin : public InternalPlugin, public UidHelper<PeakMeterPlugin>
{
public:
    PeakMeterPlugin(HostControl host_control);

    virtual ~PeakMeterPlugin() = default;

    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void process_event(const RtEvent& event) override;

    void process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer) override;

    static std::string_view static_uid();

private:
    void _process_peak_detection(const ChunkSampleBuffer& in, bool linked, bool send_only_peaks);

    void _process_clip_detection(const ChunkSampleBuffer& in, bool linked);

    void _update_refresh_interval(float rate, float sample_rate);

    // Output parameters
    std::array<FloatParameterValue*, MAX_METERED_CHANNELS> _level_parameters;
    std::array<BoolParameterValue*, MAX_METERED_CHANNELS> _clip_parameters;

    // Input parameters
    BoolParameterValue*  _link_channels_parameter;
    BoolParameterValue*  _send_peaks_only_parameter;
    FloatParameterValue* _update_rate_parameter;
    ObjectId             _update_rate_id;

    uint64_t _clip_hold_samples;
    std::array<uint64_t, MAX_METERED_CHANNELS> _clip_hold_count;
    std::array<bool, MAX_METERED_CHANNELS> _clipped;

    int _refresh_interval;
    int _sample_count{0};
    bool _peak_hysteresis{true};

    float _sample_rate;

    std::array<ValueSmootherFilter<float>, MAX_METERED_CHANNELS> _smoothers;
};

}// namespace peak_meter_plugin
}// namespace sushi

#endif //SUSHI_PEAK_METER_PLUGIN_H
