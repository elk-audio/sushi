#include <pluginterfaces/base/ustring.h>
#include "vst3x_wrapper.h"
#include "logging.h"

namespace sushi {
namespace vst3 {

constexpr int VST_NAME_BUFFER_SIZE = 128;

MIND_GET_LOGGER;

void Vst3xWrapper::_cleanup()
{

}

ProcessorReturnCode Vst3xWrapper::init(const int sample_rate)
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

    if (!_setup_busses())
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

void Vst3xWrapper::process_event(Event event)
{
    switch (event.type())
    {
        case EventType::FLOAT_PARAMETER_CHANGE:
        {
            MIND_LOG_INFO("Plugin has {} parameters", _in_parameter_changes.getParameterCount());
            auto typed_event = event.parameter_change_event();
            auto param_queue = _in_parameter_changes.getParameterData(typed_event->param_id());
            if (!param_queue)
            {
                MIND_LOG_INFO("No parameter queue for parameter {}", typed_event->param_id());
                break;
            }
            int return_index;
            param_queue->addPoint(typed_event->sample_offset(), typed_event->value(), return_index);
            break;
            // TODO - find a good use for return_index here
        }
        case EventType::NOTE_ON:
        case EventType::NOTE_OFF:
        case EventType::NOTE_AFTERTOUCH:
        case EventType::WRAPPED_MIDI_EVENT:
        {
            //_in_event_list.addEvent()
        }

        default:
            break;
    }

}

void Vst3xWrapper::process_audio(const ChunkSampleBuffer &in_buffer, ChunkSampleBuffer &out_buffer)
{
    _process_data.assign_buffers(in_buffer, out_buffer);
    _instance.processor()->process(_process_data);
    _process_data.clear();
}


void Vst3xWrapper::set_enabled(bool enabled)
{
    auto res = _instance.processor()->setProcessing(Steinberg::TBool(enabled));
    if (res == Steinberg::kResultOk)
    {
        _enabled = enabled;
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
            Steinberg::UString128 str(info.title, VST_NAME_BUFFER_SIZE);
            str.toAscii(name_c_str, VST_NAME_BUFFER_SIZE);
            if (register_parameter(new FloatParameterDescriptor(name_c_str, name_c_str, 0, 1, nullptr)))
            {
                MIND_LOG_INFO("Registered parameter {}.", name_c_str);
            } else
            {
                MIND_LOG_INFO("Error registering parameter {}.", name_c_str);
            }
            int return_index;
            _in_parameter_changes.addParameterData(Steinberg::Vst::ParamID(i), return_index);
            //MIND_LOG_INFO("Registered parameter index: {}", return_index);
        }
    }
    //MIND_LOG_INFO("Registered {} parameters", _in_parameter_changes.getParameterCount());
    return true;
}

bool Vst3xWrapper::_setup_busses()
{
    int input_audio_busses = _instance.component()->getBusCount(Steinberg::Vst::MediaTypes::kAudio, Steinberg::Vst::BusDirections::kInput);
    int output_audio_busses = _instance.component()->getBusCount(Steinberg::Vst::MediaTypes::kAudio, Steinberg::Vst::BusDirections::kOutput);
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
            res = _instance.component()->activateBus(Steinberg::Vst::MediaTypes::kAudio,
                                                     Steinberg::Vst::BusDirections::kInput, i, Steinberg::TBool(true));
            if (res != Steinberg::kResultOk)
            {
                MIND_LOG_ERROR("Failed to activate plugin input bus {}", i);
                return false;
            }
        }
    }
    for (int i = 0; i < output_audio_busses; ++i)
    {
        auto res = _instance.component()->getBusInfo(Steinberg::Vst::MediaTypes::kAudio,
                                                     Steinberg::Vst::BusDirections::kOutput, i, info);
        if (res == Steinberg::kResultOk && info.busType == Steinberg::Vst::BusTypes::kMain) // Then use this one
        {
            _max_output_channels = info.channelCount;
            res = _instance.component()->activateBus(Steinberg::Vst::MediaTypes::kAudio,
                                                     Steinberg::Vst::BusDirections::kOutput, i, Steinberg::TBool(true));
            if (res != Steinberg::kResultOk)
            {
                MIND_LOG_ERROR("Failed to activate plugin output bus {}", i);
                return false;
            }
        }
    }
    return true;
}

bool Vst3xWrapper::_setup_channels()
{
    /* Try set up a stereo output pair and, if there is an input bus, a stereo input pair */
    if (_max_input_channels == 0 || _max_output_channels < 2)
    {
        MIND_LOG_ERROR("Not enough channels supported {}:{}", _max_input_channels, _max_input_channels);
        return false;
    }
    Steinberg::Vst::SpeakerArrangement input_arr = (_max_input_channels == 0)? Steinberg::Vst::SpeakerArr::kEmpty :
                                                                              Steinberg::Vst::SpeakerArr::kStereo;
    Steinberg::Vst::SpeakerArrangement output_arr = Steinberg::Vst::SpeakerArr::kStereo;

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

} // end namespace vst3
} // end namespace sushi