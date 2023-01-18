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
 * @brief Simple plugin for playing wav files.
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#ifndef SUSHI_WAV_PLAYER_PLUGIN_H
#define SUSHI_WAV_PLAYER_PLUGIN_H

#include <array>
#include <sndfile.h>

#include "fifo/circularfifo_memory_relaxed_aquire_release.h"

#include "dsp_library/value_smoother.h"
#include "library/internal_plugin.h"

namespace sushi::wav_player_plugin {

constexpr int N_AUDIO_CHANNELS = 2;
constexpr int RINGBUFFER_SIZE = 65536 / AUDIO_CHUNK_SIZE;
constexpr int POST_WRITE_FREQUENCY = (RINGBUFFER_SIZE / 4);
constexpr int SAMPLE_WRITE_LIMIT = 48000 * N_AUDIO_CHANNELS * 3600; // Limit file size to 1 hour of stereo audio
constexpr float DEFAULT_WRITE_INTERVAL = 1.0f;
constexpr float MAX_WRITE_INTERVAL = 4.0f;
constexpr float MIN_WRITE_INTERVAL = 0.5f;


class WavStreamer : public RtDeletable
{
public:
    WavStreamer(const std::string& file);

    void set_sample_rate(float sample_rate)
    {
        _sample_rate = sample_rate;
    }

    void set_loop_mode(bool enabled)
    {
        _looping = enabled;
    }

    void set_playback_speed(float speed)
    {
        _speed = speed;
    }

    void fill_audio_data(ChunkSampleBuffer& buffer);

    bool needs_disc_access();

    void read_from_disk();

    void reset_play_head();

private:
    float _sample_rate{0};
    float _wave_samplerate{0};
    float _speed{1};
    float _index;
    bool _looping{false};


    std::array<std::vector<std::array<float, 2>>, 3> _sample_data;
};



class WavPlayerPlugin : public InternalPlugin, public UidHelper<WavPlayerPlugin>
{
public:
    WavPlayerPlugin(HostControl host_control);

    ~WavPlayerPlugin();

    virtual ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void set_enabled(bool enabled) override;

    void set_bypassed(bool bypassed) override;

    void process_event(const RtEvent& event) override;

    void process_audio(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer) override;

    ProcessorReturnCode set_property_value(ObjectId property_id, const std::string& value) override;

    static int non_rt_callback(void* data, EventId id)
    {
        return reinterpret_cast<WavPlayerPlugin*>(data)->_read_audio_data(id);
    }

    static std::string_view static_uid();

private:
    int _read_audio_data(EventId id);

    float                         _wave_sample_rate;
    float                         _sample_rate;

    ValueSmootherFilter<float>    _gain_smoother;

    FloatParameterValue* _gain_parameter;
    FloatParameterValue* _speed_parameter;
    FloatParameterValue* _fade_parameter;
    BoolParameterValue* _start_stop_parameter;
    BoolParameterValue* _loop_parameter;

    BypassManager _bypass_manager;
    WavStreamer*  _streamer;
};

}// namespace sushi::wav_player_plugin

#endif //SUSHI_WAV_PLAYER_PLUGIN_H
