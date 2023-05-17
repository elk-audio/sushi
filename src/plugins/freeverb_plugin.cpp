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
 * @brief Wrapper for Freeverb plugin
 * @copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include <cassert>
#include <memory>

#include "freeverb_plugin.h"

namespace sushi {
namespace freeverb_plugin {

constexpr auto PLUGIN_UID = "sushi.testing.freeverb";
constexpr auto DEFAULT_LABEL = "Freeverb";


FreeverbPlugin::FreeverbPlugin(HostControl host_control) : InternalPlugin(host_control)
{
    _max_input_channels = 2;
    _max_output_channels = 2;
    Processor::set_name(PLUGIN_UID);
    Processor::set_label(DEFAULT_LABEL);

    _freeze = register_bool_parameter("freeze", "Freeze", "",
                                      false, Direction::AUTOMATABLE);
    _room_size = register_float_parameter("room_size", "Room Size", "",
                                          0.5f, 0.0f, 1.0f,
                                          Direction::AUTOMATABLE,
                                          new FloatParameterPreProcessor(0.0f, 1.0f));
    _width = register_float_parameter("width", "Width", "",
                                      0.5f, 0.0f, 1.0f,
                                      Direction::AUTOMATABLE,
                                      new FloatParameterPreProcessor(0.0f, 1.0f));
    _damp = register_float_parameter("damp", "Damping", "",
                                     0.5f, 0.0f, 1.0f,
                                     Direction::AUTOMATABLE,
                                     new FloatParameterPreProcessor(0.0f, 1.0f));
    _dry_wet = register_float_parameter("dry_wet", "Dry/Wet Mix", "",
                                        0.5f, 0.0f, 1.0f,
                                        Direction::AUTOMATABLE,
                                        new FloatParameterPreProcessor(0.0f, 1.0f));

    _reverb_model = std::make_unique<revmodel>();

    assert(_freeze);
    assert(_room_size);
    assert(_width);
    assert(_damp);
    assert(_dry_wet);
}

ProcessorReturnCode FreeverbPlugin::init(float sample_rate)
{
    configure(sample_rate);
    return ProcessorReturnCode::OK;
}

void FreeverbPlugin::configure(float sample_rate)
{
    _sample_rate = sample_rate;
    return;
}

void FreeverbPlugin::set_enabled(bool enabled)
{
    Processor::set_enabled(enabled);
}

void FreeverbPlugin::set_bypassed(bool bypassed)
{
    _host_control.post_event(new SetProcessorBypassEvent(this->id(), bypassed, IMMEDIATE_PROCESS));
}

void FreeverbPlugin::process_event(const RtEvent& event)
{
    switch (event.type())
    {
    case RtEventType::SET_BYPASS:
    {
        bool bypassed = static_cast<bool>(event.processor_command_event()->value());
        InternalPlugin::set_bypassed(bypassed);
        _bypass_manager.set_bypass(bypassed, _sample_rate);
        break;
    }

    case RtEventType::BOOL_PARAMETER_CHANGE:
    {
        InternalPlugin::process_event(event);
        auto typed_event = event.parameter_change_event();
        if (typed_event->param_id() == _freeze->descriptor()->id())
        {
            if (_freeze->processed_value())
            {
                _reverb_model->setmode(1.0f);
            }
            else
            {
                _reverb_model->setmode(0.0f);
            }
        }
        break;
    }

    case RtEventType::FLOAT_PARAMETER_CHANGE:
    {
        InternalPlugin::process_event(event);
        auto typed_event = event.parameter_change_event();
        auto value = typed_event->value();
        if (typed_event->param_id() == _dry_wet->descriptor()->id())
        {
            _reverb_model->setwet(value);
            _reverb_model->setdry(1.0f - value);
        }
        else if (typed_event->param_id() == _room_size->descriptor()->id())
        {
            _reverb_model->setroomsize(value);
        }
        else if (typed_event->param_id() == _width->descriptor()->id())
        {
            _reverb_model->setwidth(value);
        }
        else if (typed_event->param_id() == _damp->descriptor()->id())
        {
            _reverb_model->setdamp(value);
        }
        break;
    }

    default:
        InternalPlugin::process_event(event);
        break;
    }
}

void FreeverbPlugin::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    if (_bypass_manager.should_process())
    {
        float* input_l = const_cast<float*>(in_buffer.channel(0));
        // reuse the same buffer if we have only one channel
        float* input_r = const_cast<float*>(in_buffer.channel(0));
        if (_current_input_channels > 1)
        {
            input_r = const_cast<float*>(in_buffer.channel(1));
        }
        float* output_l = out_buffer.channel(0);
        // reuse the same buffer if we have only one channel
        float* output_r = out_buffer.channel(0);
        if (_current_output_channels > 1)
        {
            output_r = out_buffer.channel(1);
        }

        _reverb_model->processreplace(input_l, input_r, output_l, output_r, AUDIO_CHUNK_SIZE, 1);

        if (_bypass_manager.should_ramp())
        {
            _bypass_manager.crossfade_output(in_buffer, out_buffer,
                                             _current_input_channels,
                                             _current_output_channels);
        }
    }
    else
    {
        bypass_process(in_buffer, out_buffer);
    }
}

std::string_view FreeverbPlugin::static_uid()
{
    return PLUGIN_UID;
}


}// namespace freeverb_plugin
}// namespace sushi

