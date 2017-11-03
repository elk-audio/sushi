#ifdef SUSHI_BUILD_WITH_VST3

#include <pluginterfaces/base/ustring.h>
#include "vst3x_wrapper.h"
#include "logging.h"

namespace sushi {
namespace vst3 {

constexpr int VST_NAME_BUFFER_SIZE = 128;

MIND_GET_LOGGER;

void Vst3xWrapper::_cleanup()
{
    if (_instance.component())
        set_enabled(false);
}

ProcessorReturnCode Vst3xWrapper::init(float sample_rate)
{
    _sample_rate = sample_rate;
    bool loaded;
    std::tie(loaded, _instance) = _loader.load_plugin();
    if (!loaded)
    {
        _cleanup();
        return ProcessorReturnCode::PLUGIN_LOAD_ERROR;
    }
    set_name(_instance.name());
    set_label(_instance.name());

    if (!_setup_audio_busses() || !_setup_event_busses())
    {
        return ProcessorReturnCode::PLUGIN_INIT_ERROR;
    }
    auto res = _instance.component()->setActive(Steinberg::TBool(true));
    if (res != Steinberg::kResultOk)
    {
        MIND_LOG_ERROR("Failed to activate component with error code: {}", res);
        return ProcessorReturnCode::PLUGIN_INIT_ERROR;
    }

    if (!_setup_channels())
    {
        return ProcessorReturnCode::PLUGIN_INIT_ERROR;
    }
    if (!_setup_processing())
    {
        return ProcessorReturnCode::PLUGIN_INIT_ERROR;
    }
    if (!_register_parameters())
    {
        return ProcessorReturnCode::PARAMETER_ERROR;
    }
    return ProcessorReturnCode::OK;
}

void Vst3xWrapper::configure(float sample_rate)
{
    _sample_rate = sample_rate;
    bool reset_enabled = enabled();
    if (reset_enabled)
    {
        set_enabled(false);
    }
    if (!_setup_processing())
    {
        // TODO how to handle this?
        MIND_LOG_ERROR("Error setting sample rate to {}", sample_rate);
    }
    if (reset_enabled)
    {
        set_enabled(true);
    }
    return;
}


void Vst3xWrapper::process_event(Event event)
{
    switch (event.type())
    {
        case EventType::FLOAT_PARAMETER_CHANGE:
        {
            int index;
            auto typed_event = event.parameter_change_event();
            auto param_queue = _in_parameter_changes.addParameterData(typed_event->param_id(), index);
            if (param_queue)
            {
                param_queue->addPoint(typed_event->sample_offset(), typed_event->value(), index);
            }
            break;
        }
        case EventType::NOTE_ON:
        {
            auto vst_event = convert_note_on_event(event.keyboard_event());
            _in_event_list.addEvent(vst_event);
            break;
        }
        case EventType::NOTE_OFF:
        {
            auto vst_event = convert_note_off_event(event.keyboard_event());
            _in_event_list.addEvent(vst_event);
            break;
        }
        case EventType::NOTE_AFTERTOUCH:
        {
            auto vst_event = convert_aftertouch_event(event.keyboard_event());
            _in_event_list.addEvent(vst_event);
            break;
        }
        case EventType::WRAPPED_MIDI_EVENT:
        {
            // TODO - Invoke midi decoder here, vst3 doesn't support raw midi
        }
        default:
            break;
    }
    return;
}

void Vst3xWrapper::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    if(_bypassed && !_can_do_soft_bypass)
    {
        bypass_process(in_buffer, out_buffer);
    }
    else
    {
        _process_data.assign_buffers(in_buffer, out_buffer, _current_input_channels, _current_output_channels);
        _instance.processor()->process(_process_data);
        _forward_events(_process_data);
    }
    _process_data.clear();
}

void Vst3xWrapper::set_input_channels(int channels)
{
    Processor::set_input_channels(channels);
    _setup_channels();
}

void Vst3xWrapper::set_output_channels(int channels)
{
    Processor::set_output_channels(channels);
    _setup_channels();
}


void Vst3xWrapper::set_enabled(bool enabled)
{
    auto res = _instance.processor()->setProcessing(Steinberg::TBool(enabled));
    if (res == Steinberg::kResultOk)
    {
        _enabled = enabled;
    }
}

void Vst3xWrapper::set_bypassed(bool bypassed)
{
    Processor::set_bypassed(bypassed);
    if(_can_do_soft_bypass)
    {
        Event e = Event::make_parameter_change_event(0, 0, _bypass_parameter_id, bypassed? 1.0f : 0.0f);
        this->process_event(e);
    }
}

bool Vst3xWrapper::_register_parameters()
{
    int param_count = _instance.controller()->getParameterCount();
    _in_parameter_changes.setMaxParameters(param_count);
    _out_parameter_changes.setMaxParameters(param_count);

    char name_c_str[128];
    for (int i = 0; i < param_count; ++i)
    {
        Steinberg::Vst::ParameterInfo info;
        auto res = _instance.controller()->getParameterInfo(i, info);
        if (res == Steinberg::kResultOk)
        {
            /* Vst3 uses a confusing model where parameters are indexed by an integer from 0
             * to getParameterCount() - 1 (just like Vst2.4). But in addition, each parameter
             * also has a 32 bit integer id which is arbitrarily assigned. For ADelay these are
             * 100 and 101.
             * When doing real time parameter updates, the parameters must be accessed using this
             * arbitrary id and not the index. Hence the id in the registered ParameterDescriptors
             * store this id and not the index in the processor array like it does for the Vst2
             * wrapper and internal plugins. Hopefully that doesn't cause any issues. */
            Steinberg::UString128 str(info.title, VST_NAME_BUFFER_SIZE);
            str.toAscii(name_c_str, VST_NAME_BUFFER_SIZE);
            if(info.flags & Steinberg::Vst::ParameterInfo::kIsBypass)
            {
                _can_do_soft_bypass = true;
                _bypass_parameter_id = info.id;
            }
            else if (register_parameter(new FloatParameterDescriptor(name_c_str, name_c_str, 0, 1, nullptr), info.id))
            {
                MIND_LOG_INFO("Registered parameter {}.", name_c_str);
            } else
            {
                MIND_LOG_INFO("Error registering parameter {}.", name_c_str);
            }
        }
    }
    return true;
}

bool Vst3xWrapper::_setup_audio_busses()
{
    int input_audio_busses = _instance.component()->getBusCount(Steinberg::Vst::MediaTypes::kAudio, Steinberg::Vst::BusDirections::kInput);
    int output_audio_busses = _instance.component()->getBusCount(Steinberg::Vst::MediaTypes::kAudio, Steinberg::Vst::BusDirections::kOutput);
    MIND_LOG_INFO("Plugin has {} audio input buffers and {} audio output buffers", input_audio_busses, output_audio_busses);
    if (output_audio_busses == 0)
    {
        return false;
    }
    _max_input_channels = 0;
    _max_output_channels = 0;
    /* Setup 1 main output bus and 1 main input bus (if available) */
    Steinberg::Vst::BusInfo info;
    for (int i = 0; i < input_audio_busses; ++i)
    {
        auto res = _instance.component()->getBusInfo(Steinberg::Vst::MediaTypes::kAudio,
                                                     Steinberg::Vst::BusDirections::kInput, i, info);
        if (res == Steinberg::kResultOk && info.busType == Steinberg::Vst::BusTypes::kMain) // Then use this one
        {
            _max_input_channels = info.channelCount;
            _current_input_channels = _max_input_channels;
            res = _instance.component()->activateBus(Steinberg::Vst::MediaTypes::kAudio,
                                                     Steinberg::Vst::BusDirections::kInput, i, Steinberg::TBool(true));
            if (res != Steinberg::kResultOk)
            {
                MIND_LOG_ERROR("Failed to activate plugin input bus {}", i);
                return false;
            }
            break;
        }
    }
    for (int i = 0; i < output_audio_busses; ++i)
    {
        auto res = _instance.component()->getBusInfo(Steinberg::Vst::MediaTypes::kAudio,
                                                     Steinberg::Vst::BusDirections::kOutput, i, info);
        if (res == Steinberg::kResultOk && info.busType == Steinberg::Vst::BusTypes::kMain) // Then use this one
        {
            _max_output_channels = info.channelCount;
            _current_output_channels = _max_output_channels;
            res = _instance.component()->activateBus(Steinberg::Vst::MediaTypes::kAudio,
                                                     Steinberg::Vst::BusDirections::kOutput, i, Steinberg::TBool(true));
            if (res != Steinberg::kResultOk)
            {
                MIND_LOG_ERROR("Failed to activate plugin output bus {}", i);
                return false;
            }
            break;
        }
    }
    MIND_LOG_INFO("Vst3 wrapper ({}) has {} inputs and {} outputs", this->name(), _max_input_channels, _max_output_channels);
    return true;
}

bool Vst3xWrapper::_setup_event_busses()
{
    int input_busses = _instance.component()->getBusCount(Steinberg::Vst::MediaTypes::kEvent, Steinberg::Vst::BusDirections::kInput);
    int output_busses = _instance.component()->getBusCount(Steinberg::Vst::MediaTypes::kEvent, Steinberg::Vst::BusDirections::kOutput);
    MIND_LOG_INFO("Plugin has {} event input buffers and {} event output buffers", input_busses, output_busses);
    /* Try to activate all busses here */
    for (int i = 0; i < input_busses; ++i)
    {
        auto res = _instance.component()->activateBus(Steinberg::Vst::MediaTypes::kEvent,
                                                     Steinberg::Vst::BusDirections::kInput, i, Steinberg::TBool(true));
        if (res != Steinberg::kResultOk)
        {
            MIND_LOG_ERROR("Failed to activate plugin input event bus {}", i);
            return false;
        }
    }
    for (int i = 0; i < output_busses; ++i)
    {
        auto res = _instance.component()->activateBus(Steinberg::Vst::MediaTypes::kEvent,
                                                      Steinberg::Vst::BusDirections::kInput, i, Steinberg::TBool(true));
        if (res != Steinberg::kResultOk)
        {
            MIND_LOG_ERROR("Failed to activate plugin output event bus {}", i);
            return false;
        }
    }
    return true;
}

bool Vst3xWrapper::_setup_channels()
{
    MIND_LOG_INFO("Vst3 wrapper ({}) setting up {} inputs and {} outputs", this->name(), _current_input_channels, _current_output_channels);
    Steinberg::Vst::SpeakerArrangement input_arr = speaker_arr_from_channels(_current_input_channels);
    Steinberg::Vst::SpeakerArrangement output_arr = speaker_arr_from_channels(_current_output_channels);

    /* numIns and numOuts refer to the number of busses, not channels, the docs are very vague on this point */
    auto res = _instance.processor()->setBusArrangements(&input_arr, (_max_input_channels == 0)? 0:1, &output_arr, 1);
    if (res != Steinberg::kResultOk)
    {
        MIND_LOG_ERROR("Failed to set a valid channel arrangement");
        return false;
    }
    return true;
}

bool Vst3xWrapper::_setup_processing()
{
    Steinberg::Vst::ProcessSetup setup;
    setup.maxSamplesPerBlock = AUDIO_CHUNK_SIZE;
    setup.processMode = Steinberg::Vst::ProcessModes::kRealtime;
    setup.sampleRate = _sample_rate;
    setup.symbolicSampleSize = Steinberg::Vst::SymbolicSampleSizes::kSample32;
    auto res = _instance.processor()->setupProcessing(setup);
    if (res != Steinberg::kResultOk)
    {
        MIND_LOG_ERROR("Error setting up processing, error code: {}", res);
        return false;
    }
    return true;
}

void Vst3xWrapper::_forward_events(Steinberg::Vst::ProcessData& data)
{
    int event_count = data.outputEvents->getEventCount();
    for (int i = 0; i < event_count; ++i)
    {
        Steinberg::Vst::Event vst_event;
        if (data.outputEvents->getEvent(i, vst_event) == Steinberg::kResultOk)
        {
            switch (vst_event.type)
            {
                case Steinberg::Vst::Event::EventTypes::kNoteOnEvent:
                    output_event(Event::make_note_on_event(0, vst_event.sampleOffset,
                                                           vst_event.noteOn.pitch,
                                                           vst_event.noteOn.velocity));
                    break;

                case Steinberg::Vst::Event::EventTypes::kNoteOffEvent:
                    output_event(Event::make_note_off_event(0, vst_event.sampleOffset,
                                                            vst_event.noteOff.pitch,
                                                            vst_event.noteOff.velocity));
                    break;

                case Steinberg::Vst::Event::EventTypes::kPolyPressureEvent:
                    output_event(Event::make_note_aftertouch_event(0, vst_event.sampleOffset,
                                                            vst_event.polyPressure.pitch,
                                                            vst_event.polyPressure.pressure));
                    break;

                default:
                    break;
            }
        }
    }

}

Steinberg::Vst::SpeakerArrangement speaker_arr_from_channels(int channels)
{
    switch (channels)
    {
        case 0:
            return Steinberg::Vst::SpeakerArr::kEmpty;
        case 1:
            return Steinberg::Vst::SpeakerArr::kMono;
        case 2:
            return Steinberg::Vst::SpeakerArr::kStereo;
        case 3:
            return Steinberg::Vst::SpeakerArr::k30Music;
        case 4:
            return Steinberg::Vst::SpeakerArr::k40Music;
        case 5:
            return Steinberg::Vst::SpeakerArr::k50;
        case 6:
            return Steinberg::Vst::SpeakerArr::k60Music;
        case 7:
            return Steinberg::Vst::SpeakerArr::k70Music;
        default:
            return Steinberg::Vst::SpeakerArr::k80Music;
    }
}
} // end namespace vst3
} // end namespace sushi

#endif //SUSHI_BUILD_WITH_VST3
#ifndef SUSHI_BUILD_WITH_VST3
#include "library/vst3x_wrapper.h"
#include "logging.h"
namespace sushi {
namespace vst3 {
MIND_GET_LOGGER;
ProcessorReturnCode Vst3xWrapper::init(float /*sample_rate*/)
{
    /* The log print needs to be in a cpp file for initialisation order reasons */
    MIND_LOG_ERROR("Sushi was not built with Vst 3 support!");
    return ProcessorReturnCode::ERROR;
}}}
#endif
