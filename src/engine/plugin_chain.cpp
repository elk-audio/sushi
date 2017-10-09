
#include "plugin_chain.h"
#include <iostream>

namespace sushi {
namespace engine {


void PluginChain::add(Processor* processor)
{
    // If a chain adds itself to its process chain, endless loops can arrise
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
     * processing chain was empty */
    out = in_bfr;
    /* If there are keyboard events not consumed, pass them on upwards */
    while (!_event_buffer.empty())
    {
        RtEvent event;
        if (_event_buffer.pop(event))
        {
            output_event(event);
        }
    }
}

void PluginChain::update_channel_config()
{
    int input_channels = _current_input_channels;
    int output_channels;

    for (unsigned int i = 0; i < _chain.size(); ++i)
    {
        input_channels = std::min(input_channels, _chain[i]->max_input_channels());
        if (input_channels != _chain[i]->input_channels())
        {
            bool res = _chain[i]->set_input_channels(input_channels);
            assert(res);
        }
        if (i < _chain.size() - 1)
        {
            output_channels = std::min(_max_output_channels, std::min(_chain[i]->max_output_channels(),
                                                                      _chain[i+1]->max_input_channels()));
        }
        else
        {
            output_channels = std::min(_max_output_channels, std::min(_chain[i]->max_output_channels(),
                                                                      _current_output_channels));
        }
        if (output_channels != _chain[i]->output_channels())
        {
            bool res = _chain[i]->set_output_channels(output_channels);
            assert(res);
        }
        input_channels = output_channels;
    }

    if (!_chain.empty())
    {
        auto last = _chain.back();
        int chain_outputs = std::min(_current_output_channels, last->output_channels());
        if (chain_outputs != last->output_channels())
        {
            bool res = last->set_output_channels(chain_outputs);
            assert(res);
        }
    }
}

void PluginChain::process_event(RtEvent event)
{
    switch (event.type())
    {
        /* Keyboard events are cached so they can be passed on
         * to the first processor in the chain */
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

void PluginChain::set_bypassed(bool bypassed)
{
    for (auto& processor : _chain)
    {
        processor->set_bypassed(bypassed);
    }
    Processor::set_bypassed(bypassed);
}

void PluginChain::send_event(RtEvent event)
{
    switch (event.type())
    {
        /* Keyboard events are cached so they can be passed on
         * to the next processor in the chain */
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
