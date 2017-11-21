
#include "track.h"
#include "logging.h"
#include <iostream>

MIND_GET_LOGGER;

namespace sushi {
namespace engine {


void Track::add(Processor* processor)
{
    // If a track adds itself to its process track, endless loops can arrise
    assert(processor != this);
    _processors.push_back(processor);
    processor->set_event_output(this);
    update_channel_config();
}

bool Track::remove(ObjectId processor)
{
    for (auto plugin = _processors.begin(); plugin != _processors.end(); ++plugin)
    {
        if ((*plugin)->id() == processor)
        {
            (*plugin)->set_event_output(nullptr);
            _processors.erase(plugin);
            update_channel_config();
            return true;
        }
    }
    return false;
}

void Track::process_audio(const ChunkSampleBuffer& in, ChunkSampleBuffer& out)
{
    /* Alias the internal buffers to get the right channel count */
    ChunkSampleBuffer in_bfr = ChunkSampleBuffer::create_non_owning_buffer(_bfr_1, 0, _current_input_channels);
    ChunkSampleBuffer out_bfr = ChunkSampleBuffer::create_non_owning_buffer(_bfr_2, 0, _current_input_channels);
    in_bfr.clear();
    in_bfr.add(in);

    for (auto &plugin : _processors)
    {
        while (!_event_buffer.empty()) // This should only contain keyboard/note events
        {
            RtEvent event;
            if (_event_buffer.pop(event))
            {
                plugin->process_event(event);
            }
        }
        plugin->process_audio(in_bfr, out_bfr);
        std::swap(in_bfr, out_bfr);
    }

    /* Yes, it is in_bfr buffer here. Either it was swapped with out_bfr or the
     * processing track was empty */
    out = in_bfr;
    /* If there are keyboard events not consumed, pass them on upwards
     * Rewrite the processor id of the events with that of the track.
     * Eventually this should only be done for track objects */
    while (!_event_buffer.empty())
    {
        RtEvent event;
        if (_event_buffer.pop(event))
        {
            switch (event.type())
            {
                case RtEventType::NOTE_ON:
                    output_event(RtEvent::make_note_on_event(this->id(), event.sample_offset(),
                                                             event.keyboard_event()->note(),
                                                             event.keyboard_event()->velocity()));
                    break;
                case RtEventType::NOTE_OFF:
                    output_event(RtEvent::make_note_off_event(this->id(), event.sample_offset(),
                                                              event.keyboard_event()->note(),
                                                              event.keyboard_event()->velocity()));
                    break;
                case RtEventType::NOTE_AFTERTOUCH:
                    output_event(RtEvent::make_note_aftertouch_event(this->id(), event.sample_offset(),
                                                                     event.keyboard_event()->note(),
                                                                     event.keyboard_event()->velocity()));
                    break;
                default:
                    output_event(event);
            }
        }
    }
}

void Track::update_channel_config()
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

void Track::process_event(RtEvent event)
{
    switch (event.type())
    {
        /* Keyboard events are cached so they can be passed on
         * to the first processor in the track */
        case RtEventType::NOTE_ON:
        case RtEventType::NOTE_OFF:
        case RtEventType::NOTE_AFTERTOUCH:
        case RtEventType::WRAPPED_MIDI_EVENT:
            _event_buffer.push(event);
            break;

        default:
           break;
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
    switch (event.type())
    {
        /* Keyboard events are cached so they can be passed on
         * to the next processor in the track */
        case RtEventType::NOTE_ON:
        case RtEventType::NOTE_OFF:
        case RtEventType::NOTE_AFTERTOUCH:
        case RtEventType::WRAPPED_MIDI_EVENT:
            _event_buffer.push(event);
            break;

            /* Other events are passed on upstream unprocessed */
        default:
            output_event(event);
    }
}

} // namespace engine
} // namespace sushi
