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

#ifndef SUSHI_SEND_PLUGIN_H
#define SUSHI_SEND_PLUGIN_H

#include <atomic>

#include "send_return_factory.h"
#include "library/internal_plugin.h"
#include "library/constants.h"

namespace sushi {

class SendReturnFactory;
namespace return_plugin { class ReturnPlugin; }

constexpr int MAX_SEND_CHANNELS = MAX_TRACK_CHANNELS;

namespace send_plugin {

class SendPlugin : public InternalPlugin, public UidHelper<SendPlugin>
{
public:
    SendPlugin(HostControl host_control, SendReturnFactory* manager);

    virtual ~SendPlugin();

    void clear_destination();

    // From Processor
    ProcessorReturnCode init(float sample_rate) override;

    void configure(float sample_rate) override;

    void process_event(const RtEvent& event) override;

    void process_audio(const ChunkSampleBuffer& in_buffer, ChunkSampleBuffer& out_buffer) override;

    bool bypassed() const override;

    void set_bypassed(bool bypassed) override;

    ProcessorReturnCode set_property_value(ObjectId property_id, const std::string& value) override;

    static std::string_view static_uid();

private:
    void _set_destination(return_plugin::ReturnPlugin* destination);

    void _change_return_destination(const std::string& dest_name);

    float                         _sample_rate;
    return_plugin::ReturnPlugin*  _destination{nullptr};

    FloatParameterValue*          _gain_parameter;
    ValueSmootherFilter<float>    _gain_smoother;

    IntParameterValue*            _channel_count_parameter;
    IntParameterValue*            _start_channel_parameter;
    IntParameterValue*            _dest_channel_parameter;

    SendReturnFactory* _manager;
    BypassManager _bypass_manager;
};

} // namespace send_plugin
} // namespace sushi

#endif //SUSHI_SEND_PLUGIN_H
