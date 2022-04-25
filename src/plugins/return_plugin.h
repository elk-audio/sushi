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
 * @brief Aux return plugin to return audio from a Send Plugin
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_RETURN_PLUGIN_H
#define SUSHI_RETURN_PLUGIN_H

#include <atomic>

#include "library/spinlock.h"
#include "send_return_factory.h"
#include "library/internal_plugin.h"

namespace sushi {

// Forward declarations to keep the number of includes down
class SendReturnFactory;
namespace send_plugin {class SendPlugin;}

namespace return_plugin {

class ReturnPlugin : public InternalPlugin, public UidHelper<ReturnPlugin>
{
public:
    ReturnPlugin(HostControl host_control, SendReturnFactory* manager);

    ~ReturnPlugin() override;

    int return_id() const {return _return_id;}

    void send_audio(const ChunkSampleBuffer& buffer, int start_channel, float gain);

    void send_audio_with_ramp(const ChunkSampleBuffer& buffer, int start_channel, float start_gain, float end_gain);

    void add_sender(send_plugin::SendPlugin* sender);

    void remove_sender(send_plugin::SendPlugin* sender);

    // From Processor
    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void set_input_channels(int channels) override;

    void set_output_channels(int channels) override;

    void set_enabled(bool enabled) override;

    void process_event(const RtEvent& event) override;

    void process_audio(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer) override;

    bool bypassed() const override;

    void set_bypassed(bool bypassed) override;

    static std::string_view static_uid();

private:
    void _channel_config(int channels);

    void inline _swap_buffers();

    void inline _maybe_swap_buffers(Time current_time);

    float                                 _sample_rate;
    int                                   _return_id;
    SendReturnFactory*                    _manager;

    std::array<ChunkSampleBuffer, 2>      _buffers;
    ChunkSampleBuffer*                    _active_in{&_buffers[0]};
    ChunkSampleBuffer*                    _active_out{&_buffers[1]};

    SpinLock                              _buffer_lock;


    std::vector<send_plugin::SendPlugin*> _senders;

    BypassManager                         _bypass_manager;

    std::atomic<Time>                     _last_process_time{Time(0)};

    static_assert(decltype(_last_process_time)::is_always_lock_free);
};

}// namespace return_plugin
}// namespace sushi



#endif //SUSHI_RETURN_PLUGIN_H
