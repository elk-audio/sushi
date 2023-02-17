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

#include "plugins/wav_streamer_plugin.h"
#include "logging.h"

namespace sushi::wav_streamer_plugin {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("wav_player");

constexpr auto PLUGIN_UID = "sushi.testing.wav_streamer";
constexpr auto DEFAULT_LABEL = "Wav Streamer";
constexpr int FILE_PROPERTY_ID = 0;

constexpr auto  MAX_FADE_TIME = std::chrono::duration<float, std::ratio<1,1>>(100);
constexpr auto  MIN_FADE_TIME = std::chrono::duration<float, std::ratio<1,1>>(GAIN_SMOOTHING_TIME);

constexpr float MAX_FILE_LENGTH = 60 * 60 * 24;
constexpr int SEEK_UPDATE_INTERVAL = 200;

// Approximate an exponential audio fade with an x^3 curve. Works pretty good over a 60 dB range.
inline float exp_approx(float x, float range)
{
    float norm = range > 0 ? x / range : 0.0f;
    return norm * norm * norm * range;
}

// Catmull-Rom splines, aka Hermite interpolation
template <typename T>
inline T catmull_rom_cubic_int(T frac_pos, T d0, T d1, T d2, T d3)
{
    T f2 = frac_pos * frac_pos;
    T a0 = -0.5f * d0 + 1.5f * d1 - 1.5f * d2 + 0.5f * d3;
    T a1 = d0 - 2.5f * d1 + 2.0f * d2 - 0.5f * d3;
    T a2 = -0.5f * d0 + 0.5f * d2;
    T a3 = d1;

    return(a0 * frac_pos * f2 + a1 * f2 + a2 * frac_pos + a3);
}


int64_t fill_stereo_block(SNDFILE* file, AudioBlock* block, bool looping)
{
    sf_count_t samplecount = 0;
    while (samplecount < BLOCKSIZE)
    {
        auto count = sf_readf_float(file, block->audio_data[INT_MARGIN + samplecount].data(), BLOCKSIZE - samplecount);
        samplecount += count;
        if (samplecount < BLOCKSIZE)
        {
            block->is_last = true;
            if (looping)
            {
                // Start over from the beginning and continue reading.
                sf_seek(file, 0, SEEK_SET);
            }
            else
            {
                break;
            }
        }
    }
    return samplecount;
}

int64_t fill_mono_block(SNDFILE* file, AudioBlock* block, bool looping)
{
    sf_count_t samplecount = 0;
    std::vector<float> tmp_buffer(BLOCKSIZE, 0.0);
    while (samplecount < BLOCKSIZE)
    {
        auto count = sf_readf_float(file, tmp_buffer.data() + samplecount, BLOCKSIZE - samplecount);
        samplecount += count;
        if (samplecount < BLOCKSIZE)
        {
            block->is_last = true;
            if (looping)
            {
                sf_seek(file, 0, SEEK_SET);
            }
            else
            {
                break;
            }
        }
    }
    // Copy interleaved from the temporary buffer to the block
    for (int i = 0; i < BLOCKSIZE; i++)
    {
        block->audio_data[i + INT_MARGIN] = {tmp_buffer[i], tmp_buffer[i]};
    }
    return samplecount;
}

void fill_remainder(AudioBlock* block, std::array<std::array<float, 2>, INT_MARGIN>& remainder)
{
    for (size_t i = 0; i < INT_MARGIN; i++ )
    {
        block->audio_data[i] = remainder[i];
        remainder[i] = block->audio_data[i + BLOCKSIZE];
    }
}

WavStreamerPlugin::WavStreamerPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);

    _remainder.fill({0,0});

    [[maybe_unused]] bool str_pr_ok = register_property("file", "File", "");

    _gain_parameter  = register_float_parameter("volume", "Volume", "dB",
                                                0.0f, -90.0f, 24.0f,
                                                Direction::AUTOMATABLE,
                                                new dBToLinPreProcessor(-90.0f, 24.0f));

    _speed_parameter  = register_float_parameter("playback_speed", "Playback Speed", "",
                                                  1.0f, 0.5f, 2.0f,
                                                  Direction::AUTOMATABLE,
                                                  new FloatParameterPreProcessor(0.5f, 2.0f));

    _fade_parameter   = register_float_parameter("fade_time", "Fade Time", "s",
                                                  0.0f, 0.0f, MAX_FADE_TIME.count(),
                                                  Direction::AUTOMATABLE,
                                                  new FloatParameterPreProcessor(0.0f, MAX_FADE_TIME.count()));


    _seek_parameter   = register_float_parameter("seek", "Seek", "",
                                                 0.0f, 0.0f, 1.0f,
                                                 Direction::AUTOMATABLE,
                                                 new FloatParameterPreProcessor(0.0f, 1.0f));

    _pos_parameter   = register_float_parameter("position", "Position", "",
                                                0.0f, 0.0f, 1.0f,
                                                Direction::OUTPUT,
                                                new FloatParameterPreProcessor(0.0f, 1.0f));

    _length_parameter = register_float_parameter("length", "Length", "s",
                                                 0.0f, 0.0f, MAX_FILE_LENGTH,
                                                 Direction::OUTPUT,
                                                 new FloatParameterPreProcessor(0.0f, MAX_FILE_LENGTH));

    _start_stop_parameter = register_bool_parameter("playing", "Playing", "", false, Direction::AUTOMATABLE);
    _loop_parameter = register_bool_parameter("loop", "Loop", "", false, Direction::AUTOMATABLE);
    _exp_fade_parameter = register_bool_parameter("exp_fade", "Exponential fade", "", false, Direction::AUTOMATABLE);

    assert(_gain_parameter && _speed_parameter && _fade_parameter && _pos_parameter &&
           _start_stop_parameter &&_loop_parameter && _exp_fade_parameter && str_pr_ok);
    _max_input_channels = 0;
}

WavStreamerPlugin::~WavStreamerPlugin()
{
    if (_file)
    {
        std::scoped_lock lock(_file_mutex);
        _close_audio_file();
    }
    AudioBlock* block;
    while (_block_queue.pop(block))
    {
        delete block;
    }
}

ProcessorReturnCode WavStreamerPlugin::init(float sample_rate)
{
    configure(sample_rate);
    return ProcessorReturnCode::OK;
}

void WavStreamerPlugin::configure(float sample_rate)
{
    _sample_rate = sample_rate;
    _gain_smoother.set_lag_time(GAIN_SMOOTHING_TIME, sample_rate / AUDIO_CHUNK_SIZE);
    _exp_gain_smoother.set_lag_time(GAIN_SMOOTHING_TIME, sample_rate / AUDIO_CHUNK_SIZE);
}

void WavStreamerPlugin::set_enabled(bool enabled)
{
    Processor::set_enabled(enabled);
}

void WavStreamerPlugin::set_bypassed(bool bypassed)
{
    _host_control.post_event(new SetProcessorBypassEvent(this->id(), bypassed, IMMEDIATE_PROCESS));
}

void WavStreamerPlugin::process_event(const RtEvent& event)
{
    switch (event.type())
    {
        case RtEventType::SET_BYPASS:
        {
            bool bypassed = static_cast<bool>(event.processor_command_event()->value());
            _bypass_manager.set_bypass(bypassed, _sample_rate);
            break;
        }
        case RtEventType::BOOL_PARAMETER_CHANGE:
        case RtEventType::INT_PARAMETER_CHANGE:
        case RtEventType::FLOAT_PARAMETER_CHANGE:
        {
            InternalPlugin::process_event(event);
            auto typed_event = event.parameter_change_event();
            if (typed_event->param_id() == _start_stop_parameter->descriptor()->id())
            {
                _start_stop_playing(_start_stop_parameter->processed_value());
            }
            else if (typed_event->param_id() == _seek_parameter->descriptor()->id())
            {
                request_non_rt_task(set_seek_callback);
            }
            break;
        }

        default:
            InternalPlugin::process_event(event);
            break;
    }
}

void WavStreamerPlugin::process_audio([[maybe_unused]] const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer)
{
    if (_current_block == nullptr || (_current_block && _current_block->file_idx != _file_idx))
    {
        _load_new_block();
        _update_file_length_display();
    }

    if (_current_block && _bypass_manager.should_process() && _mode != sushi::wav_streamer_plugin::StreamingMode::STOPPED)
    {
        float gain_value = _gain_parameter->processed_value();

        if (_mode == StreamingMode::PLAYING || _mode == StreamingMode::STARTING)
        {
            _gain_smoother.set(gain_value);
            _exp_gain_smoother.set(gain_value);
        }

        _fill_audio_data(out_buffer, _file_samplerate / _sample_rate * _speed_parameter->processed_value());

        _handle_fades(out_buffer);
    }
    else
    {
        out_buffer.clear();
    }

    if (++_seek_update_count > SEEK_UPDATE_INTERVAL)
    {
        _update_position_display(_loop_parameter->processed_value());
        _seek_update_count = 0;
    }

    _mode = _update_mode(_mode);
}

ProcessorReturnCode WavStreamerPlugin::set_property_value(ObjectId property_id, const std::string& value)
{
    auto status = InternalPlugin::set_property_value(property_id, value);
    if (status == ProcessorReturnCode::OK && property_id == FILE_PROPERTY_ID)
    {
        _open_audio_file(value);
        _read_audio_data();
    }
    return status;
}

int WavStreamerPlugin::set_seek_callback(void* data, [[maybe_unused]] EventId id)
{
    assert(data);
    auto instance = reinterpret_cast<WavStreamerPlugin*>(data);
    instance->_set_seek();
    instance->_read_audio_data();
    return 0;
}

int WavStreamerPlugin::read_data_callback(void* data, [[maybe_unused]] EventId id)
{
    assert(data);
    auto instance = reinterpret_cast<WavStreamerPlugin*>(data);
    instance->_read_audio_data();
    return 0;
}

std::string_view WavStreamerPlugin::static_uid()
{
    return PLUGIN_UID;
}

bool WavStreamerPlugin::_open_audio_file(const std::string& path)
{
    std::scoped_lock lock(_file_mutex);

    if (_file)
    {
        _close_audio_file();
    }

    _file = sf_open(path.c_str(), SFM_READ, &_file_info);
    _file_idx += 1;

    if (_file == nullptr)
    {
        _file_length = 0.0f;
        std::string str_error = sf_strerror(nullptr);
        InternalPlugin::set_property_value(FILE_PROPERTY_ID, "Error: " + str_error);
        SUSHI_LOG_ERROR("Failed to load audio file: {}, error: {}", path, str_error);
        return false;
    }

    _file_samplerate = _file_info.samplerate;
    _file_length = _file_info.frames;
    // The file length parameter will be updated from the audio thread

    SUSHI_LOG_INFO("Opened file: {}, {} channels, {} frames, {} Hz", path, _file_info.channels, _file_info.frames, _file_info.samplerate);
    return true;
}

void WavStreamerPlugin::_close_audio_file()
{
    sf_close(_file);
    _file_length = 0;
    _file_pos = 0;
}

int WavStreamerPlugin::_read_audio_data()
{
    std::scoped_lock lock(_file_mutex);

    bool looping = _loop_parameter->processed_value();
    if (_file)
    {
        while (!_block_queue.wasFull())
        {
            auto block = new AudioBlock;
            block->file_pos = sf_seek(_file, 0, SEEK_CUR);
            block->file_idx = _file_idx;

            sf_count_t samplecount;
            if (_file_info.channels == 1)
            {
                samplecount = fill_mono_block(_file, block, looping);
            }
            else
            {
                samplecount = fill_stereo_block(_file, block, looping);
            }
            // Blocks overlap to make interpolation easier
            fill_remainder(block, _remainder);
            _block_queue.push(block);

            if (samplecount < BLOCKSIZE)
            {
                break;
            }
        }
    }
    return 0;
}

void WavStreamerPlugin::_fill_audio_data(ChunkSampleBuffer& buffer, float speed)
{
    bool stereo = buffer.channel_count() > 1;

    for (int s = 0; s < AUDIO_CHUNK_SIZE; s++)
    {
        const auto& data = _current_block->audio_data;

        auto first = static_cast<int>(_current_block_pos);
        float frac_pos = _current_block_pos - std::floor(_current_block_pos);
        assert(first >= 0);
        assert(first < BLOCKSIZE);

        float left = catmull_rom_cubic_int(frac_pos, data[first][LEFT_CHANNEL_INDEX], data[first + 1][LEFT_CHANNEL_INDEX],
                                           data[first + 2][LEFT_CHANNEL_INDEX], data[first + 3][LEFT_CHANNEL_INDEX]);

        float right = catmull_rom_cubic_int(frac_pos, data[first][RIGHT_CHANNEL_INDEX], data[first + 1][RIGHT_CHANNEL_INDEX],
                                            data[first + 2][RIGHT_CHANNEL_INDEX], data[first + 3][RIGHT_CHANNEL_INDEX]);

        if (stereo)
        {
            buffer.channel(LEFT_CHANNEL_INDEX)[s] = left;
            buffer.channel(RIGHT_CHANNEL_INDEX)[s] = right;
        }
        else
        {
            buffer.channel(LEFT_CHANNEL_INDEX)[s] = 0.5f * (left + right);
        }

        _current_block_pos += speed;
        if (_current_block_pos >= BLOCKSIZE)
        {
            // Don't reset to 0, as we want to preserve the fractional position.
            _current_block_pos -= BLOCKSIZE;
            if (!_load_new_block())
            {
                break;
            }
        }
    }
    _file_pos += speed * AUDIO_CHUNK_SIZE;
}

StreamingMode WavStreamerPlugin::_update_mode(StreamingMode current)
{
    switch (current)
    {
        case StreamingMode::STARTING:
            if (_gain_smoother.stationary())
            {
                _gain_smoother.set_lag_time(GAIN_SMOOTHING_TIME, _sample_rate / AUDIO_CHUNK_SIZE);
                _exp_gain_smoother.set_lag_time(GAIN_SMOOTHING_TIME, _sample_rate / AUDIO_CHUNK_SIZE);
                return StreamingMode::PLAYING;
            }
            break;

        case StreamingMode::STOPPING:
            if (_gain_smoother.stationary())
            {
                return StreamingMode::STOPPED;
            }
            break;

        default: {}

    }
    return current;
}

bool WavStreamerPlugin::_load_new_block()
{
    auto prev_block = _current_block;
    AudioBlock* new_block = nullptr;

    while (_block_queue.pop(new_block))
    {
        if (new_block->file_idx == _file_idx)
        {
            _file_pos = new_block->file_pos;
            _update_file_length_display();
            break;
        }
        else // Block is stale
        {
            if (prev_block)
            {
                async_delete(prev_block);
            }
            prev_block = new_block;
            new_block = nullptr;
        }
    }

    _current_block = new_block;

    if (prev_block)
    {
        if ((prev_block->is_last && !_loop_parameter->processed_value()) || _file == nullptr)
        {
            _handle_end_of_file();
        }
        async_delete(prev_block);
    }

    if (_block_queue.wasEmpty())
    {
        // Schedule a task to load more blocks.
        request_non_rt_task(read_data_callback);
    }

    return _current_block;
}

void WavStreamerPlugin::_start_stop_playing(bool start)
{
    auto lag = std::max(MIN_FADE_TIME, MAX_FADE_TIME * _fade_parameter->normalized_value());

    if (start && (_mode != StreamingMode::PLAYING && _mode != StreamingMode::STARTING))
    {
        _mode = StreamingMode::STARTING;
        _gain_smoother.set_lag_time(lag, _sample_rate / AUDIO_CHUNK_SIZE);
        _gain_smoother.set(_gain_parameter->processed_value());
        _exp_gain_smoother.set_lag_time(lag, _sample_rate / AUDIO_CHUNK_SIZE);
        _exp_gain_smoother.set(_gain_parameter->processed_value());
    }

    if (!start && (_mode != StreamingMode::STOPPED && _mode != StreamingMode::STOPPING))
    {
        _mode = StreamingMode::STOPPING;
        _gain_smoother.set_lag_time(lag, _sample_rate / AUDIO_CHUNK_SIZE);
        _gain_smoother.set(0.0f);
        _exp_gain_smoother.set_lag_time(lag, _sample_rate / AUDIO_CHUNK_SIZE);
        _exp_gain_smoother.set(0.0f);
    }
}

void WavStreamerPlugin::_update_position_display(bool looping)
{
    float position = 0;
    if (_file_length > 0.0f)
    {
        // If looping is on, the last block will contain both the start and the end of the files, so let position wraparound
        if (looping)
        {
            position = std::fmod(_file_pos / _file_length, 1.0f);
        }
        else // The last block will contain a bit of silence at the end, let position stay at 1.0
        {
            position = std::clamp(_file_pos / _file_length, 0.0f, 1.0f);
        }
    }

    if (position != _pos_parameter->normalized_value())
    {
        set_parameter_and_notify(_pos_parameter, position);
    }
}

void WavStreamerPlugin::_update_file_length_display()
{
    float length = _file_length / _sample_rate / MAX_FILE_LENGTH;
    if (length != _length_parameter->normalized_value())
    {
        set_parameter_and_notify(_length_parameter, length);
    }
}

void WavStreamerPlugin::_set_seek()
{
    std::scoped_lock lock(_file_mutex);
    if (_file)
    {
        float pos = _seek_parameter->normalized_value();
        SUSHI_LOG_DEBUG("Setting seek to {}", pos);
        sf_seek(_file, pos * _file_length, SEEK_SET);
        _file_idx +=1;
    }
}

void WavStreamerPlugin::_handle_end_of_file()
{
    _mode = StreamingMode::STOPPED;
    _gain_smoother.set_direct(0.0f);
    _exp_gain_smoother.set_direct(0.0f);
    _file_pos = 0;

    set_parameter_and_notify(_start_stop_parameter, false);
    request_non_rt_task(set_seek_callback);
    _update_position_display(false);
}

void WavStreamerPlugin::_handle_fades(ChunkSampleBuffer& buffer)
{
    if (_gain_smoother.stationary()) // Both smoothers run with the same lag, so both should be stationary
    {
        buffer.apply_gain(_gain_smoother.value());
    }
    else // Ramp because start/stop or gain parameter changed
    {
        float start;
        float end;
        if (_exp_fade_parameter->processed_value()) // Exponential fade is enabled
        {
            start = _exp_gain_smoother.value();
            end = _exp_gain_smoother.next_value();
            _gain_smoother.next_value();
        }
        else
        {
            start = _gain_smoother.value();
            end = _gain_smoother.next_value();
            _exp_gain_smoother.next_value();
        }
        buffer.ramp(start, end);
    }

    if (_bypass_manager.should_ramp()) // Ramp because bypass was triggered
    {
        _bypass_manager.ramp_output(buffer);
    }
}

}// namespace sushi::wav_player_plugin