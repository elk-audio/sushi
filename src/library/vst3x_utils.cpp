#import "library/vst3x_utils.h"


namespace sushi {
namespace vst3{

void SushiProcessData::assign_buffers(const ChunkSampleBuffer& input, ChunkSampleBuffer& output)
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
    inputs->numChannels = input.channel_count();
    outputs->numChannels = output.channel_count();
}

Steinberg::Vst::Event convert_note_on_event(const KeyboardEvent* event)
{
    Steinberg::Vst::Event vst_event;
    vst_event.sampleOffset = event->sample_offset();
    vst_event.flags = 0;
    vst_event.type = Steinberg::Vst::Event::kNoteOnEvent;
    vst_event.noteOn.pitch = static_cast<Steinberg::int16>(event->note());
    vst_event.noteOn.velocity = event->velocity();
    vst_event.noteOn.tuning = 0.0f;
    vst_event.noteOn.noteId = -1;
    vst_event.noteOn.channel = 0;
    return vst_event;
}

Steinberg::Vst::Event convert_note_off_event(const KeyboardEvent* event)
{
    Steinberg::Vst::Event vst_event;
    vst_event.sampleOffset = event->sample_offset();
    vst_event.flags = 0;
    vst_event.type = Steinberg::Vst::Event::kNoteOffEvent;
    vst_event.noteOff.pitch = static_cast<Steinberg::int16>(event->note());
    vst_event.noteOff.velocity = event->velocity();
    vst_event.noteOff.tuning = 0.0f;
    vst_event.noteOff.noteId = -1;
    vst_event.noteOff.channel = 0;
    return vst_event;
}

Steinberg::Vst::Event convert_aftertouch_event(const KeyboardEvent* event)
{
    Steinberg::Vst::Event vst_event;
    vst_event.sampleOffset = event->sample_offset();
    vst_event.flags = 0;
    vst_event.type = Steinberg::Vst::Event::kPolyPressureEvent;
    vst_event.polyPressure.pitch = static_cast<Steinberg::int16>(event->note());
    vst_event.polyPressure.pressure = event->velocity();
    vst_event.polyPressure.noteId = -1;
    vst_event.polyPressure.channel = 0;
    return vst_event;
}

} // end namespace vst3
} // end namespace sushi