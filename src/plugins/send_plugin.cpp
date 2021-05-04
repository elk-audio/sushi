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
 * @brief Aux send plugin to send audio to a return plugin
 * @copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "send_plugin.h"
#include "return_plugin.h"

namespace sushi {
namespace send_plugin {

constexpr auto DEFAULT_NAME = "send";
constexpr auto DEFAULT_LABEL = "Send";

SendPlugin::SendPlugin(HostControl host_control, SendReturnFactory* manager) : InternalPlugin(host_control),
                                                                               _manager(manager)
{
    Processor::set_name(DEFAULT_NAME);
    Processor::set_label(DEFAULT_LABEL);
    _gain_parameter  = register_float_parameter("gain", "Gain", "dB",
                                                0.0f, -120.0f, 24.0f,
                                                new dBToLinPreProcessor(-120.0f, 24.0f));

    [[maybe_unused]] bool str_pr_ok = register_string_property("destination_name", "destination name", "");
    assert(_gain_parameter && str_pr_ok);
}

SendPlugin::~SendPlugin()
{
    if (_destination)
    {
        _destination->remove_sender(this);
    }
}

void SendPlugin::set_destination(return_plugin::ReturnPlugin* destination)
{
    _destination = destination; // TODO - When nulling this - make sure that it is not used afterwards
    if (destination)
    {
        destination->add_sender(this);
    }
}

ProcessorReturnCode SendPlugin::init(float sample_rate)
{
    configure(sample_rate);
    return ProcessorReturnCode::OK;
}

void SendPlugin::configure(float sample_rate)
{
    _sample_rate = sample_rate;
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
    if (_bypass_manager.should_process())
    {
        if (_destination)
        {
            _destination->send_audio(in_buffer, _gain_parameter->processed_value());
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

}// namespace send_plugin
}// namespace sushi