#ifndef SUSHI_VST3X_HOST_CONTEXT_H
#define SUSHI_VST3X_HOST_CONTEXT_H

#include <pluginterfaces/vst/ivsteditcontroller.h>
#include "pluginterfaces/vst/ivsthostapplication.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#define RELEASE = 1
#include "public.sdk/source/vst/hosting/module.h"
#pragma GCC diagnostic ignored "-Wextra"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#pragma GCC diagnostic pop
#undef RELEASE
namespace sushi {
namespace vst3 {

class ConnectionProxy;

class SushiHostApplication : public Steinberg::Vst::HostApplication
{
public:
    SushiHostApplication() : Steinberg::Vst::HostApplication() {}
    Steinberg::tresult getName (Steinberg::Vst::String128 name) override;
};

/**
 * @brief Container to hold plugin modules and manage their lifetimes
 */
class PluginInstance
{
public:
    PluginInstance();
    ~PluginInstance();

    bool load_plugin(const std::string& plugin_path, const std::string& plugin_name);
    const std::string& name() {return _name;};
    Steinberg::Vst::IComponent* component() {return _component.get();}
    Steinberg::Vst::IAudioProcessor* processor() {return _processor.get();}
    Steinberg::Vst::IEditController* controller() {return _controller.get();}

private:
    bool _connect_components();

    std::string _name;
    SushiHostApplication _host_app;
    std::shared_ptr<VST3::Hosting::Module> _module;

    Steinberg::IPtr<Steinberg::Vst::IComponent>      _component;
    Steinberg::IPtr<Steinberg::Vst::IAudioProcessor> _processor;
    Steinberg::IPtr<Steinberg::Vst::IEditController> _controller;

    Steinberg::OPtr<ConnectionProxy> _controller_connection;
    Steinberg::OPtr<ConnectionProxy> _component_connection;
};

Steinberg::Vst::IComponent* load_component(Steinberg::IPluginFactory* factory, const std::string& plugin_name);
Steinberg::Vst::IAudioProcessor* load_processor(Steinberg::Vst::IComponent* component);
Steinberg::Vst::IEditController* load_controller(Steinberg::IPluginFactory* factory, Steinberg::Vst::IComponent*);


} // end namespace vst
} // end namespace sushi
#endif //SUSHI_VST3X_HOST_CONTEXT_H
