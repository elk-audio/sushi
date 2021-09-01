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
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <cassert>

#include "track.h"
#include "logging.h"
#include "library/constants.h"

SUSHI_GET_LOGGER_WITH_MODULE_NAME("track");

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

Track::Track(HostControl host_control, int channels,
             performance::PerformanceTimer* timer) : InternalPlugin(host_control),
                                                     _input_buffer{std::max(channels, 2)},
                                                     _output_buffer{std::max(channels, 2)},
                                                     _input_busses{1},
                                                     _output_busses{1},
                                                     _multibus{false},
                                                     _timer{timer}
    {
    _max_input_channels = channels;
    _max_output_channels = std::max(channels, 2);
    _current_input_channels = channels;
    _current_output_channels = channels;
    _common_init();
}

Track::Track(HostControl host_control, int input_busses, int output_busses,
             performance::PerformanceTimer* timer) :  InternalPlugin(host_control),
                                                      _input_buffer{std::max(input_busses, output_busses) * 2},
                                                      _output_buffer{std::max(input_busses, output_busses) * 2},
                                                      _input_busses{input_busses},
                                                      _output_busses{output_busses},
                                                      _multibus{(input_busses > 1 || output_busses > 1)},
                                                      _timer{timer}
{
    int channels = std::max(input_busses, output_busses) * 2;
    _max_input_channels = channels;
    _max_output_channels = channels;
    _current_input_channels = channels;
    _current_output_channels = channels;
    _common_init();
}

ProcessorReturnCode Track::init(float sample_rate)
{
    this->configure(sample_rate);
    return ProcessorReturnCode::OK;
}

void Track::configure(float sample_rate)
{
    for (auto& i : _pan_gain_smoothers_right)
    {
        i.set_lag_time(GAIN_SMOOTHING_TIME, sample_rate / AUDIO_CHUNK_SIZE);
    }
    for (auto& i : _pan_gain_smoothers_left)
    {
        i.set_lag_time(GAIN_SMOOTHING_TIME, sample_rate / AUDIO_CHUNK_SIZE);
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
        _update_channel_config();
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
            _update_channel_config();
            return true;
        }
    }
    return false;
}

void Track::render()
{
    process_audio(_input_buffer, _output_buffer);
    for (int bus = 0; bus < _output_busses; ++bus)
    {
        auto buffer = ChunkSampleBuffer::create_non_owning_buffer(_output_buffer, bus * 2, 2);
        _apply_pan_and_gain(buffer, bus);
    }
}

void Track::process_audio(const ChunkSampleBuffer& /*in*/, ChunkSampleBuffer& out)
{
    auto track_timestamp = _timer->start_timer();
    /* For Tracks, process function is called from render() and the input audio data
     * should be copied to _input_buffer prior to this call.
     * We alias the buffers so we can swap them cheaply, without copying the underlying
     * data, though we can't alias in since it is const, even though it points to
     * _input_buffer  */
    ChunkSampleBuffer aliased_in = ChunkSampleBuffer::create_non_owning_buffer(_input_buffer);
    ChunkSampleBuffer aliased_out = ChunkSampleBuffer::create_non_owning_buffer(out);

    for (auto &processor : _processors)
    {
        auto processor_timestamp = _timer->start_timer();
        while (!_kb_event_buffer.empty())
        {
            RtEvent event;
            if (_kb_event_buffer.pop(event))
            {
                processor->process_event(event);
            }
        }
        ChunkSampleBuffer proc_in = ChunkSampleBuffer::create_non_owning_buffer(aliased_in, 0, processor->input_channels());
        ChunkSampleBuffer proc_out = ChunkSampleBuffer::create_non_owning_buffer(aliased_out, 0, processor->output_channels());
        processor->process_audio(proc_in, proc_out);
        std::swap(aliased_in, aliased_out);
        _timer->stop_timer_rt_safe(processor_timestamp, processor->id());
    }

    int output_channels = _processors.empty() ? _current_output_channels : _processors.back()->output_channels();

    if (output_channels > 0)
    {
        aliased_out.replace(aliased_in);
    }
    else
    {
        aliased_out.clear();
    }

    /* If there are keyboard events not consumed, pass them on upwards so the engine can process them */
    _process_output_events();
    _timer->stop_timer_rt_safe(track_timestamp, this->id());
}

void Track::process_event(const RtEvent& event)
{
    if (is_keyboard_event(event))
    {
        /* Keyboard events are cached so they can be passed on
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

void Track::_common_init()
{
    _processors.reserve(TRACK_MAX_PROCESSORS);

    _gain_parameters.at(0) = register_float_parameter("gain", "Gain", "dB",
                                                         0.0f, -120.0f, 24.0f,
                                                         new dBToLinPreProcessor(-120.0f, 24.0f));

    _pan_parameters.at(0) = register_float_parameter("pan", "Pan", "",
                                                        0.0f, -1.0f, 1.0f,
                                                        nullptr);

    for (int bus = 1 ; bus < _output_busses; ++bus)
    {
        _gain_parameters.at(bus) = register_float_parameter("gain_sub_" + std::to_string(bus), "Gain", "dB",
                                                            0.0f, -120.0f, 24.0f,
                                                            new dBToLinPreProcessor(-120.0f, 24.0f));

        _pan_parameters.at(bus) = register_float_parameter("pan_sub_" + std::to_string(bus), "Pan", "",
                                                           0.0f, -1.0f, 1.0f,
                                                           new FloatParameterPreProcessor(-1.0f, 1.0f));
    }

    for (auto& i : _pan_gain_smoothers_right)
    {
        i.set_direct(DEFAULT_TRACK_GAIN);
    }

    for (auto& i : _pan_gain_smoothers_left)
    {
        i.set_direct(DEFAULT_TRACK_GAIN);
    }
}

void Track::_update_channel_config()
{
    int input_channels = _current_input_channels;
    int output_channels;
    int max_processed_output_channels = _max_output_channels;

    if (_max_input_channels == 1)
    {
        max_processed_output_channels = 1;
    }

    for (unsigned int i = 0; i < _processors.size(); ++i)
    {
        input_channels = std::min(input_channels, _processors[i]->max_input_channels());

        if (input_channels != _processors[i]->input_channels())
        {
            _processors[i]->set_input_channels(input_channels);
        }

        if (i < _processors.size() - 1)
        {
            output_channels = std::min(max_processed_output_channels, std::min(_processors[i]->max_output_channels(),
                                                                      _processors[i+1]->max_input_channels()));
        }
        else
        {
            output_channels = std::min(max_processed_output_channels, std::min(_processors[i]->max_output_channels(),
                                                                      _current_output_channels));
        }

        if (output_channels != _processors[i]->output_channels())
        {
            _processors[i]->set_output_channels(output_channels);
        }

        input_channels = output_channels;
    }

    if (!_processors.empty())
    {
        auto last = _processors.back();
        int track_outputs = std::min(_current_output_channels, last->output_channels());

        if (track_outputs != last->output_channels())
        {
            last->set_output_channels(track_outputs);
        }
    }
}

void Track::_process_output_events()
{
    while (!_kb_event_buffer.empty())
    {
        RtEvent event;
        if (_kb_event_buffer.pop(event))
        {
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
                                                                event.keyboard_event()->channel(),
                                                                event.keyboard_common_event()->value()));
                    break;
                case RtEventType::PITCH_BEND:
                    output_event(RtEvent::make_pitch_bend_event(id(), event.sample_offset(),
                                                                event.keyboard_event()->channel(),
                                                                event.keyboard_common_event()->value()));
                    break;
                case RtEventType::MODULATION:
                    output_event(RtEvent::make_kb_modulation_event(id(), event.sample_offset(),
                                                                   event.keyboard_event()->channel(),
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
    }
}

void Track::_apply_pan_and_gain(ChunkSampleBuffer& buffer, int bus)
{
    float gain = _gain_parameters[bus]->processed_value();
    float pan = _pan_parameters[bus]->processed_value();
    auto [left_gain, right_gain] = calc_l_r_gain(gain, pan);
    _pan_gain_smoothers_left[bus].set(left_gain);
    _pan_gain_smoothers_right[bus].set(right_gain);

    ChunkSampleBuffer left = ChunkSampleBuffer::create_non_owning_buffer(buffer, LEFT_CHANNEL_INDEX, 1);
    ChunkSampleBuffer right = ChunkSampleBuffer::create_non_owning_buffer(buffer, RIGHT_CHANNEL_INDEX, 1);

    if (_current_input_channels == 1)
    {
        right.replace(left);
    }

    if (_pan_gain_smoothers_left[bus].stationary() && _pan_gain_smoothers_right[bus].stationary())
    {
        left.apply_gain(left_gain);
        right.apply_gain(right_gain);
    }
    else // value needs smoothing
    {
        left.ramp(_pan_gain_smoothers_left[bus].value(), _pan_gain_smoothers_left[bus].next_value());
        right.ramp(_pan_gain_smoothers_right[bus].value(), _pan_gain_smoothers_right[bus].next_value());
    }
}

} // namespace engine
} // namespace sushi
