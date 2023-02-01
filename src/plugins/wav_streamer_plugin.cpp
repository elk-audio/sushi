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
 * @brief Simple plugin playing wav files.
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include "plugins/wav_streamer_plugin.h"
#include "logging.h"

namespace sushi::wav_streamer_plugin {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("wav_player");

constexpr auto PLUGIN_UID = "sushi.testing.wav_player";
constexpr auto DEFAULT_LABEL = "Wav Player";
constexpr int FILE_PROPERTY_ID = 0;

//constexpr float MAX_FADE_TIME = 5;
constexpr auto  MAX_FADE_TIME = std::chrono::duration<float, std::ratio<1,1>>(5);
constexpr auto  MIN_FADE_TIME = std::chrono::duration<float, std::ratio<1,1>>(GAIN_SMOOTHING_TIME);

// Approximate an exponential audio fade with an x^3 curve. Works pretty good over a 60 dB range.
inline float exp_approx(float x)
{
    return x * x * x;
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


WavStreamerPlugin::WavStreamerPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);
    [[maybe_unused]] bool str_pr_ok = register_property("file", "File", "");

    _gain_parameter  = register_float_parameter("volume", "Volume", "dB",
                                                0.0f, -120.0f, 36.0f,
                                                Direction::AUTOMATABLE,
                                                new dBToLinPreProcessor(-120.0f, 36.0f));

    _speed_parameter  = register_float_parameter("playback_speed", "Playback Speed", "",
                                                  1.0f, 0.5f, 2.0f,
                                                  Direction::AUTOMATABLE,
                                                  new FloatParameterPreProcessor(0.5f, 2.0f));

    _fade_parameter   = register_float_parameter("fade_time", "Fade Time", "s",
                                                  0.0f, 0.0f, MAX_FADE_TIME.count(),
                                                  Direction::AUTOMATABLE,
                                                  new FloatParameterPreProcessor(0.0f, MAX_FADE_TIME.count()));

    _start_stop_parameter = register_bool_parameter("playing", "Playing", "", false, Direction::AUTOMATABLE);
    _loop_parameter = register_bool_parameter("loop", "Loop", "", false, Direction::AUTOMATABLE);
    _exp_fade_parameter = register_bool_parameter("exp_fade", "Exponential fade", "", false, Direction::AUTOMATABLE);

    assert(_gain_parameter && _speed_parameter && _fade_parameter && _start_stop_parameter && _loop_parameter && str_pr_ok);
    _max_input_channels = 0;
}

WavStreamerPlugin::~WavStreamerPlugin()
{
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
            break;
        }

        default:
            InternalPlugin::process_event(event);
            break;
    }
}

void WavStreamerPlugin::process_audio(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer)
{
    if (_bypass_manager.should_process() && _mode != sushi::wav_streamer_plugin::StreamingMode::STOPPED)
    {
        if (_mode == StreamingMode::PLAYING)
        {
            _gain_smoother.set(_gain_parameter->processed_value());
        }

        //SUSHI_LOG_INFO("Playing {} {}", _current_block_index, _gain_smoother.value());
        // Use an exponential curve to fade audio and not a linear ramp
        bool lin_fade = true; // _exp_fade_parameter->processed_value();

        _fill_audio_data(out_buffer, _wave_samplerate / _sample_rate * _speed_parameter->processed_value());

        if (_gain_smoother.stationary())
        {
            out_buffer.apply_gain(lin_fade ? _gain_smoother.value() : exp_approx(_gain_smoother.value()));
        }
        else
        {
            float start = _gain_smoother.value();
            float end = _gain_smoother.next_value();
            out_buffer.ramp(lin_fade ? start : exp_approx(start), lin_fade ? end : exp_approx(end));
        }
        if (_bypass_manager.should_ramp())
        {
            _bypass_manager.ramp_output(out_buffer);
        }
    }
    else
    {
        out_buffer.clear();
    }

    _mode = _update_mode(_mode);
}

ProcessorReturnCode WavStreamerPlugin::set_property_value(ObjectId property_id, const std::string& value)
{
    if (property_id == FILE_PROPERTY_ID)
    {
        _load_audio_file(value);
    }
    return InternalPlugin::set_property_value(property_id, value);
}

std::string_view WavStreamerPlugin::static_uid()
{
    return PLUGIN_UID;
}

bool WavStreamerPlugin::_load_audio_file(const std::string& path)
{
    std::scoped_lock lock(_audio_file_mutex);

    if (_audio_file)
    {
        _close_audio_file();
    }

    _audio_file = sf_open(path.c_str(), SFM_READ, &_soundfile_info);
    if (_audio_file == nullptr)
    {
        SUSHI_LOG_ERROR("Failed to load audio file: {}, error: {}", path, sf_strerror(nullptr));
        return false;
    }

    _wave_samplerate = _soundfile_info.samplerate;
    SUSHI_LOG_INFO("Loaded file: {}, {} channels, {} frames, {} Hz", path, _soundfile_info.channels,
                   _soundfile_info.frames, _soundfile_info.samplerate);

    return true;
}

void WavStreamerPlugin::_close_audio_file()
{
    sf_close(_audio_file);
}

int WavStreamerPlugin::_read_audio_data([[maybe_unused]] EventId id)
{
    std::scoped_lock lock(_audio_file_mutex);

    bool looping = _loop_parameter->processed_value();
    if (_audio_file)
    {
        SUSHI_LOG_DEBUG("Reading wave data from disk");
        bool end_reached = false;

        while (!_block_queue.wasFull() && !end_reached)
        {
            auto block = new AudioBlock;
            block->audio_data.fill({0.0,0.0});
            sf_count_t samplecount = 0;

            if (_soundfile_info.channels == 2)
            {
                while (samplecount < BLOCKSIZE)
                {
                    auto count = sf_readf_float(_audio_file, block->audio_data[INT_MARGIN + samplecount].data(), BLOCKSIZE - samplecount);
                    samplecount += count;
                    if (samplecount < BLOCKSIZE && looping)
                    {
                        // Start over from the beginning and read again.
                        sf_seek(_audio_file, 0, SEEK_SET);
                    }
                    else
                    {
                        end_reached = true;
                        break;
                    }
                }
            }
            else // Mono file, read to a temporary buffer first.
            {
                std::vector<float> tmp_buffer(BLOCKSIZE, 0.0);
                while (samplecount < BLOCKSIZE)
                {
                    auto count = sf_readf_float(_audio_file, tmp_buffer.data() + samplecount, BLOCKSIZE - samplecount);
                    samplecount += count;
                    if (samplecount < BLOCKSIZE && looping)
                    {
                        sf_seek(_audio_file, 0, SEEK_SET);
                    }
                    else
                    {
                        end_reached = true;
                        break;
                    }
                }
                // Copy interleaved from the temporary buffer to the block
                for (int i = 0; i < BLOCKSIZE; i++)
                {
                    block->audio_data[i + INT_MARGIN] = {tmp_buffer[i], tmp_buffer[i]};
                }
            }

            SUSHI_LOG_DEBUG("Pushed 1 audio block");
            _block_queue.push(block);
        }
    }
    return 0;
}

void WavStreamerPlugin::_fill_audio_data(ChunkSampleBuffer& buffer, float speed)
{
    if (_current_block == nullptr)
    {
        if (!_load_new_block())
        {
            buffer.clear();
            return;
        }
    }

    bool stereo = buffer.channel_count() > 1;

    for (int s = 0; s < AUDIO_CHUNK_SIZE; s++)
    {
        const auto& data = _current_block->audio_data;

        auto first = static_cast<int>(_current_block_index);
        float frac_pos = _current_block_index - std::floor(_current_block_index);
        assert(first >= 0);
        assert(first < BLOCKSIZE);

        // TODO - Non interpolated copy for when speed == 1.0 ?
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
            buffer.channel(0)[s] = 0.5f * (left + right);
        }

        _current_block_index += speed;
        if (_current_block_index >= BLOCKSIZE)
        {
            // Don't reset to 0, as we want to preserve the fractional position.
            _current_block_index -= BLOCKSIZE;
            if (!_load_new_block())
            {
                break;
            }
        }
    }
}

StreamingMode WavStreamerPlugin::_update_mode(StreamingMode current)
{
    switch (current)
    {
        case StreamingMode::STARTING:
            if (_gain_smoother.stationary())
            {
                _gain_smoother.set_lag_time(GAIN_SMOOTHING_TIME, _sample_rate / AUDIO_CHUNK_SIZE);
                return StreamingMode::PLAYING;
            }

        case StreamingMode::STOPPING:
            if (_gain_smoother.stationary())
            {
                return StreamingMode::STOPPED;
            }
        default:
            return current;
    }
}

bool WavStreamerPlugin::_load_new_block()
{
    auto old_block = _current_block;
    bool status = _block_queue.pop(_current_block);

    if (status == false)
    {
        _current_block = nullptr;
    }

    // TODO - or always do this?
    if (_block_queue.wasEmpty())
    {
        // Schedule a request to load more blocks.
        request_non_rt_task(non_rt_callback);
    }

    // Delete old block in non-rt thread.
    if (old_block)
    {
        async_delete(old_block);
    }
    return status;
}

void WavStreamerPlugin::_start_stop_playing(bool start)
{
    auto lag = std::max(MIN_FADE_TIME, MAX_FADE_TIME * _fade_parameter->processed_value());

    if (start && (_mode != StreamingMode::PLAYING && _mode != StreamingMode::STARTING))
    {
        _mode = StreamingMode::STARTING;
        _gain_smoother.set_lag_time(lag, _sample_rate / AUDIO_CHUNK_SIZE);
        _gain_smoother.set(_gain_parameter->processed_value());
    }

    if (!start && (_mode != StreamingMode::STOPPED && _mode != StreamingMode::STOPPING))
    {
        _mode = StreamingMode::STOPPING;
        _gain_smoother.set_lag_time(lag, _sample_rate / AUDIO_CHUNK_SIZE);
        _gain_smoother.set(0.0f);
    }
}


}// namespace sushi::wav_player_plugin