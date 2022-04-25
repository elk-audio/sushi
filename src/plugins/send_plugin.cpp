/*
 * Copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Aux send plugin to send audio to a return plugin
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <string>

#include "logging.h"
#include "send_plugin.h"
#include "return_plugin.h"
#include "library/constants.h"

SUSHI_GET_LOGGER_WITH_MODULE_NAME("send_plugin");

namespace sushi {
namespace send_plugin {

constexpr auto PLUGIN_UID = "sushi.testing.send";
constexpr auto DEFAULT_LABEL = "Send";
constexpr auto DEFAULT_DEST = "No destination";
constexpr auto DEST_PROPERTY_ID = 4;

SendPlugin::SendPlugin(HostControl host_control, SendReturnFactory* manager) : InternalPlugin(host_control),
                                                                               _manager(manager)
{
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);
    _gain_parameter  = register_float_parameter("gain", "Gain", "dB",
                                                0.0f, -120.0f, 24.0f,
                                                Direction::AUTOMATABLE,
                                                new dBToLinPreProcessor(-120.0f, 24.0f));

    _channel_count_parameter  = register_int_parameter("channel_count", "Channel count", "",
                                                       MAX_TRACK_CHANNELS, 0, MAX_TRACK_CHANNELS,
                                                       Direction::AUTOMATABLE);
    _start_channel_parameter  = register_int_parameter("start_channel", "Start channel", "",
                                                       0, 0, MAX_TRACK_CHANNELS,
                                                       Direction::AUTOMATABLE);
    _dest_channel_parameter  = register_int_parameter("dest_channel", "Destination channel", "",
                                                       0, 0, MAX_TRACK_CHANNELS,
                                                       Direction::AUTOMATABLE);

    _gain_smoother.set_direct(_gain_parameter->processed_value());

    [[maybe_unused]] bool str_pr_ok = register_property("destination_name", "destination name", DEFAULT_DEST);
    assert(_gain_parameter && str_pr_ok);
    assert(_channel_count_parameter && _start_channel_parameter && _dest_channel_parameter);
}

SendPlugin::~SendPlugin()
{
    if (_destination)
    {
        _destination->remove_sender(this);
    }
}

void SendPlugin::clear_destination()
{
    _destination = nullptr;
    set_property_value(DEST_PROPERTY_ID, DEFAULT_DEST);
}

void SendPlugin::_set_destination(return_plugin::ReturnPlugin* destination)
{
    assert(destination);

    if (_destination)
    {
        _destination->remove_sender(this);
    }
    _destination = destination;
    destination->add_sender(this);
}

ProcessorReturnCode SendPlugin::init(float sample_rate)
{
    configure(sample_rate);
    return ProcessorReturnCode::OK;
}

void SendPlugin::configure(float sample_rate)
{
    _sample_rate = sample_rate;
    _gain_smoother.set_lag_time(GAIN_SMOOTHING_TIME, sample_rate / AUDIO_CHUNK_SIZE);
}

void SendPlugin::process_event(const RtEvent& event)
{
    switch (event.type())
    {
        case RtEventType::SET_BYPASS:
        {
            bool bypassed = static_cast<bool>(event.processor_command_event()->value());
            _bypass_manager.set_bypass(bypassed, _sample_rate);
            break;
        }

        default:
            InternalPlugin::process_event(event);
            break;
    }
}

void SendPlugin::process_audio(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer)
{
    bypass_process(in_buffer, out_buffer);

    if (_bypass_manager.should_process() && _destination)
    {
        float gain = _gain_parameter->processed_value();
        _gain_smoother.set(gain);

        int dest_channel = _dest_channel_parameter->processed_value();
        int start_channel = _start_channel_parameter->processed_value();
        int channels = std::min(_channel_count_parameter->processed_value(), in_buffer.channel_count() - start_channel);

        if (channels <= 0)
        {
            return;
        }

        const auto buffer = ChunkSampleBuffer::create_non_owning_buffer(const_cast<ChunkSampleBuffer&>(in_buffer),
                                                                        start_channel, channels);

        // Ramp if bypass was recently toggled
        if (_bypass_manager.should_ramp())
        {
            auto [start, end] = _bypass_manager.get_ramp();
            start *= _gain_smoother.value();
            end *= _gain_smoother.next_value();
            _destination->send_audio_with_ramp(buffer, dest_channel, start, end);
        }

        // Don't ramp, nominal case
        else if (_gain_smoother.stationary())
        {
            _destination->send_audio(buffer, dest_channel, gain);
        }

        // Ramp because send gain was recently changed
        else
        {
            float start = _gain_smoother.value();
            float end = _gain_smoother.next_value();
            _destination->send_audio_with_ramp(buffer, dest_channel, start, end);
        }
    }
}

bool SendPlugin::bypassed() const
{
    return _bypass_manager.bypassed();
}

void SendPlugin::set_bypassed(bool bypassed)
{
    _host_control.post_event(new SetProcessorBypassEvent(this->id(), bypassed, IMMEDIATE_PROCESS));
}

ProcessorReturnCode SendPlugin::set_property_value(ObjectId property_id, const std::string& value)
{
    if (property_id == DEST_PROPERTY_ID)
    {
        _change_return_destination(value);
    }
    return InternalPlugin::set_property_value(property_id, value);
}

std::string_view SendPlugin::static_uid()
{
    return PLUGIN_UID;
}

void SendPlugin::_change_return_destination(const std::string& dest_name)
{
    return_plugin::ReturnPlugin* return_plugin = _manager->lookup_return_plugin(dest_name);
    if (return_plugin)
    {
        _set_destination(return_plugin);
    }
    else
    {
        SUSHI_LOG_WARNING("Return plugin {} not found", dest_name);
    }
}

}// namespace send_plugin
}// namespace sushi
