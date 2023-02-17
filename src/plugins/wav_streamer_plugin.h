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
 * @brief Plugin for streaming large wav files from disk
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
    STOPPED
};

// Roughly 2 seconds of stereo audio per block @ 48kHz.
constexpr ssize_t BLOCKSIZE = 100'000;
constexpr ssize_t QUEUE_SIZE = 4;

// Extra margin for interpolation
constexpr size_t PRE_SAMPLES = 1;
constexpr size_t POST_SAMPLES = 2;
constexpr size_t INT_MARGIN = PRE_SAMPLES + POST_SAMPLES;

/**
 * @brief A block of stereo audio data with some basic control data
 */
struct AudioBlock : public RtDeletable
{
    AudioBlock() : file_pos(0), file_idx(0), is_last(false)
    {
        audio_data.fill({0.0f, 0.0f});
    }

    int64_t file_pos;
    int file_idx;
    bool is_last;
    std::array<std::array<float, 2>, BLOCKSIZE + INT_MARGIN> audio_data;
};

/**
 * @brief Fill an AudioBlock with data from a stereo file
 * @param file  An open SDNFILE object
 * @param block An allocated AudioBlock to be populated
 * @param looping If true, reading starts over from the beginning of the file if the end is reached
 * @return The number of frames read
 */
int64_t fill_stereo_block(SNDFILE* file, AudioBlock* block, bool looping);

/**
 * @brief Fill an AudioBlock with data from a mono file
 * @param file  An open SDNFILE object
 * @param block An allocated AudioBlock to be populated
 * @param looping If true, reading starts over from the beginning of the file if the end is reached
 * @return The number of frames read
 */
int64_t fill_mono_block(SNDFILE* file, AudioBlock* block, bool looping);

void fill_remainder(AudioBlock* block, std::array<std::array<float, 2>, INT_MARGIN>& remainder);

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

    static int read_data_callback(void* data, EventId id);

    static int set_seek_callback(void* data, EventId id);

    static std::string_view static_uid();

private:
    bool _open_audio_file(const std::string& path);

    void _close_audio_file();

    int _read_audio_data();

    void _fill_audio_data(ChunkSampleBuffer& buffer, float speed);

    StreamingMode _update_mode(StreamingMode current);

    bool _load_new_block();

    void _start_stop_playing(bool start);

    void _update_position_display(bool looping);

    void _update_file_length_display();

    void _set_seek();

    void _handle_fades(ChunkSampleBuffer& buffer);

    void _handle_end_of_file();

    ValueSmootherRamp<float>   _gain_smoother;
    ValueSmootherExpRamp<float> _exp_gain_smoother;

    FloatParameterValue* _gain_parameter;
    FloatParameterValue* _speed_parameter;
    FloatParameterValue* _fade_parameter;
    FloatParameterValue* _pos_parameter;
    FloatParameterValue* _seek_parameter;
    FloatParameterValue* _length_parameter;
    BoolParameterValue*  _start_stop_parameter;
    BoolParameterValue*  _loop_parameter;
    BoolParameterValue*  _exp_fade_parameter;

    float _sample_rate{0};
    float _file_samplerate{0};
    float _file_length{1};
    int   _file_idx{0};

    std::array<std::array<float, 2>, INT_MARGIN> _remainder;

    std::mutex  _file_mutex;
    SNDFILE*    _file{nullptr};
    SF_INFO     _file_info;

    BypassManager _bypass_manager;

    StreamingMode _mode{sushi::wav_streamer_plugin::StreamingMode::STOPPED};

    AudioBlock* _current_block{nullptr};
    float _current_block_pos{0};
    float _file_pos{0};

    int _seek_update_count{0};

    memory_relaxed_aquire_release::CircularFifo<AudioBlock*, QUEUE_SIZE> _block_queue;
};

}// namespace sushi::wav_player_plugin

#endif //SUSHI_WAV_STREAMER_PLUGIN_H
