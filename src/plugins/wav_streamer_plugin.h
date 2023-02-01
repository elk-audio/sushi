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

#ifndef SUSHI_WAV_STREAMER_PLUGIN_H
#define SUSHI_WAV_STREAMER_PLUGIN_H

#include <array>
#include <sndfile.h>

#include "fifo/circularfifo_memory_relaxed_aquire_release.h"

#include "dsp_library/value_smoother.h"
#include "library/internal_plugin.h"

namespace sushi::wav_streamer_plugin {

enum class StreamingMode
{
    PLAYING,
    STARTING,
    STOPPING,
    STOPPED,};

// Roughly 3 seconds of stereo audio per block @ 48kHz.
constexpr ssize_t BLOCKSIZE = 150'000;
// Extra margin for interpolation
constexpr size_t PRE_SAMPLES = 1;
constexpr size_t POST_SAMPLES = 2;
constexpr size_t INT_MARGIN = PRE_SAMPLES + POST_SAMPLES;


struct AudioBlock : public RtDeletable
{
    std::array<std::array<float, 2>, BLOCKSIZE + INT_MARGIN> audio_data;
};

class WavStreamerPlugin : public InternalPlugin, public UidHelper<WavStreamerPlugin>
{
public:
    WavStreamerPlugin(HostControl host_control);

    ~WavStreamerPlugin();

    virtual ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void set_enabled(bool enabled) override;

    void set_bypassed(bool bypassed) override;

    void process_event(const RtEvent& event) override;

    void process_audio(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer) override;

    ProcessorReturnCode set_property_value(ObjectId property_id, const std::string& value) override;

    static int non_rt_callback(void* data, EventId id)
    {
        return reinterpret_cast<WavStreamerPlugin*>(data)->_read_audio_data(id);
    }

    static std::string_view static_uid();

private:
    bool _load_audio_file(const std::string& path);

    void _close_audio_file();

    int _read_audio_data(EventId id);

    void _fill_audio_data(ChunkSampleBuffer& buffer, float speed);

    StreamingMode _update_mode(StreamingMode current);

    bool _load_new_block();

    void _start_stop_playing(bool start);

    ValueSmootherRamp<float>    _gain_smoother;

    FloatParameterValue* _gain_parameter;
    FloatParameterValue* _speed_parameter;
    FloatParameterValue* _fade_parameter;
    BoolParameterValue*  _start_stop_parameter;
    BoolParameterValue*  _loop_parameter;
    BoolParameterValue*  _exp_fade_parameter;

    float _sample_rate{0};
    float _wave_samplerate{0};

    std::mutex  _audio_file_mutex;
    SNDFILE*    _audio_file{nullptr};
    SF_INFO     _soundfile_info;
    sf_count_t  _soundfile_index;

    BypassManager _bypass_manager;

    StreamingMode _mode{sushi::wav_streamer_plugin::StreamingMode::STOPPED};

    AudioBlock* _current_block{nullptr};
    float _current_block_index{0};

    memory_relaxed_aquire_release::CircularFifo<AudioBlock*, 5> _block_queue;
};

}// namespace sushi::wav_player_plugin

#endif //SUSHI_WAV_STREAMER_PLUGIN_H
