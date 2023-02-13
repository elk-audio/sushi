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
 * @brief Simple plugin for writing wav files.
 * @copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "plugins/wav_writer_plugin.h"
#include "logging.h"

namespace sushi {
namespace wav_writer_plugin {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("wav_writer");

constexpr auto PLUGIN_UID = "sushi.testing.wav_writer";
constexpr auto DEFAULT_LABEL = "Wav writer";
constexpr auto DEFAULT_PATH = "./";
constexpr int DEST_FILE_PROPERTY_ID = 0;

WavWriterPlugin::WavWriterPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);
    _max_input_channels = N_AUDIO_CHANNELS;
    _max_output_channels = N_AUDIO_CHANNELS;

    [[maybe_unused]] bool str_pr_ok = register_property("destination_file", "Destination file", "");
    _recording_parameter = register_bool_parameter("recording", "Recording", "bool", false, Direction::AUTOMATABLE);
    _write_speed_parameter = register_float_parameter("write_speed", "Write Speed", "writes/s",
                                                      DEFAULT_WRITE_INTERVAL,
                                                      MIN_WRITE_INTERVAL,
                                                      MAX_WRITE_INTERVAL,
                                                      Direction::AUTOMATABLE);

    assert(_recording_parameter && _write_speed_parameter && str_pr_ok);
}

WavWriterPlugin::~WavWriterPlugin()
{
    _stop_recording();
}

ProcessorReturnCode WavWriterPlugin::init(float sample_rate)
{
    memset(&_soundfile_info, 0, sizeof(_soundfile_info));
    _soundfile_info.samplerate = sample_rate;
    _soundfile_info.channels = N_AUDIO_CHANNELS;
    _soundfile_info.format = (SF_FORMAT_WAV | SF_FORMAT_PCM_24);
    _write_speed = _write_speed_parameter->domain_value();

    return ProcessorReturnCode::OK;
}

void WavWriterPlugin::configure(float sample_rate)
{
    _soundfile_info.samplerate = sample_rate;
}

void WavWriterPlugin::set_bypassed(bool bypassed)
{
    Processor::set_bypassed(bypassed);
}

void WavWriterPlugin::process_audio(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer)
{
    bypass_process(in_buffer, out_buffer);
    // Put samples in the ringbuffer already in interleaved format
    if (_recording_parameter->processed_value())
    {
        std::array<float, AUDIO_CHUNK_SIZE * N_AUDIO_CHANNELS> temp_buffer;

        // If input is mono put the same audio in both left and right channels.
        if (in_buffer.channel_count() == 1)
        {
            for (int i = 0; i < AUDIO_CHUNK_SIZE; ++i)
            {
                float sample = in_buffer.channel(0)[i];
                temp_buffer[i*2] = sample;
                temp_buffer[i*2+1] = sample;
            }
        }
        else
        {
            in_buffer.to_interleaved(&temp_buffer[0]);
        }
        _ring_buffer.push(temp_buffer);
    }

    // Post RtEvent to write at an interval specified by POST_WRITE_FREQUENCY
    if (_post_write_timer > POST_WRITE_FREQUENCY)
    {
        _post_write_event();
        _post_write_timer = 0;
    }
    _post_write_timer++;
}

WavWriterStatus WavWriterPlugin::_start_recording()
{
    std::string destination_file_path = property_value(DEST_FILE_PROPERTY_ID).second;
    if (destination_file_path.empty())
    {
        // If no file name was passed. Set it to the default;
        destination_file_path = std::string(DEFAULT_PATH + this->name() + "_output");
    }

    _actual_file_path = _available_path(destination_file_path);
    _output_file = sf_open(_actual_file_path.c_str(), SFM_WRITE, &_soundfile_info);
    if (_output_file == nullptr)
    {
        SUSHI_LOG_ERROR("libsndfile error: {}", sf_strerror(_output_file));
        return WavWriterStatus::FAILURE;
    }
    SUSHI_LOG_INFO("Started recording to file: {}", _actual_file_path);
    return WavWriterStatus::SUCCESS;
}

WavWriterStatus WavWriterPlugin::_stop_recording()
{
    _write_to_file(); // write any leftover samples
    int status = sf_close(_output_file);
    if (status != 0)
    {
        SUSHI_LOG_ERROR("libsndfile error: {}", sf_error_number(status));
        return WavWriterStatus::FAILURE;
    }
    SUSHI_LOG_INFO("Finished recording to file: {}", _actual_file_path);
    _output_file = nullptr;
    return WavWriterStatus::SUCCESS;
}

void WavWriterPlugin::_post_write_event()
{
    auto e = RtEvent::make_async_work_event(&WavWriterPlugin::non_rt_callback, this->id(), this);
    output_event(e);
}

int WavWriterPlugin::_write_to_file()
{
    std::array<float, AUDIO_CHUNK_SIZE * N_AUDIO_CHANNELS> cur_buffer;
    while (_ring_buffer.pop(cur_buffer))
    {
        for (auto cur_sample : cur_buffer)
        {
            _file_buffer.push_back(cur_sample);
            _samples_received++;
        }
    }

    _samples_written = 0;

    unsigned int write_limit = static_cast<int>(_write_speed * _soundfile_info.samplerate);
    if (_samples_received > write_limit || _recording_parameter->domain_value() == false)
    {
        while (_samples_written < _samples_received)
        {
            sf_count_t samples_to_write = static_cast<sf_count_t>(_samples_received - _samples_written);
            if (sf_error(_output_file) == 0)
            {
                _samples_written += sf_write_float(_output_file,
                                                  &_file_buffer[_samples_written],
                                                   samples_to_write);
            }
            else
            {
                SUSHI_LOG_ERROR("libsndfile: {}", sf_strerror(_output_file));
                return 0;
            }

        }
        sf_write_sync(_output_file);
        _file_buffer.clear();
        _samples_received = 0;
    }
    return _samples_written;
}

int WavWriterPlugin::_non_rt_callback(EventId /* id */)
{
    WavWriterStatus status = WavWriterStatus::SUCCESS;
    if (_recording_parameter->domain_value() && _total_samples_written < SAMPLE_WRITE_LIMIT)
    {
        if (_output_file == nullptr)
        {
            // only change write speed before recording starts
            _write_speed = _write_speed_parameter->domain_value();
            status = _start_recording();
            if (status == WavWriterStatus::FAILURE)
            {
                return status;
            }
        }
        int samples_written = _write_to_file();
        if (samples_written > 0)
        {
            SUSHI_LOG_DEBUG("Sucessfully wrote {} samples", samples_written);
        }
        _total_samples_written += samples_written;
    }
    else
    {
        if (_output_file)
        {
            status = _stop_recording();
            _total_samples_written = 0;
        }
    }
    return status;
}

std::string WavWriterPlugin::_available_path(const std::string& requested_path)
{
    std::string suffix = ".wav";
    std::string new_path = requested_path + suffix;
    SF_INFO temp_info;
    SNDFILE* temp_file = sf_open(new_path.c_str(), SFM_READ, &temp_info);
    int suffix_counter = 1;
    while (sf_error(temp_file) == 0)
    {
        int status = sf_close(temp_file);
        if (status != 0)
        {
            SUSHI_LOG_ERROR("libsndfile error: {} {}",status, sf_error_number(status));
        }
        SUSHI_LOG_DEBUG("File {} already exists", new_path);
        new_path = requested_path + "_" + std::to_string(suffix_counter) + suffix;
        temp_file = sf_open(new_path.c_str(), SFM_READ, &temp_info);
        suffix_counter++;
    }
    int status = sf_close(temp_file);
    if (status != 0)
    {
        SUSHI_LOG_ERROR("libsndfile error: {}", sf_error_number(status));
    }
    return new_path;
}

std::string_view WavWriterPlugin::static_uid()
{
    return PLUGIN_UID;
}

} // namespace wav_writer_plugin
} // namespace sushis
