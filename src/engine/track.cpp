
#include "track.h"
#include "logging.h"
#include <string>

MIND_GET_LOGGER;

namespace sushi {
namespace engine {


Track::Track(int channels) : _input_buffer{std::max(channels, 2)},
                             _output_buffer{std::max(channels, 2)},
                             _input_busses{1},
                             _output_busses{1},
                             _multibus{false}
{
    _max_input_channels = channels;
    _max_output_channels = channels;
    _current_input_channels = channels;
    _current_output_channels = channels;
    _common_init();
}

Track::Track(int input_busses, int output_busses) :  _input_buffer{std::max(input_busses, output_busses) * 2},
                                                     _output_buffer{std::max(input_busses, output_busses) * 2},
                                                     _input_busses{input_busses},
                                                     _output_busses{output_busses},
                                                     _multibus{(input_busses > 1 || output_busses > 1)}
{
    int channels = std::max(input_busses, output_busses) * 2;
    _max_input_channels = channels;
    _max_output_channels = channels;
    _current_input_channels = channels;
    _current_output_channels = channels;
    _common_init();
}


bool Track::add(Processor* processor)
{
    if (_processors.size() >= TRACK_MAX_PROCESSORS || processor == this)
    {
        // If a track adds itself to its process chain, endless loops can arrise
        return false;
    }
    _processors.push_back(processor);
    processor->set_event_output(this);
    _update_channel_config();
    return true;
}

bool Track::remove(ObjectId processor)
{
    for (auto plugin = _processors.begin(); plugin != _processors.end(); ++plugin)
    {
        if ((*plugin)->id() == processor)
        {
            (*plugin)->set_event_output(nullptr);
            _processors.erase(plugin);
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
        apply_pan_and_gain(buffer, _gain_parameters[bus]->value(), _pan_parameters[bus]->value());
    }
}

void Track::process_audio(const ChunkSampleBuffer& /*in*/, ChunkSampleBuffer& out)
{
    /* For Tracks, process function is called from render() and the input audio data
     * should be copied to _input_buffer prior to this call.
     * We alias the buffers so we can swap them cheaply, without copying the underlying
     * data, though we can't alias in since it is const, even though it points to
     * _input_buffer  */
    ChunkSampleBuffer aliased_in = ChunkSampleBuffer::create_non_owning_buffer(_input_buffer);
    ChunkSampleBuffer aliased_out = ChunkSampleBuffer::create_non_owning_buffer(out);

    for (auto &processor : _processors)
    {
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
    }
    int output_channels = _processors.empty() ? _current_output_channels : _processors.back()->output_channels();
    if (output_channels > 0)
    {
        _output_buffer.replace(_input_buffer);
    }
    else
    {
        _output_buffer.clear();
    }

    /* If there are keyboard events not consumed, pass them on upwards so the engine can process them */
    _process_output_events();
}

void Track::process_event(RtEvent event)
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

void Track::send_event(RtEvent event)
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
    _gain_parameters[0]  = register_float_parameter("gain", "Gain", 0.0f, -120.0f, 24.0f, new dBToLinPreProcessor(-120.0f, 24.0f));
    _pan_parameters[0]  = register_float_parameter("pan", "Pan", 0.0f, -1.0f, 1.0f, new FloatParameterPreProcessor(-1.0f, 1.0f));
    for (int bus = 1 ; bus < _output_busses; ++bus)
    {
        _gain_parameters[bus]  = register_float_parameter("gain_sub_" + std::to_string(bus), "Gain", 0.0f, -120.0f, 24.0f, new dBToLinPreProcessor(-120.0f, 24.0f));
        _pan_parameters[bus]  = register_float_parameter("pan_sub_" + std::to_string(bus), "Pan", 0.0f, -1.0f, 1.0f, new FloatParameterPreProcessor(-1.0f, 1.0f));
    }
}

void Track::_update_channel_config()
{
    int input_channels = _current_input_channels;
    int output_channels;

    for (unsigned int i = 0; i < _processors.size(); ++i)
    {
        input_channels = std::min(input_channels, _processors[i]->max_input_channels());
        if (input_channels != _processors[i]->input_channels())
        {
            _processors[i]->set_input_channels(input_channels);
        }
        if (i < _processors.size() - 1)
        {
            output_channels = std::min(_max_output_channels, std::min(_processors[i]->max_output_channels(),
                                                                      _processors[i+1]->max_input_channels()));
        }
        else
        {
            output_channels = std::min(_max_output_channels, std::min(_processors[i]->max_output_channels(),
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
                                                             event.keyboard_event()->note(),
                                                             event.keyboard_event()->velocity()));
                    break;
                case RtEventType::NOTE_OFF:
                    output_event(RtEvent::make_note_off_event(id(), event.sample_offset(),
                                                              event.keyboard_event()->note(),
                                                              event.keyboard_event()->velocity()));
                    break;
                case RtEventType::NOTE_AFTERTOUCH:
                    output_event(RtEvent::make_note_aftertouch_event(id(), event.sample_offset(),
                                                                     event.keyboard_event()->note(),
                                                                     event.keyboard_event()->velocity()));
                    break;
                case RtEventType::AFTERTOUCH:
                    output_event(RtEvent::make_aftertouch_event(id(), event.sample_offset(),
                                                                event.keyboard_common_event()->value()));
                    break;
                case RtEventType::PITCH_BEND:
                    output_event(RtEvent::make_pitch_bend_event(id(), event.sample_offset(),
                                                                event.keyboard_common_event()->value()));
                    break;
                case RtEventType::MODULATION:
                    output_event(RtEvent::make_kb_modulation_event(id(), event.sample_offset(),
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

void apply_pan_and_gain(ChunkSampleBuffer& buffer, float gain, float pan)
{
    float left_gain, right_gain;
    ChunkSampleBuffer left = ChunkSampleBuffer::create_non_owning_buffer(buffer, LEFT_CHANNEL_INDEX, 1);
    ChunkSampleBuffer right = ChunkSampleBuffer::create_non_owning_buffer(buffer, RIGHT_CHANNEL_INDEX, 1);
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
    left.apply_gain(left_gain);
    right.apply_gain(right_gain);
}


} // namespace engine
} // namespace sushi
