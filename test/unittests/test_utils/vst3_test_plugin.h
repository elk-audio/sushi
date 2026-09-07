#ifndef SUSHI_LIBRARY_VST_3_TEST_PLUGIN_H
#define SUSHI_LIBRARY_VST_3_TEST_PLUGIN_H

#ifdef SUSHI_BUILD_WITH_VST3

#include <array>

#include "pluginterfaces/base/funknownimpl.h"
#include "public.sdk/source/vst/vstsinglecomponenteffect.h"
#include "elk_vst3_extensions/elk_vst3_extensions.h"

namespace test_utils {

constexpr auto CALLBACK_ID = 101;

class Vst3TestPlugin : public Steinberg::Vst::SingleComponentEffect, ::elk::IElkControllerExtension, ::elk::IElkProcessorExtension
{
public:
    Vst3TestPlugin();
    ~Vst3TestPlugin();

    static constexpr size_t STRING_PROPERTIES = 2;
    static constexpr int PROPERTY_ID_1 = 5;
    static constexpr int PROPERTY_ID_2 = 7;

    // From IAudioProcessor
    Steinberg::tresult PLUGIN_API initialize (Steinberg::FUnknown* context) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API setBusArrangements (Steinberg::Vst::SpeakerArrangement* inputs, Steinberg::int32 numIns, Steinberg::Vst::SpeakerArrangement* outputs, Steinberg::int32 numOuts) SMTG_OVERRIDE;

    Steinberg::tresult PLUGIN_API setActive (Steinberg::TBool state) SMTG_OVERRIDE;
    Steinberg::tresult PLUGIN_API process (Steinberg::Vst::ProcessData& data) SMTG_OVERRIDE;

    // From Elk extensions
    Steinberg::int32 PLUGIN_API getPropertyCount() override;

    Steinberg::tresult PLUGIN_API getPropertyInfo(Steinberg::int32 propertyIndex, elk::PropertyInfo& info) override;

    Steinberg::tresult PLUGIN_API getPropertyValue(Steinberg::int32 propertyId, elk::PropertyValue& value) override;

    Steinberg::tresult PLUGIN_API setPropertyValue(Steinberg::int32 propertyId, const elk::PropertyValue& value) override;

    void PLUGIN_API propertyValueChanged(Steinberg::int32 propertyId, const elk::PropertyValue& value) override;

    void PLUGIN_API asyncWorkCompleted(Steinberg::int32 requestId, Steinberg::int32 requestReturnValue) override;

    Steinberg::tresult PLUGIN_API queryInterface (const Steinberg::TUID iid, void** obj) override;

    Steinberg::tresult PLUGIN_API setComponentHandler (Steinberg::Vst::IComponentHandler* handler) override;

    // This refers the refcount methods to the given implementation
    REFCOUNT_METHODS (Steinberg::Vst::EditControllerEx1)

    bool send_async_work_request();

    // Everything is public to make it easy for tests to verify that everything works as intended
    Steinberg::OPtr<Steinberg::Vst::IHostApplication> host_app{nullptr};
    Steinberg::OPtr<elk::IElkHostExtension> host_extension{nullptr};
    Steinberg::Vst::IComponentHandler* component_handler{nullptr};
    Steinberg::OPtr<elk::IElkComponentHandlerExtension> component_handler_extension{nullptr};


    std::array<elk::PropertyInfo, STRING_PROPERTIES> _string_properties;
    std::array<std::string, STRING_PROPERTIES> _property_values;
    int _last_changed_property_id;
    std::string _last_changed_property_value;
    bool _last_changed_property_was_rt_thread{false};

    int _last_async_id;
    int _last_async_id_received;
    int _last_async_status;
};

}// namespace test_utils

#else //SUSHI_BUILD_WITH_VST3

class Vst3TestPlugin
{
}

#endif  //SUSHI_BUILD_WITH_VST3
#endif //SUSHI_LIBRARY_VST_3_TEST_PLUGIN_H
