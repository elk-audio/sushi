
#include "plugin_chain.h"
#include <iostream>

namespace sushi {
namespace engine {


void PluginChain::add(Processor* processor)
{
    // If a chain adds itself to it's process chain, endless loops can arrise
    assert(processor != this);
    _chain.push_back(processor);
    processor->set_event_output(this);
    update_channel_config();
}

bool PluginChain::remove(ObjectId processor)
{
    for (auto plugin = _chain.begin(); plugin != _chain.end(); ++plugin)
    {
        if ((*plugin)->id() == processor)
        {
            (*plugin)->set_event_output(nullptr);
            _chain.erase(plugin);
            update_channel_config();
            return true;
        }
    }
    return false;
}

void PluginChain::process_audio(const ChunkSampleBuffer& in, ChunkSampleBuffer& out)
{
    /* Alias the internal buffers to get the right channel count */
    ChunkSampleBuffer in_bfr = ChunkSampleBuffer::create_non_owning_buffer(_bfr_1, 0, _current_input_channels);
    ChunkSampleBuffer out_bfr = ChunkSampleBuffer::create_non_owning_buffer(_bfr_2, 0, _current_input_channels);
    in_bfr.clear();
    in_bfr.add(in);

    for (auto &plugin : _chain)
    {
        while (!_event_buffer.empty()) // This should only contain keyboard/note events
        {
            Event event;
            if (_event_buffer.pop(event))
            {
                plugin->process_event(event);
            }
        }
        plugin->process_audio(in_bfr, out_bfr);
        std::swap(in_bfr, out_bfr);
    }

    /* Yes, it is in_bfr buffer here. Either it was swapped with out_bfr or the
     * processing chain was empty */
    out.add(in_bfr);
    /* If there are keyboard events not consumed, pass them on upwards */
    while (!_event_buffer.empty())
    {
        Event event;
        if (_event_buffer.pop(event))
        {
            output_event(event);
        }
    }
}

void PluginChain::update_channel_config()
{
    /* No real channel config negotiation here, simply  assume that the channel
     * config for the chain also applies to all processors that are part of it.
     * ie. if the chain is stereo in/stereo out, all processors should also be
     * configured as stereo in/stereo out. */
    if (_chain.empty())
    {
        return;
    }
    for (auto& processor : _chain)
    {
        processor->set_input_channels(_current_input_channels);
        processor->set_output_channels(_current_input_channels);
    }
}

void PluginChain::process_event(Event event)
{
    switch (event.type())
    {
        /* Keyboard events are cached so they can be passed on
         * to the first processor in the chain */
        case EventType::NOTE_ON:
        case EventType::NOTE_OFF:
        case EventType::NOTE_AFTERTOUCH:
        case EventType::WRAPPED_MIDI_EVENT:
            _event_buffer.push(event);
            break;

            /* Handle events sent to this processor here */
        default:
           break;
    }
}

void PluginChain::send_event(Event event)
{
    switch (event.type())
    {
        /* Keyboard events are cached so they can be passed on
         * to the next processor in the chain */
        case EventType::NOTE_ON:
        case EventType::NOTE_OFF:
        case EventType::NOTE_AFTERTOUCH:
        case EventType::WRAPPED_MIDI_EVENT:
            _event_buffer.push(event);
            break;

            /* Other events are passed on upstream unprocessed */
        default:
            output_event(event);
    }
}

} // namespace engine
} // namespace sushi
