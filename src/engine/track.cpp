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
 * @brief Class to represent a mixer track with a chain of processors
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <cassert>

#include "track.h"
#include "library/constants.h"

namespace sushi {
namespace engine {

constexpr int TRACK_MAX_PROCESSORS = 32;
constexpr float PAN_GAIN_3_DB = 1.412537f;
constexpr float DEFAULT_TRACK_GAIN = 1.0f;

/* Map pan and gain to left and right gain with a 3 dB pan law */
inline std::pair<float, float> calc_l_r_gain(float gain, float pan)
{
    float left_gain, right_gain;
    if (pan < 0.0f) // Audio panned left
    {
        left_gain = gain * (1.0f + pan - PAN_GAIN_3_DB * pan);
        right_gain = gain * (1.0f + pan);
    }
    else            // Audio panned right
    {
        left_gain = gain * (1.0f - pan);
        right_gain = gain * (1.0f - pan + PAN_GAIN_3_DB * pan);
    }
    return {left_gain, right_gain};
}

Track::Track(HostControl host_control,
             int channels,
             performance::PerformanceTimer* timer,
             bool pan_controls,
             TrackType type) : InternalPlugin(host_control),
                                  _input_buffer{std::max(channels, 2)},
                                  _output_buffer{std::max(channels, 2)},
                                  _buses{1},
                                  _type{type},
                                  _timer{timer}
{
    _max_input_channels = channels;
    _max_output_channels = std::max(channels, 2);
    _current_input_channels = channels;
    _current_output_channels = channels;
    _common_init((pan_controls && channels <= 2) ? PanMode::PAN_AND_GAIN : PanMode::GAIN_ONLY);
}

Track::Track(HostControl host_control, int buses, performance::PerformanceTimer* timer) : InternalPlugin(host_control),
                                                                                          _input_buffer{buses * 2},
                                                                                          _output_buffer{buses * 2},
                                                                                          _buses{buses},
                                                                                          _type{TrackType::REGULAR},
                                                                                          _timer{timer}
{
    int channels = buses * 2;
    _max_input_channels = channels;
    _max_output_channels = channels;
    _current_input_channels = channels;
    _current_output_channels = channels;
    _common_init(PanMode::PAN_AND_GAIN_PER_BUS);
}

ProcessorReturnCode Track::init(float sample_rate)
{
    this->configure(sample_rate);
    return ProcessorReturnCode::OK;
}

void Track::configure(float sample_rate)
{
    for (auto& i : _smoothers)
    {
        i[LEFT_CHANNEL_INDEX].set_lag_time(GAIN_SMOOTHING_TIME, sample_rate / AUDIO_CHUNK_SIZE);
        i[RIGHT_CHANNEL_INDEX].set_lag_time(GAIN_SMOOTHING_TIME, sample_rate / AUDIO_CHUNK_SIZE);
    }
}

bool Track::add(Processor* processor, std::optional<ObjectId> before_position)
{
    if (_processors.size() >= TRACK_MAX_PROCESSORS || processor == this)
    {
        // If a track adds itself to its process chain, endless loops can arise
        // In addition, _processors must not allocate if running in the rt-thread
        return false;
    }
    assert(processor->active_rt_processing() == false);

    bool added = false;
    if (before_position.has_value())
    {
        for (auto i = _processors.cbegin(); i != _processors.cend(); ++i)
        {
            if ((*i)->id() == *before_position) // * accesses value without throwing
            {
                _processors.insert(i, processor);
                added = true;
                break;
            }
        }
    }
    else
    {
        _processors.push_back(processor);
        added = true;
    }

    if (added)
    {
        processor->set_event_output(this);
        processor->set_active_rt_processing(true);
    }
    return added;
}

bool Track::remove(ObjectId processor)
{
    for (auto i = _processors.begin(); i != _processors.end(); ++i)
    {
        if ((*i)->id() == processor)
        {
            (*i)->set_event_output(nullptr);
            (*i)->set_active_rt_processing(false);
            _processors.erase(i);
            return true;
        }
    }
    return false;
}

void Track::render()
{
    process_audio(_input_buffer, _output_buffer);
    _input_buffer.clear();
}

void Track::process_audio(const ChunkSampleBuffer& in, ChunkSampleBuffer& out)
{
    /* For Tracks, process function is called from render() and the input audio data
     * should be copied to _input_buffer prior to this call. */

    auto track_timestamp = _timer->start_timer();

    /* Process all the plugins in the chain, to guarantee that memory declared const is never
     * written to, the const cast below is only done if in already points to _input_buffer
     * (which is the case if process_audio() is called from render()), or if there is max 1
     * plugin in the chain, in which case there will be no ping-pong copying between buffers. */
    if (in.channel(0) == _input_buffer.channel(0) || _processors.size() <= 1)
    {
        _process_plugins(const_cast<ChunkSampleBuffer&>(in), out);
    }
    else
    {
        _input_buffer.replace(in);
        _process_plugins(_input_buffer, out);
    }

    /* If there are keyboard events not consumed, pass them on upwards so the engine can process them */
    _process_output_events();

    bool muted = _mute_parameter->processed_value();

    switch (_pan_mode)
    {
        case PanMode::GAIN_ONLY:
            _apply_gain(out, muted);
            break;

        case PanMode::PAN_AND_GAIN:
            _apply_pan_and_gain(out, muted);
            break;

        case PanMode::PAN_AND_GAIN_PER_BUS:
            _apply_pan_and_gain_per_bus(out, muted);
            break;
    }

    _timer->stop_timer_rt_safe(track_timestamp, this->id());
}

void Track::process_event(const RtEvent& event)
{
    if (is_keyboard_event(event))
    {
        /* Keyboard events are cached, so they can be passed on
         * to the next processor in the track */
        _kb_event_buffer.push(event);
    }
    else
    {
        InternalPlugin::process_event(event);
    }
}

void Track::set_bypassed(bool bypassed)
{
    for (auto& processor : _processors)
    {
        processor->set_bypassed(bypassed);
    }

    Processor::set_bypassed(bypassed);
}

void Track::send_event(const RtEvent& event)
{
    if (is_keyboard_event(event))
    {
        _kb_event_buffer.push(event);
    }
    else
    {
        output_event(event);
    }
}

void Track::_common_init(PanMode mode)
{
    _processors.reserve(TRACK_MAX_PROCESSORS);
    _pan_mode = mode;

    _gain_parameters.at(0) = register_float_parameter("gain", "Gain", "dB",
                                                      0.0f, -120.0f, 24.0f,
                                                      Direction::AUTOMATABLE,
                                                      new dBToLinPreProcessor(-120.0f, 24.0f));
    _smoothers.emplace_back();

    if (mode == PanMode::PAN_AND_GAIN || mode == PanMode::PAN_AND_GAIN_PER_BUS)
    {
        _pan_parameters.at(0) = register_float_parameter("pan", "Pan", "",
                                                         0.0f, -1.0f, 1.0f,
                                                         Direction::AUTOMATABLE,
                                                         nullptr);
    }
    _mute_parameter = register_bool_parameter("mute", "Mute", "", false, Direction::AUTOMATABLE);

    if (mode == PanMode::PAN_AND_GAIN_PER_BUS)
    {
        for (int bus = 1; bus < _buses; ++bus)
        {
            _gain_parameters.at(bus) = register_float_parameter("gain_sub_" + std::to_string(bus), "Gain", "dB",
                                                                0.0f, -120.0f, 24.0f,
                                                                Direction::AUTOMATABLE,
                                                                new dBToLinPreProcessor(-120.0f, 24.0f));

            _pan_parameters.at(bus) = register_float_parameter("pan_sub_" + std::to_string(bus), "Pan", "",
                                                               0.0f, -1.0f, 1.0f,
                                                               Direction::AUTOMATABLE,
                                                               new FloatParameterPreProcessor(-1.0f, 1.0f));
            _smoothers.emplace_back();
        }
    }

    for (auto& i : _smoothers)
    {
        i[LEFT_CHANNEL_INDEX].set_direct(DEFAULT_TRACK_GAIN);
        i[RIGHT_CHANNEL_INDEX].set_direct(DEFAULT_TRACK_GAIN);
    }
}

void Track::_process_plugins(ChunkSampleBuffer& in, ChunkSampleBuffer& out)
{
    /* Alias the buffers, so we can swap them cheaply, without copying the underlying data */

    ChunkSampleBuffer aliased_in = ChunkSampleBuffer::create_non_owning_buffer(in);
    ChunkSampleBuffer aliased_out = ChunkSampleBuffer::create_non_owning_buffer(out);

    for (auto &processor : _processors)
    {
        auto processor_timestamp = _timer->start_timer();
        /* Note that processors can put events back into this queue, hence we're not draining the queue
         * but checking the size first to avoid an infinite loop */
        for (int kb_events = _kb_event_buffer.size(); kb_events > 0; --kb_events)
        {
            processor->process_event(_kb_event_buffer.pop());
        }

        ChunkSampleBuffer proc_in = ChunkSampleBuffer::create_non_owning_buffer(aliased_in, 0, processor->input_channels());
        ChunkSampleBuffer proc_out = ChunkSampleBuffer::create_non_owning_buffer(aliased_out, 0, processor->output_channels());
        processor->process_audio(proc_in, proc_out);

        int unused_channels = aliased_out.channel_count() - processor->output_channels();
        if (unused_channels > 0)
        {
            // If processor has fewer channels than the track, zero the rest to avoid passing garbage to the next processor
            auto unused = ChunkSampleBuffer::create_non_owning_buffer(aliased_out, aliased_out.channel_count() - unused_channels, unused_channels);
            unused.clear();
        }

        swap(aliased_in, aliased_out);
        _timer->stop_timer_rt_safe(processor_timestamp, static_cast<int>(processor->id()));
    }

    int output_channels = _processors.empty() ? _current_output_channels : _processors.back()->output_channels();

    if (output_channels > 0)
    {
        /* aliased_out contains the output of the last processor. If the number of processors
         * is even, then aliased_out already points to out, otherwise we need to copy to it */
        if (aliased_in.channel(0) == in.channel(0))
        {
            out.replace(aliased_in);
        }
    }
    else
    {
        out.clear();
    }
}

void Track::_process_output_events()
{
    while (!_kb_event_buffer.empty())
    {
        const RtEvent& event = _kb_event_buffer.pop();
        switch (event.type())
        {
            case RtEventType::NOTE_ON:
                output_event(RtEvent::make_note_on_event(id(), event.sample_offset(),
                                                         event.keyboard_event()->channel(),
                                                         event.keyboard_event()->note(),
                                                         event.keyboard_event()->velocity()));
                break;
            case RtEventType::NOTE_OFF:
                output_event(RtEvent::make_note_off_event(id(), event.sample_offset(),
                                                          event.keyboard_event()->channel(),
                                                          event.keyboard_event()->note(),
                                                          event.keyboard_event()->velocity()));
                break;
            case RtEventType::NOTE_AFTERTOUCH:
                output_event(RtEvent::make_note_aftertouch_event(id(), event.sample_offset(),
                                                                 event.keyboard_event()->channel(),
                                                                 event.keyboard_event()->note(),
                                                                 event.keyboard_event()->velocity()));
                break;
            case RtEventType::AFTERTOUCH:
                output_event(RtEvent::make_aftertouch_event(id(), event.sample_offset(),
                                                            event.keyboard_common_event()->channel(),
                                                            event.keyboard_common_event()->value()));
                break;
            case RtEventType::PITCH_BEND:
                output_event(RtEvent::make_pitch_bend_event(id(), event.sample_offset(),
                                                            event.keyboard_common_event()->channel(),
                                                            event.keyboard_common_event()->value()));
                break;
            case RtEventType::MODULATION:
                output_event(RtEvent::make_kb_modulation_event(id(), event.sample_offset(),
                                                               event.keyboard_common_event()->channel(),
                                                               event.keyboard_common_event()->value()));
                break;
            case RtEventType::WRAPPED_MIDI_EVENT:
                output_event(RtEvent::make_wrapped_midi_event(id(), event.sample_offset(),
                                                              event.wrapped_midi_event()->midi_data()));
                break;

            default:
                output_event(event);
        }
    }
    _kb_event_buffer.clear(); // Reset the read & write index to reuse the same memory area every time.
}

void Track::_apply_pan_and_gain(ChunkSampleBuffer& buffer, bool muted)
{
    assert(buffer.channel_count() <= 2);

    float gain = muted ? 0.0f : _gain_parameters.front()->processed_value();
    float pan = _pan_parameters.front()->processed_value();
    auto[left_gain, right_gain] = calc_l_r_gain(gain, pan);

    auto& left_smoother = _smoothers.front()[LEFT_CHANNEL_INDEX];
    auto& right_smoother = _smoothers.front()[RIGHT_CHANNEL_INDEX];
    left_smoother.set(left_gain);
    right_smoother.set(right_gain);

    ChunkSampleBuffer left = ChunkSampleBuffer::create_non_owning_buffer(buffer, LEFT_CHANNEL_INDEX, 1);
    ChunkSampleBuffer right = ChunkSampleBuffer::create_non_owning_buffer(buffer, RIGHT_CHANNEL_INDEX, 1);

    if (_current_input_channels == 1)
    {
        right.replace(left);
    }

    if (left_smoother.stationary() && left_smoother.stationary())
    {
        left.apply_gain(left_gain);
        right.apply_gain(right_gain);
    }
    else // Value needs smoothing
    {
        left.ramp(left_smoother.value(), left_smoother.next_value());
        right.ramp(right_smoother.value(), right_smoother.next_value());
    }
}

void Track::_apply_pan_and_gain_per_bus(ChunkSampleBuffer& buffer, bool muted)
{
    for (int bus = 0; bus < _buses; ++bus)
    {
        auto bus_buffer = ChunkSampleBuffer::create_non_owning_buffer(buffer, bus * 2, 2);
        float gain = muted ? 0.0f : _gain_parameters[bus]->processed_value();
        float pan = _pan_parameters[bus]->processed_value();
        auto[left_gain, right_gain] = calc_l_r_gain(gain, pan);
        auto& left_smoother = _smoothers[bus][LEFT_CHANNEL_INDEX];
        auto& right_smoother = _smoothers[bus][RIGHT_CHANNEL_INDEX];

        left_smoother.set(left_gain);
        right_smoother.set(right_gain);

        ChunkSampleBuffer left = ChunkSampleBuffer::create_non_owning_buffer(bus_buffer, LEFT_CHANNEL_INDEX, 1);
        ChunkSampleBuffer right = ChunkSampleBuffer::create_non_owning_buffer(bus_buffer, RIGHT_CHANNEL_INDEX, 1);

        if (left_smoother.stationary() && left_smoother.stationary())
        {
            left.apply_gain(left_gain);
            right.apply_gain(right_gain);
        }
        else // Value needs smoothing
        {
            left.ramp(left_smoother.value(), left_smoother.next_value());
            right.ramp(right_smoother.value(), right_smoother.next_value());
        }
    }
}

void Track::_apply_gain(ChunkSampleBuffer& buffer, bool muted)
{
    float gain = muted ? 0.0f : _gain_parameters.front()->processed_value();

    auto& gain_smoother = _smoothers.front()[LEFT_CHANNEL_INDEX];
    gain_smoother.set(gain);

    if (gain_smoother.stationary())
    {
        buffer.apply_gain(gain);
    }
    else // Value needs smoothing
    {
        buffer.ramp(gain_smoother.value(), gain_smoother.next_value());
    }
}

} // namespace engine
} // namespace sushi
