/**
 * @Brief Helper classes for VST 3.x plugins.
 * @copyright MIND Music Labs AB, Stockholm
 *
 */

#ifndef SUSHI_VST3X_UTILS_H
#define SUSHI_VST3X_UTILS_H

#include <cassert>

#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"
#define DEVELOPMENT
#include "public.sdk/source/vst/hosting/eventlist.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#undef DEVELOPMENT

#include "library/sample_buffer.h"
#include "library/plugin_events.h"

namespace sushi {
namespace vst3 {

constexpr int VST_WRAPPER_MAX_N_CHANNELS = 2;

/**
 * @brief Wrapping the processdata in our own class for convenience
 */
class SushiProcessData : public Steinberg::Vst::ProcessData
{
public:
    SushiProcessData(Steinberg::Vst::EventList* in_event_list,
                     Steinberg::Vst::EventList* out_event_list,
                     Steinberg::Vst::ParameterChanges* in_parameter_changes,
                     Steinberg::Vst::ParameterChanges* out_parameter_changes) :
                                    _in_events(in_event_list),
                                    _out_events(out_event_list),
                                    _in_parameters(in_parameter_changes),
                                    _out_parameters(out_parameter_changes)
    {
        Steinberg::Vst::ProcessData();

        _input_buffers.channelBuffers32 = _process_inputs;
        _output_buffers.channelBuffers32 = _process_outputs;
        inputs = &_input_buffers;
        outputs = &_output_buffers;
        numInputs = 1;  /* Note: number of busses, not channels */
        numOutputs = 1; /* Note: number of busses, not channels */
        numSamples = AUDIO_CHUNK_SIZE;
        symbolicSampleSize = Steinberg::Vst::SymbolicSampleSizes::kSample32;
        processMode = Steinberg::Vst::ProcessModes::kRealtime;

        inputEvents = in_event_list;
        outputEvents = out_event_list;
        inputParameterChanges = in_parameter_changes;
        outputParameterChanges = out_parameter_changes;
    }
    /**
     * @brief Re-map the internal buffers to point to the given samplebuffers.
     *        Use before calling process(data)
     * @param input Input buffers
     * @param output Output buffers
     */
    void assign_buffers(const ChunkSampleBuffer& input, ChunkSampleBuffer& output);

    /**
     * @brief Clear all event and parameter changes to prepare for a new round
     *        of processing. Call when process(data) has returned
     */
    void clear()
    {
        _in_events->clear();
        _out_events->clear();
        _in_parameters->clearQueue();
        _out_parameters->clearQueue();
    }

private:
    float* _process_inputs[VST_WRAPPER_MAX_N_CHANNELS];
    float* _process_outputs[VST_WRAPPER_MAX_N_CHANNELS];
    Steinberg::Vst::AudioBusBuffers _input_buffers;
    Steinberg::Vst::AudioBusBuffers _output_buffers;
    /* Keep pointers to the implementations so we can call clear on them */
    Steinberg::Vst::EventList* _in_events;
    Steinberg::Vst::EventList* _out_events;
    Steinberg::Vst::ParameterChanges* _in_parameters;
    Steinberg::Vst::ParameterChanges* _out_parameters;
};

/**
 * @brief Convert a Sushi NoteOn event to a Vst3 Note on event
 * @param event A Sushi note on event
 * @return a Vst3 note on event
 */
Steinberg::Vst::Event convert_note_on_event(const KeyboardEvent* event);

/**
 * @brief Convert a Sushi NoteOff event to a Vst3 Note off event
 * @param event A Sushi note off event
 * @return a Vst3 note off event
 */
Steinberg::Vst::Event convert_note_off_event(const KeyboardEvent* event);

/**
 * @brief Convert a Sushi Aftertouch event to a Vst3 event
 * @param event A Sushi Aftertouch event
 * @return a Vst3 poly pressure event
 */
Steinberg::Vst::Event convert_aftertouch_event(const KeyboardEvent* event);


} // end namespace vst3
} // end namespace sushi


#endif //SUSHI_VST3X_UTILS_H
