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

#include <cassert>

#include "spdlog/fmt/bundled/format.h"

#include "peak_meter_plugin.h"

namespace sushi {
namespace peak_meter_plugin {

constexpr float DEFAULT_REFRESH_RATE = 25;
// The time for meters to drop ~10dB
constexpr auto REFRESH_TIME = std::chrono::milliseconds(250);

constexpr auto CLIP_HOLD_TIME = std::chrono::seconds(5);

// Full range of the output parameters is -120dB to +24dB
constexpr float OUTPUT_MIN_DB = -120.0f;
constexpr float OUTPUT_MAX_DB = 24.0f;
constexpr float OUTPUT_MIN = 1.0e-6f; // -120dB

constexpr auto PLUGIN_UID = "sushi.testing.peakmeter";
constexpr auto DEFAULT_LABEL = "Peak Meter";

// Convert a gain value to a normalised gain value
inline float to_normalised_dB(float gain)
{
    float db_gain = 20.0f * std::log10(std::max(gain, OUTPUT_MIN));
    float norm = (db_gain - OUTPUT_MIN_DB) / (OUTPUT_MAX_DB - OUTPUT_MIN_DB);
    return std::clamp(norm, 0.0f, 1.0f);
}

PeakMeterPlugin::PeakMeterPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    _clip_hold_count.fill(0.0);
    _clipped.fill(false);
    _max_input_channels = MAX_METERED_CHANNELS;
    _max_output_channels = MAX_METERED_CHANNELS;
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);

    _link_channels_parameter = register_bool_parameter("link_channels", "Link Channels 1 & 2", "", false, Direction::AUTOMATABLE);
    _send_peaks_only_parameter = register_bool_parameter("peaks_only", "Peaks Only", "", false, Direction::AUTOMATABLE);
    _update_rate_parameter = register_float_parameter("update_rate", "Update Rate", "/s", DEFAULT_REFRESH_RATE,
                                                      0.1, 25,
                                                      Direction::AUTOMATABLE,
                                                      new FloatParameterPreProcessor(0.1, DEFAULT_REFRESH_RATE));
    _update_rate_id = _update_rate_parameter->descriptor()->id();

    std::string param_name = "level_{}";
    std::string param_label = "Level ch {}";
    for (int i = 0; i < MAX_METERED_CHANNELS; ++i)
    {
        _level_parameters[i] = register_float_parameter(fmt::format(param_name, i), fmt::format(param_label, i), "dB",
                                                        OUTPUT_MIN_DB, OUTPUT_MIN_DB, OUTPUT_MAX_DB,
                                                        Direction::OUTPUT,
                                                        new dBToLinPreProcessor(OUTPUT_MIN_DB, OUTPUT_MAX_DB));
        assert (_level_parameters[i]);
    }

    param_name = "clip_{}";
    param_label = "Clip ch {}";
    for (int i = 0; i < MAX_METERED_CHANNELS; ++i)
    {
        _clip_parameters[i] = register_bool_parameter(fmt::format(param_name, i),
                                                      fmt::format(param_label, i),
                                                      "",
                                                      false,
                                                      Direction::OUTPUT);
    }

    assert(_link_channels_parameter && _send_peaks_only_parameter && _update_rate_parameter);
}

void PeakMeterPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    bypass_process(in_buffer, out_buffer);

    bool linked = _link_channels_parameter->processed_value();
    bool send_only_peaks = _send_peaks_only_parameter->processed_value();
    _process_peak_detection(in_buffer, linked, send_only_peaks);
    _process_clip_detection(in_buffer, linked);
}

ProcessorReturnCode PeakMeterPlugin::init(float sample_rate)
{
    _sample_rate = sample_rate;
    _update_refresh_interval(DEFAULT_REFRESH_RATE, sample_rate);
    return ProcessorReturnCode::OK;
}

void PeakMeterPlugin::configure(float sample_rate)
{
    _sample_rate = sample_rate;
    _update_refresh_interval(_update_rate_parameter->processed_value(), sample_rate);
}

void PeakMeterPlugin::process_event(const RtEvent& event)
{
    InternalPlugin::process_event(event);

    if (event.type() == RtEventType::FLOAT_PARAMETER_CHANGE &&
        event.parameter_change_event()->param_id() == _update_rate_id)
    {
        _update_refresh_interval(_update_rate_parameter->processed_value(), _sample_rate);
    }
}

void PeakMeterPlugin::_update_refresh_interval(float rate, float sample_rate)
{
    _refresh_interval = static_cast<int>(std::round(sample_rate / rate));
    _clip_hold_samples = sample_rate * CLIP_HOLD_TIME.count();
    for (auto& i :  _smoothers)
    {
        i.set_lag_time(REFRESH_TIME, sample_rate / AUDIO_CHUNK_SIZE);
    }
}

void PeakMeterPlugin::_process_peak_detection(const ChunkSampleBuffer& in, bool linked, bool send_only_peaks)
{
    std::array<float, MAX_METERED_CHANNELS> peak;
    peak.fill(0.0f);

    int channels = std::min(MAX_METERED_CHANNELS, in.channel_count());

    for (int ch = 0; ch < channels; ++ch)
    {
        peak[ch] = in.calc_peak_value(ch);
    }

    if (linked && in.channel_count() > 1)
    {
        float max_peak = std::max(peak[0], peak[1]);
        peak[0] = max_peak;
        peak[1] = max_peak;
    }

    bool update = false;
    _sample_count += AUDIO_CHUNK_SIZE;
    if (_sample_count > _refresh_interval)
    {
        _sample_count -= _refresh_interval;
        update = true;
        if (send_only_peaks)
        {
            update = _peak_hysteresis;
        }
    }

    for (int ch = 0; ch < channels; ++ch)
    {
        float value = peak[ch];
        auto& filter = _smoothers[ch];
        if (value > filter.value())
        {
            filter.set_direct(value);
            _peak_hysteresis = true;
        }
        else
        {
            filter.set(value);
        }

        if (update)
        {
            set_parameter_and_notify(_level_parameters[ch], to_normalised_dB(filter.value()));
            _peak_hysteresis = false;
        }
        filter.next_value();
    }
}

void PeakMeterPlugin::_process_clip_detection(const ChunkSampleBuffer& in, bool linked)
{
    std::array<bool, MAX_METERED_CHANNELS> clipped_ch;
    clipped_ch.fill(false);

    int channels = std::min(MAX_METERED_CHANNELS, in.channel_count());

    for (int ch = 0; ch < channels; ++ch)
    {
        clipped_ch[ch] = in.count_clipped_samples(ch) > 0;
    }

    if (linked && channels > 1)
    {
        clipped_ch[0] |= clipped_ch[1];
        clipped_ch[1] |= clipped_ch[0];
    }

    for (int ch = 0; ch < std::min(MAX_METERED_CHANNELS, in.channel_count()); ++ch)
    {
        if (clipped_ch[ch])
        {
            _clip_hold_count[ch] = 0;
            if (_clipped[ch] == false)
            {
                _clipped[ch] = true;
                set_parameter_and_notify(_clip_parameters[ch], true);
            }
        }
        else if (_clipped[ch] && _clip_hold_count[ch] > _clip_hold_samples)
        {
            _clipped[ch] = false;
            set_parameter_and_notify(_clip_parameters[ch], false);
        }
        _clip_hold_count[ch] += AUDIO_CHUNK_SIZE;
    }
}

std::string_view PeakMeterPlugin::static_uid()
{
    return PLUGIN_UID;
}

}// namespace gain_plugin
}// namespace sushi