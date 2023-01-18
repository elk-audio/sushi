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

#include "plugins/wav_player_plugin.h"
#include "logging.h"

namespace sushi::wav_player_plugin {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("wav_player");

constexpr auto PLUGIN_UID = "sushi.testing.wav_player";
constexpr auto DEFAULT_LABEL = "Wav Player";
constexpr int FILE_PROPERTY_ID = 0;

constexpr float MAX_FADE_TIME = 5;

WavStreamer::WavStreamer(const std::string& file)

{

}


void WavStreamer::fill_audio_data(ChunkSampleBuffer& buffer)
{

}

bool WavStreamer::needs_disc_access()
{
    return false;
}

void WavStreamer::read_from_disk()
{

}

void WavStreamer::reset_play_head()
{

}

WavPlayerPlugin::WavPlayerPlugin(HostControl host_control) : InternalPlugin(host_control)
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
                                                  0.0f, 0.0f, MAX_FADE_TIME,
                                                  Direction::AUTOMATABLE,
                                                  new FloatParameterPreProcessor(0.0f, MAX_FADE_TIME));

    _start_stop_parameter = register_bool_parameter("playing", "Playing", "", true, Direction::AUTOMATABLE);
    _loop_parameter = register_bool_parameter("loop", "Loop", "", true, Direction::AUTOMATABLE);

    assert(_gain_parameter && _speed_parameter && _fade_parameter && _start_stop_parameter && _loop_parameter && str_pr_ok);
    _max_input_channels = 0;
}

WavPlayerPlugin::~WavPlayerPlugin()
{
    if (_streamer)
    {
        delete _streamer;
    }
}

ProcessorReturnCode WavPlayerPlugin::init(float sample_rate)
{
    configure(sample_rate);
    return ProcessorReturnCode::OK;
}

void WavPlayerPlugin::configure(float sample_rate)
{
    _sample_rate = sample_rate;
    _gain_smoother.set_lag_time(MINIMUM_GAIN_SMOOTHING_TIME, sample_rate / AUDIO_CHUNK_SIZE);
    if (_streamer)
    {
        _streamer->set_sample_rate(sample_rate);
    }
}

void WavPlayerPlugin::set_enabled(bool enabled)
{
    Processor::set_enabled(enabled);
}

void WavPlayerPlugin::set_bypassed(bool bypassed)
{
    _host_control.post_event(new SetProcessorBypassEvent(this->id(), bypassed, IMMEDIATE_PROCESS));
}

void WavPlayerPlugin::process_event(const RtEvent& event)
{
    switch (event.type())
    {
        case RtEventType::SET_BYPASS:
        {
            bool bypassed = static_cast<bool>(event.processor_command_event()->value());
            _bypass_manager.set_bypass(bypassed, _sample_rate);
            break;
        }

        case RtEventType::DATA_PROPERTY_CHANGE:
        {
            auto typed_event = event.data_parameter_change_event();
            auto value = typed_event->value();
            async_delete(_streamer);

            _streamer = reinterpret_cast<WavStreamer*>(value.data);
            break;
        }

        default:
            InternalPlugin::process_event(event);
            break;
    }
}

void WavPlayerPlugin::process_audio(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer)
{
    if (_bypass_manager.should_process() && _streamer)
    {
        _streamer->fill_audio_data(out_buffer);


        if (_bypass_manager.should_ramp())
        {
            _bypass_manager.ramp_output(out_buffer);
        }
    }
    else
    {
        out_buffer.clear();
    }
}

ProcessorReturnCode WavPlayerPlugin::set_property_value(ObjectId property_id, const std::string& value)
{
    if (property_id == FILE_PROPERTY_ID)
    {
        //
    }
    return InternalPlugin::set_property_value(property_id, value);
}

std::string_view WavPlayerPlugin::static_uid()
{
    return PLUGIN_UID;
}

int WavPlayerPlugin::_read_audio_data(EventId id)
{
    if (_streamer)
    {
        SUSHI_LOG_DEBUG("Reading wave data from disk");
        _streamer->read_from_disk();
    }
    return 0;
}

}// namespace sushi::wav_player_plugin