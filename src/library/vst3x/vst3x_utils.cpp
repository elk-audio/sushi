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
 * @brief Helper classes for VST 3.x plugins.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "vst3x_utils.h"

namespace sushi {
namespace vst3{

void SushiProcessData::assign_buffers(const ChunkSampleBuffer& input, ChunkSampleBuffer& output,
                                      int in_channels, int out_channels)
{
    assert(input.channel_count() <= VST_WRAPPER_MAX_N_CHANNELS &&
           output.channel_count() <= VST_WRAPPER_MAX_N_CHANNELS);
    for (int i = 0; i < input.channel_count(); ++i)
    {
        _process_inputs[i] = const_cast<float*>(input.channel(i));
    }
    for (int i = 0; i < output.channel_count(); ++i)
    {
        _process_outputs[i] = output.channel(i);
    }
    inputs->numChannels = in_channels;
    outputs->numChannels = out_channels;
}

Steinberg::Vst::Event convert_note_on_event(const KeyboardRtEvent* event)
{
    assert(event->type() == RtEventType::NOTE_ON);
    Steinberg::Vst::Event vst_event;
    vst_event.busIndex = 0;
    vst_event.sampleOffset = event->sample_offset();
    vst_event.ppqPosition = 0;
    vst_event.flags = 0;
    vst_event.type = Steinberg::Vst::Event::kNoteOnEvent;
    vst_event.noteOn.channel = static_cast<Steinberg::int16>(event->channel());
    vst_event.noteOn.pitch = static_cast<Steinberg::int16>(event->note());
    vst_event.noteOn.tuning = 0.0f;
    vst_event.noteOn.velocity = event->velocity();
    vst_event.noteOn.length = 0;
    vst_event.noteOn.noteId = -1;
    return vst_event;
}

Steinberg::Vst::Event convert_note_off_event(const KeyboardRtEvent* event)
{
    assert(event->type() == RtEventType::NOTE_OFF);
    Steinberg::Vst::Event vst_event;
    vst_event.busIndex = 0;
    vst_event.sampleOffset = event->sample_offset();
    vst_event.ppqPosition = 0;
    vst_event.flags = 0;
    vst_event.type = Steinberg::Vst::Event::kNoteOffEvent;
    vst_event.noteOff.channel = static_cast<Steinberg::int16>(event->channel());
    vst_event.noteOff.pitch = static_cast<Steinberg::int16>(event->note());
    vst_event.noteOff.velocity = event->velocity();
    vst_event.noteOff.noteId = -1;
    vst_event.noteOff.tuning = 0.0f;
    return vst_event;
}

Steinberg::Vst::Event convert_aftertouch_event(const KeyboardRtEvent* event)
{
    assert(event->type() == RtEventType::NOTE_AFTERTOUCH);
    Steinberg::Vst::Event vst_event;
    vst_event.busIndex = 0;
    vst_event.sampleOffset = event->sample_offset();
    vst_event.ppqPosition = 0;
    vst_event.flags = 0;
    vst_event.type = Steinberg::Vst::Event::kPolyPressureEvent;
    vst_event.polyPressure.channel = static_cast<Steinberg::int16>(event->channel());
    vst_event.polyPressure.pitch = static_cast<Steinberg::int16>(event->note());
    vst_event.polyPressure.pressure = event->velocity();
    vst_event.polyPressure.noteId = -1;
    return vst_event;
}

void StateParamValue::set_values(const ObjectId& id, const float& value)
{
    _id = id;
    _value = value;
}

Steinberg::tresult StateParamValue::getPoint(Steinberg::int32 /*id*/,
                                             Steinberg::int32& sampleOffset,
                                             Steinberg::Vst::ParamValue& value)
{
    sampleOffset = 0;
    value = _value;
    return Steinberg::kResultOk;
}

Steinberg::int32 Vst3xRtState::getParameterCount()
{
    return _parameter_changes.size();
}


Steinberg::Vst::IParamValueQueue* Vst3xRtState::getParameterData(Steinberg::int32 index)
{
    if (static_cast<size_t>(index) >= _parameter_changes.size())
    {
        return nullptr;
    }
    const auto& value = _parameter_changes[index];
    _transfer_value.set_values(value.first, value.second);
    return &_transfer_value;
}

Steinberg::Vst::IParamValueQueue* Vst3xRtState::addParameterData(const Steinberg::Vst::ParamID& /*id*/,
                                                                 Steinberg::int32& /*index*/)
{
    return nullptr;
}

} // end namespace vst3
} // end namespace sushi
