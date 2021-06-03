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

ReturnPlugin::ReturnPlugin(HostControl host_control, SendReturnFactory* manager) : InternalPlugin(host_control),
                                                                                   _manager(manager)
{
    _buffers[0] = ChunkSampleBuffer(MAX_SEND_CHANNELS);
    _buffers[1] = ChunkSampleBuffer(MAX_SEND_CHANNELS);
    _max_input_channels = MAX_SEND_CHANNELS;
    _max_output_channels = MAX_SEND_CHANNELS;
}

ReturnPlugin::~ReturnPlugin()
{
    _manager->on_return_destruction(this);
    for (auto& sender : _senders)
    {
        sender->set_destination(nullptr);
    }
}

void ReturnPlugin::send_audio(const ChunkSampleBuffer& send_buffer, float gain)
{
    std::scoped_lock<SpinLock> lock(_buffer_lock);

    _maybe_swap_buffers(_host_control.transport()->current_process_time());

    if (send_buffer.channel_count() == 1)
    {
        _active_in->add_with_gain(send_buffer, gain);
    }
    else
    {
        int channels = std::min(send_buffer.channel_count(), _current_output_channels);
        for (int c = 0; c < channels; ++c)
        {
            _active_in->add_with_gain(c, c, send_buffer, gain);
        }
    }
}

void ReturnPlugin::send_audio_with_ramp(const ChunkSampleBuffer& send_buffer, float start_gain, float end_gain)
{
    std::scoped_lock<SpinLock> lock(_buffer_lock);

    _maybe_swap_buffers(_host_control.transport()->current_process_time());

    if (send_buffer.channel_count() == 1)
    {
        _active_in->add_with_ramp(send_buffer, start_gain, end_gain);
    }
    else
    {
        int channels = std::min(send_buffer.channel_count(), _current_output_channels);
        for (int c = 0; c < channels; ++c)
        {
            _active_in->add_with_ramp(c, c, send_buffer, start_gain, end_gain);
        }
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
        out_buffer.replace(*_active_out);

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


}// namespace return_plugin
}// namespace sushi