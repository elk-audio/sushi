/*
 * Copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Aux return plugin to return audio from a send plugin
 * @copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <algorithm>

#include "return_plugin.h"
#include "send_plugin.h"

namespace sushi {
namespace return_plugin {

constexpr auto PLUGIN_UID = "sushi.testing.return";
constexpr auto DEFAULT_LABEL = "Return";

ReturnPlugin::ReturnPlugin(HostControl host_control, SendReturnFactory* manager) : InternalPlugin(host_control),
                                                                                   _manager(manager)
{
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);
    _max_input_channels = MAX_SEND_CHANNELS;
    _max_output_channels = MAX_SEND_CHANNELS;
}

ReturnPlugin::~ReturnPlugin()
{
    _manager->on_return_destruction(this);
    for (auto& sender : _senders)
    {
        sender->clear_destination();
    }
}

void ReturnPlugin::send_audio(const ChunkSampleBuffer& buffer, int start_channel, float gain)
{
    std::scoped_lock<SpinLock> lock(_buffer_lock);

    _maybe_swap_buffers(_host_control.transport()->current_process_time());

    int max_channels = std::max(0, std::min(buffer.channel_count(), _current_output_channels - start_channel));

    for (int c = 0 ; c < max_channels; ++c)
    {
        _active_in->add_with_gain(start_channel++, c, buffer, gain);
    }
}

void ReturnPlugin::send_audio_with_ramp(const ChunkSampleBuffer& buffer, int start_channel,
                                        float start_gain, float end_gain)
{
    std::scoped_lock<SpinLock> lock(_buffer_lock);

    _maybe_swap_buffers(_host_control.transport()->current_process_time());

    int max_channels = std::max(0, std::min(buffer.channel_count(), _current_output_channels - start_channel));

    for (int c = 0 ; c < max_channels; ++c)
    {
        _active_in->add_with_ramp(start_channel++, c, buffer, start_gain, end_gain);
    }
}

void ReturnPlugin::add_sender(send_plugin::SendPlugin* sender)
{
    _senders.push_back(sender);
}

void ReturnPlugin::remove_sender(send_plugin::SendPlugin* sender)
{
    _senders.erase(std::remove(_senders.begin(), _senders.end(), sender), _senders.end());
}

ProcessorReturnCode ReturnPlugin::init(float sample_rate)
{
    configure(sample_rate);
    return ProcessorReturnCode::OK;
}

void ReturnPlugin::configure(float sample_rate)
{
    _sample_rate = sample_rate;
    for (auto& buffer : _buffers)
    {
        buffer.clear();
    }
}

void ReturnPlugin::set_input_channels(int channels)
{
    Processor::set_input_channels(channels);
    _channel_config(channels);
}

void ReturnPlugin::set_output_channels(int channels)
{
    Processor::set_output_channels(channels);
    _channel_config(channels);
}

void ReturnPlugin::set_enabled(bool enabled)
{
    if (enabled == false)
    {
        for (auto& buffer : _buffers)
        {
            buffer.clear();
        }
    }
}

void ReturnPlugin::process_event(const RtEvent& event)
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

void ReturnPlugin::process_audio(const ChunkSampleBuffer& /*in_buffer*/, ChunkSampleBuffer& out_buffer)
{
    {
        std::scoped_lock<SpinLock> lock(_buffer_lock);
        _maybe_swap_buffers(_host_control.transport()->current_process_time());
    }

    if (_bypass_manager.should_process())
    {
        auto buffer = ChunkSampleBuffer::create_non_owning_buffer(*_active_out, 0, out_buffer.channel_count());
        out_buffer.replace(buffer);

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

bool ReturnPlugin::bypassed() const
{
    return _bypass_manager.bypassed();
}

void ReturnPlugin::set_bypassed(bool bypassed)
{
    _host_control.post_event(new SetProcessorBypassEvent(this->id(), bypassed, IMMEDIATE_PROCESS));
}

std::string_view ReturnPlugin::static_uid()
{
    return PLUGIN_UID;
}

void inline ReturnPlugin::_swap_buffers()
{
    std::swap(_active_in, _active_out);
    _active_in->clear();
}

void inline ReturnPlugin::_maybe_swap_buffers(Time current_time)
{
    Time last_time = _last_process_time.load(std::memory_order_acquire);
    if (last_time != current_time)
    {
        _last_process_time.store(current_time, std::memory_order_release);
        _swap_buffers();
    }
}

void ReturnPlugin::_channel_config(int channels)
{
    int max_channels = std::max(std::max(channels, _current_input_channels), _current_output_channels);
    if (_buffers.front().channel_count() != max_channels)
    {
        _buffers.fill(ChunkSampleBuffer(max_channels));
    }
}

}// namespace return_plugin
}// namespace sushi