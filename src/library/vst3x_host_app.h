#ifndef SUSHI_VST3X_HOST_CONTEXT_H
#define SUSHI_VST3X_HOST_CONTEXT_H

#include "library/id_generator.h"
#include "library/constants.h"

#include <pluginterfaces/vst/ivsteditcontroller.h>
#include <third-party/vstsdk3/pluginterfaces/vst/ivstunits.h>
#include "pluginterfaces/vst/ivsthostapplication.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#define RELEASE = 1
#include "public.sdk/source/vst/hosting/module.h"
#pragma GCC diagnostic ignored "-Wextra"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#pragma GCC diagnostic pop
#undef RELEASE
namespace sushi {
namespace vst3 {

class Vst3xWrapper;
class ConnectionProxy;

class SushiHostApplication : public Steinberg::Vst::HostApplication
{
public:
    SushiHostApplication() : Steinberg::Vst::HostApplication() {}
    Steinberg::tresult getName (Steinberg::Vst::String128 name) override;
};

class ComponentHandler : public Steinberg::Vst::IComponentHandler
{
public:
    MIND_DECLARE_NON_COPYABLE(ComponentHandler);

    explicit ComponentHandler(Vst3xWrapper* wrapper_instance);
    Steinberg::tresult PLUGIN_API beginEdit (Steinberg::Vst::ParamID /*id*/) override {return Steinberg::kNotImplemented;}
    Steinberg::tresult PLUGIN_API performEdit (Steinberg::Vst::ParamID parameter_id, Steinberg::Vst::ParamValue normalized_value);
    Steinberg::tresult PLUGIN_API endEdit (Steinberg::Vst::ParamID /*parameter_id*/) override {return Steinberg::kNotImplemented;}
    Steinberg::tresult PLUGIN_API restartComponent (Steinberg::int32 /*flags*/) override {return Steinberg::kNotImplemented;}

    Steinberg::tresult PLUGIN_API queryInterface (const Steinberg::TUID /*_iid*/, void** /*obj*/) override {return Steinberg::kNoInterface;}
    Steinberg::uint32 PLUGIN_API addRef () override { return 1000; }
    Steinberg::uint32 PLUGIN_API release () override { return 1000; }
private:
    Vst3xWrapper* _wrapper_instance;
};


/**
 * @brief Container to hold plugin modules and manage their lifetimes
 */
class PluginInstance
{
public:
    MIND_DECLARE_NON_COPYABLE(PluginInstance);

    PluginInstance();
    ~PluginInstance();

    bool load_plugin(const std::string& plugin_path, const std::string& plugin_name);
    const std::string& name() const {return _name;}
    const std::string& vendor() const {return _vendor;}
    Steinberg::Vst::IComponent* component() {return _component.get();}
    Steinberg::Vst::IAudioProcessor* processor() {return _processor.get();}
    Steinberg::Vst::IEditController* controller() {return _controller.get();}
    Steinberg::Vst::IUnitInfo* unit_info() {return _unit_info;}
    Steinberg::Vst::IMidiMapping* midi_mapper() {return _midi_mapper;}
    bool notify_controller(Steinberg::Vst::IMessage* message);
    bool notify_processor(Steinberg::Vst::IMessage* message);

private:
    void _query_extension_interfaces();
    bool _connect_components();

    std::string _name;
    std::string _vendor;
    SushiHostApplication _host_app;
    std::shared_ptr<VST3::Hosting::Module> _module;

    // Reference counted pointers to plugin objects
    Steinberg::IPtr<Steinberg::Vst::IComponent>      _component;
    Steinberg::IPtr<Steinberg::Vst::IAudioProcessor> _processor;
    Steinberg::IPtr<Steinberg::Vst::IEditController> _controller;

    // Abstract optional interfaces implemented by one of the objects above:
    Steinberg::Vst::IMidiMapping*                    _midi_mapper{nullptr};
    Steinberg::Vst::IUnitInfo*                       _unit_info{nullptr};

    Steinberg::Vst::IConnectionPoint* _controller_connection_point{nullptr};
    Steinberg::Vst::IConnectionPoint* _component_connection_point{nullptr};
};

Steinberg::Vst::IComponent* load_component(Steinberg::IPluginFactory* factory, const std::string& plugin_name);
Steinberg::Vst::IAudioProcessor* load_processor(Steinberg::Vst::IComponent* component);
Steinberg::Vst::IEditController* load_controller(Steinberg::IPluginFactory* factory, Steinberg::Vst::IComponent*);


} // end namespace vst
} // end namespace sushi
#endif //SUSHI_VST3X_HOST_CONTEXT_H
