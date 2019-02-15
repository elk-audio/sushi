/**
 * @brief Midi i/o plugin example with a simple arpeggiator
 * @copyright MIND Music Labs AB, Stockholm
 */

#ifndef SUSHI_TRANSPOSER_PLUGIN_H
#define SUSHI_TRANSPOSER_PLUGIN_H

#include "library/rt_event_fifo.h"
#include "library/internal_plugin.h"


namespace sushi {
namespace transposer_plugin {

class TransposerPlugin : public InternalPlugin
{
public:
    TransposerPlugin(HostControl host_control);

    ~TransposerPlugin() {}

    ProcessorReturnCode init(float sample_rate) override;

    //void configure(float sample_rate) override;

    void process_event(RtEvent event) override;

    void process_audio(const ChunkSampleBuffer&/*in_buffer*/, ChunkSampleBuffer& /*out_buffer*/) override;

private:

    int _transpose_note(int note);

    MidiDataByte _transpose_midi(MidiDataByte midi_msg);

    FloatParameterValue*   _transpose_parameter;

    RtEventFifo _queue;
};

float samples_per_note(float note_fraction, float tempo, float samplerate);

}// namespace transposer_plugin
}// namespace sushi


#endif //SUSHI_TRANSPOSER_PLUGIN_H
