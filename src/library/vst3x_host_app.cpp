#include <cstring>

#define DEVELOPMENT true

#include "pluginterfaces/base/ustring.h"
#include "public.sdk/source/vst/hosting/stringconvert.h"
#undef DEVELOPMENT

#include "vst3x_host_app.h"
#include "logging.h"

namespace sushi {
namespace vst3 {

MIND_GET_LOGGER;

constexpr char HOST_NAME[] = "Sushi";

Steinberg::tresult SushiHostApplication::getName(Steinberg::Vst::String128 name)

{
    // TODO, Eventually find a better way, building and including the
    // entire sdk lib for small util functions like this feels stupid.
    Steinberg::UString128 str(HOST_NAME);
    str.copyTo (name, 0);
    return Steinberg::kResultOk;
}

/*Steinberg::tresult HostApplication::createInstance(Steinberg::TUID cid,
                                                   Steinberg::TUID _iid,
                                                   void** obj)
{

}*/

PluginLoader::PluginLoader(const std::string& plugin_absolute_path, const std::string& plugin_name) :
                                                                               _path(plugin_absolute_path),
                                                                               _name(plugin_name)
{
    _host_app.release();
}


PluginLoader::~PluginLoader()
{}

std::pair<bool, PluginInstance> PluginLoader::load_plugin()
{
    std::string error_msg;
    PluginInstance instance;
    _module = VST3::Hosting::Module::create(_path, error_msg);
    if (!_module)
    {
        MIND_LOG_ERROR("Failed to load VST3 Module: {}", error_msg);
        return std::make_pair(false, instance);
    }
    auto factory = _module->getFactory().get();
    if (!factory)
    {
        MIND_LOG_ERROR("Failed to get PluginFactory, plugin is probably broken");
        return std::make_pair(false, instance);
    }

    auto component = load_component(factory, _name);
    if (!component)
    {
        return std::make_pair(false, instance);
    }
    auto res = component->initialize(&_host_app);
    if (res != Steinberg::kResultOk)
    {
        MIND_LOG_ERROR("Failed to initialize component with error code: {}", res);
        return std::make_pair(false, instance);
    }

    auto processor = load_processor(component);
    if (!processor)
    {
        MIND_LOG_ERROR("Failed to get processor from component");
        return std::make_pair(false, instance);
    }

    auto controller = load_controller(factory, component);
    if (!controller)
    {
        MIND_LOG_ERROR("Failed to load controller");
        return std::make_pair(false, instance);
    }
    res = controller->initialize(&_host_app);
    if (res != Steinberg::kResultOk)
    {
        MIND_LOG_ERROR("Failed to initialize component with error code: {}", res);
        return std::make_pair(false, instance);
    }

    instance._component = component;
    instance._processor = processor;
    instance._controller = controller;
    instance._name = _name;
    return std::make_pair(true, instance);
}


Steinberg::Vst::IComponent* load_component(Steinberg::IPluginFactory* factory,
                                           const std::string& plugin_name)
{
    for (int i = 0; i < factory->countClasses(); ++i)
    {
        Steinberg::PClassInfo info;
        factory->getClassInfo(0, &info);
        MIND_LOG_DEBUG("Querying plugin {} of type {}", info.name, info.category);
        if (info.name == plugin_name)
        {
            Steinberg::Vst::IComponent* component;
            auto res = factory->createInstance(info.cid, Steinberg::Vst::IComponent::iid,
                                               reinterpret_cast<void**>(&component));
            if (res == Steinberg::kResultOk)
            {
                MIND_LOG_INFO("Creating plugin {}", info.name);
                return component;
            }
            MIND_LOG_ERROR("Failed to create component with error code: {}", res);
            return nullptr;
        }
    }
    MIND_LOG_ERROR("No match for plugin {} in factory", plugin_name);
    return nullptr;
}

Steinberg::Vst::IAudioProcessor* load_processor(Steinberg::Vst::IComponent* component)
{
    /* This is how you properly cast the component to a processor */
    Steinberg::Vst::IAudioProcessor* processor;
    auto res = component->queryInterface (Steinberg::Vst::IAudioProcessor::iid,
                                     reinterpret_cast<void**>(&processor));
    if (res == Steinberg::kResultOk)
    {
        return processor;
    }
    return nullptr;
}

Steinberg::Vst::IEditController* load_controller(Steinberg::IPluginFactory* factory,
                                                  Steinberg::Vst::IComponent* component)
{
    /* The controller can be implemented both as a part of the component or
     * as a separate object, Steinberg recommends the latter though */
    Steinberg::Vst::IEditController* controller;

    auto res = component->queryInterface(Steinberg::Vst::IEditController::iid,
                                         reinterpret_cast<void**>(&controller));
    if (res == Steinberg::kResultOk)
    {
        return controller;
    }
    /* Else try to instatiate the controller as a separate object */
    Steinberg::FUID controllerID;
    if (component->getControllerClassId(controllerID) == Steinberg::kResultTrue && controllerID.isValid())
    {
        res = factory->createInstance(controllerID, Steinberg::Vst::IEditController::iid,
                                       reinterpret_cast<void**>(&controller));
        if (res == Steinberg::kResultOk)
        {
            return controller;
        }
        MIND_LOG_ERROR("Failed to create controller with error code: {}", res);
    }
    return nullptr;
}


} // end namespace vst3
} // end namespace sushi
