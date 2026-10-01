#ifdef SUSHI_BUILD_WITH_VST3

#include "gtest/gtest.h"

#include <twine/twine.h>

#include "vst3_test_plugin.h"
#include "library/vst3x/vst3x_utils.h"
#include "public.sdk/source/vst/vstsinglecomponenteffect.cpp"
#include "public.sdk/source/vst/utility/stringconvert.h"

using namespace Steinberg;
using namespace Steinberg::Vst;
using namespace sushi::internal::vst3;
namespace test_utils {

Vst3TestPlugin::Vst3TestPlugin()
{
    elk::PropertyInfo info;
    info.id = PROPERTY_ID_1;
    VST3::StringConvert::convert("property_1", info.name);
    VST3::StringConvert::convert("Property 1", info.label);
    info.flags = elk::PropertyInfo::kAudioThreadNotify;
    _string_properties[0] = info;

    info.id = PROPERTY_ID_2;
    VST3::StringConvert::convert("property_2", info.name);
    VST3::StringConvert::convert("Property 2", info.label);
    info.flags = elk::PropertyInfo::kIsReadOnly;
    _string_properties[1] = info;
}

Steinberg::int32 test_callback(void* data, Steinberg::uint16 id)
{
    return CALLBACK_ID + id;
}

void assert_true(bool value)
{
    ASSERT_TRUE(value);
}

Vst3TestPlugin::~Vst3TestPlugin() = default;

Steinberg::tresult Vst3TestPlugin::initialize(Steinberg::FUnknown* context)
{
    auto result = SingleComponentEffect::initialize(context);
    if (result == kResultTrue)
    {
        parameters.addParameter (STR16 ("Bypass"), nullptr, 1, 0, ParameterInfo::kCanAutomate | ParameterInfo::kIsBypass, 1);
        parameters.addParameter (STR16 ("Delay"), STR16 ("sec"), 0, 1, ParameterInfo::kCanAutomate, 2);
        parameters.addParameter (STR16 ("Output"), STR16 (""), 0, 1, ParameterInfo::kIsReadOnly, 3);

        addAudioInput (STR16 ("AudioInput"), SpeakerArr::kStereo);
        addAudioOutput (STR16 ("AudioOutput"), SpeakerArr::kStereo);
    }

    host_app = query_vst_interface<IHostApplication>(context);
    host_extension = query_vst_interface<elk::IElkHostExtension>(context);

    return kResultTrue;
}

Steinberg::tresult Vst3TestPlugin::setBusArrangements(Steinberg::Vst::SpeakerArrangement* inputs, Steinberg::int32 numIns, Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 numOuts)
{
    return SingleComponentEffect::setBusArrangements(inputs, numIns, outputs, numOuts);
}

Steinberg::tresult Vst3TestPlugin::setActive(Steinberg::TBool state)
{
    return SingleComponentEffect::setActive(state);
}

Steinberg::tresult Vst3TestPlugin::process(Steinberg::Vst::ProcessData& data)
{
    // Test that Sushi's time capabilities are reported correctly
    assert_true(data.processContext->state & Steinberg::Vst::ProcessContext::kSystemTimeValid);
    assert_true(data.processContext->state & Steinberg::Vst::ProcessContext::kContTimeValid);
    assert_true(data.processContext->state & Steinberg::Vst::ProcessContext::kBarPositionValid);
    assert_true(data.processContext->state & Steinberg::Vst::ProcessContext::kTempoValid);
    assert_true(data.processContext->state & Steinberg::Vst::ProcessContext::kTimeSigValid);

    return SingleComponentEffect::process(data);
}

Steinberg::int32 Vst3TestPlugin::getPropertyCount()
{
    return _string_properties.size();
}
Steinberg::tresult Vst3TestPlugin::getPropertyInfo(Steinberg::int32 propertyIndex, elk::PropertyInfo& info)
{
    if (propertyIndex < _string_properties.size())
    {
        info = _string_properties[propertyIndex];
        return kResultOk;
    }
    return kInvalidArgument;
}

Steinberg::tresult Vst3TestPlugin::getPropertyValue(Steinberg::int32 propertyId, elk::PropertyValue& value)
{
    for (int i = 0; i < _string_properties.size(); ++i)
    {
        if (propertyId == _string_properties[i].id)
        {
            value.value = _property_values[i].c_str();
            value.length = _property_values[i].size();
            return kResultOk;
        }
    }
    return kInvalidArgument;
}

Steinberg::tresult Vst3TestPlugin::setPropertyValue(Steinberg::int32 propertyId, const elk::PropertyValue& value)
{
    for (int i = 0; i < _string_properties.size(); ++i)
    {
        if (propertyId == _string_properties[i].id)
        {
            _property_values[i].clear();
            _property_values[i].append(value.value, value.length);
            return kResultOk;
        }
    }
    return kInvalidArgument;
}

void Vst3TestPlugin::propertyValueChanged(Steinberg::int32 propertyId, const elk::PropertyValue& value)
{
    _last_changed_property_id = propertyId;
    _last_changed_property_value.assign(value.value, value.length);
    _last_changed_property_was_rt_thread = twine::is_current_thread_realtime();
}

void Vst3TestPlugin::asyncWorkCompleted(Steinberg::int32 requestId, Steinberg::int32 requestReturnValue)
{
    _last_async_id_received = requestId;
    _last_async_status = requestReturnValue;
}

Steinberg::tresult Vst3TestPlugin::queryInterface(const char* iid, void** obj)
{
    DEF_INTERFACE (elk::IElkControllerExtension);
    DEF_INTERFACE (elk::IElkProcessorExtension);
    return SingleComponentEffect::queryInterface (iid, obj);
}

Steinberg::tresult Vst3TestPlugin::setComponentHandler(Steinberg::Vst::IComponentHandler* handler)
{
    if (EditController::setComponentHandler(handler) == kResultOk)
    {
        component_handler = componentHandler.get();
        component_handler_extension = query_vst_interface<elk::IElkComponentHandlerExtension>(handler);
    }
    return kResultOk;
}

bool Vst3TestPlugin::send_async_work_request()
{
    Steinberg::int32 id;
    auto res = host_extension->requestAsyncWork(test_callback, nullptr, id);
    _last_async_id = id;
    if (res == Steinberg::kResultOk)
    {
        return true;
    }
    return false;
}

}// namespace test_utils


#endif //SUSHI_BUILD_WITH_VST3