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

#include "peak_meter_plugin.h"

namespace sushi {
namespace peak_meter_plugin {

constexpr int MAX_CHANNELS = 16;
/* Number of updates per second */
constexpr float REFRESH_RATE = 25;
constexpr auto REFRESH_TIME = std::chrono::milliseconds(3 * 1000 / static_cast<int>(REFRESH_RATE));

constexpr auto CLIP_HOLD_TIME = std::chrono::seconds(5);
/* fc in Hz, Tweaked by eyeballing mostly */
constexpr float SMOOTHING_CUTOFF = 2.3;
/* Represents -120dB */

constexpr float OUTPUT_MIN_DB = -120.0f;
constexpr float OUTPUT_MAX_DB = 24.0f;
constexpr float OUTPUT_MIN = 1.0e-6f; // -120dB

static const std::string DEFAULT_NAME = "sushi.testing.peakmeter";
static const std::string DEFAULT_LABEL = "Peak Meter";

// Convert a gain value to a normalised gain value
inline float to_normalised_dB(float gain)
{
    float db_gain = 20.0f * std::log10(std::max(gain, OUTPUT_MIN));
    float norm = (db_gain - OUTPUT_MIN_DB) / (OUTPUT_MAX_DB - OUTPUT_MIN_DB);
    return std::clamp(norm, 0.0f, 1.0f);
}

PeakMeterPlugin::PeakMeterPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    _max_input_channels = MAX_CHANNELS;
    _max_output_channels = MAX_CHANNELS;
    Processor::set_name(DEFAULT_NAME);
    Processor::set_label(DEFAULT_LABEL);
    _level_parameters[LEFT_CHANNEL_INDEX] = register_float_parameter("left_level", "Level (left)", "dB",
                                                                     OUTPUT_MIN_DB, OUTPUT_MIN_DB, OUTPUT_MAX_DB,
                                                                     new dBToLinPreProcessor(OUTPUT_MIN, 24.0f));

    _level_parameters[RIGHT_CHANNEL_INDEX] = register_float_parameter("right_level", "Level (right)", "dB",
                                                                      OUTPUT_MIN_DB, OUTPUT_MIN_DB, OUTPUT_MAX_DB,
                                                                      new dBToLinPreProcessor(OUTPUT_MIN, 24.0f));

    _clip_parameters[LEFT_CHANNEL_INDEX] = register_bool_parameter("right_clip", "Clip (left)", "", false);
    _clip_parameters[RIGHT_CHANNEL_INDEX] = register_bool_parameter("left_clip", "Clip (right)", "", false);

    _link_channels_parameter = register_bool_parameter("link_channels", "Link Channels", "", false);

    for ([[maybe_unused]] const auto& i : _level_parameters) { assert(i);}
    for ([[maybe_unused]] const auto& i : _clip_parameters) { assert(i);}
    assert(_link_channels_parameter);
}

void PeakMeterPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    bypass_process(in_buffer, out_buffer);

    bool linked = _link_channels_parameter->processed_value();
    _process_peak_detection(in_buffer, linked);
    _process_clip_detection(in_buffer, linked);
}

ProcessorReturnCode PeakMeterPlugin::init(float sample_rate)
{
    _update_refresh_interval(sample_rate);
    return ProcessorReturnCode::OK;
}

void PeakMeterPlugin::configure(float sample_rate)
{
    _update_refresh_interval(sample_rate);
}

void PeakMeterPlugin::_update_refresh_interval(float sample_rate)
{
    _refresh_interval = static_cast<int>(std::round(sample_rate / REFRESH_RATE));
    _clip_hold_samples = sample_rate * CLIP_HOLD_TIME.count();
    for (auto& i :  _smoothers)
    {
        i.set_lag_time(REFRESH_TIME, sample_rate);
    }
}

void PeakMeterPlugin::_process_peak_detection(const ChunkSampleBuffer& in, bool linked)
{
    float peak[MAX_METERED_CHANNELS] = {0,0};
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
    }

    for (int ch = 0; ch < channels; ++ch)
    {
        float value = peak[ch];
        auto& filter = _smoothers[ch];
        if (value > filter.value())
        {
            filter.set_direct(value);
        }
        else
        {
            filter.set(value);
        }

        if (update)
        {
            set_parameter_and_notify(_level_parameters[ch], to_normalised_dB(filter.value()));
        }
        _smoothers[ch].next_value();
    }
}

void PeakMeterPlugin::_process_clip_detection(const ChunkSampleBuffer& in, bool linked)
{
    bool clipped_ch[MAX_METERED_CHANNELS] = {false, false};
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


}// namespace gain_plugin
}// namespace sushi