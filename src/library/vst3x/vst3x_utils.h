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

#ifndef SUSHI_VST3X_UTILS_H
#define SUSHI_VST3X_UTILS_H

#include <cassert>

#include "pluginterfaces/base/ipluginbase.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/vst/ivstevents.h"
#include "public.sdk/source/vst/hosting/eventlist.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"

#include "library/sample_buffer.h"
#include "library/rt_event.h"
#include "library/constants.h"
#include "library/processor_state.h"

namespace sushi {
namespace vst3 {

constexpr int VST_WRAPPER_MAX_N_CHANNELS = MAX_TRACK_CHANNELS;

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
        _input_buffers.channelBuffers32 = _process_inputs;
        _output_buffers.channelBuffers32 = _process_outputs;
        inputs = &_input_buffers;
        outputs = &_output_buffers;
        numInputs = 1;  /* Note: number of buses, not channels */
        numOutputs = 1; /* Note: number of buses, not channels */
        numSamples = AUDIO_CHUNK_SIZE;
        symbolicSampleSize = Steinberg::Vst::SymbolicSampleSizes::kSample32;
        processMode = Steinberg::Vst::ProcessModes::kRealtime;
        processContext = &_context;

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
    void assign_buffers(const ChunkSampleBuffer& input, ChunkSampleBuffer& output, int in_channels, int out_channels);

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
    Steinberg::Vst::ProcessContext  _context;
    /* Keep pointers to the implementations, so we can call clear on them. */
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
Steinberg::Vst::Event convert_note_on_event(const KeyboardRtEvent* event);

/**
 * @brief Convert a Sushi NoteOff event to a Vst3 Note off event
 * @param event A Sushi note off event
 * @return a Vst3 note off event
 */
Steinberg::Vst::Event convert_note_off_event(const KeyboardRtEvent* event);

/**
 * @brief Convert a Sushi Aftertouch event to a Vst3 event
 * @param event A Sushi Aftertouch event
 * @return a Vst3 poly pressure event
 */
Steinberg::Vst::Event convert_aftertouch_event(const KeyboardRtEvent* event);

/**
 * @brief Custom implementation of IParamValueQueue to work with Vst3xRtState below
 */
class StateParamValue : public Steinberg::Vst::IParamValueQueue
{
public:
    StateParamValue(ObjectId id, const float value) : _id(id), _value(value) {}

    void set_values(const ObjectId& id, const float& value);

    // Inherited from IParamValueQueue
    Steinberg::Vst::ParamID getParameterId() override {return _id;};

    Steinberg::int32 getPointCount() override {return 1;}

    Steinberg::tresult getPoint(Steinberg::int32 /*index*/, Steinberg::int32& sampleOffset /*out*/,
                                Steinberg::Vst::ParamValue& value /*out*/) override;

    // The functions below should not be called, as this container should not be added too
    Steinberg::tresult addPoint(Steinberg::int32 /*sampleOffset*/, Steinberg::Vst::ParamValue /*value*/,
                                Steinberg::int32& /*index*/) override {return Steinberg::kResultFalse;};

    // These are added for completeness, but this class should not be used with Steinbergs reference counters pointers
    Steinberg::tresult queryInterface(const Steinberg::TUID /*_iid*/, void** /*obj*/) override
    {
        return Steinberg::kNotImplemented;
    }

    Steinberg::uint32 addRef() override {return 1;}

    Steinberg::uint32 release() override {return 1;}

private:
    ObjectId _id;
    float _value;
};

/**
 * @brief The implementation of Steinberg::Vst::ParameterChanges supplied with the vst sdk is
 * much too inefficient for setting a large number of parameters during one audio process call.
 * Instead we wrap RtState, which has parameter changes stored sequentially in a contiguous
 * block of memory in an interface that plugins can access directly */
class Vst3xRtState : public RtState, public Steinberg::Vst::IParameterChanges
{
public:
    explicit Vst3xRtState(const ProcessorState& state) : RtState(state) {}

    Steinberg::int32 getParameterCount () override;

    Steinberg::Vst::IParamValueQueue* getParameterData (Steinberg::int32 index) override;

    Steinberg::Vst::IParamValueQueue* addParameterData (const Steinberg::Vst::ParamID& /*id*/, Steinberg::int32& /*index*/) override;

    // These are added for completeness, but this class should not be used with Steinbergs reference counters pointers
    Steinberg::tresult queryInterface (const Steinberg::TUID /*_iid*/, void** /*obj*/) override
    {
        return Steinberg::kNotImplemented;
    };

    Steinberg::uint32 addRef() override {return 1;};

    Steinberg::uint32 release() override {return 1;};

private:
    StateParamValue _transfer_value{0, 0.0f};
};

} // end namespace vst3
} // end namespace sushi


#endif //SUSHI_VST3X_UTILS_H
