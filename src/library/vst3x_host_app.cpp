#include <cstring>

#include "pluginterfaces/base/ustring.h"
#include "public.sdk/source/vst/hosting/stringconvert.h"

#include "vst3x_host_app.h"
#include "logging.h"

namespace sushi {
namespace vst3 {

MIND_GET_LOGGER_WITH_MODULE_NAME("vst3");

constexpr char HOST_NAME[] = "Sushi";

Steinberg::tresult SushiHostApplication::getName(Steinberg::Vst::String128 name)

{
    // TODO, Eventually find a better way, building and including the
    // entire sdk lib for small util functions like this feels stupid.
    Steinberg::UString128 str(HOST_NAME);
    str.copyTo (name, 0);
    return Steinberg::kResultOk;
}

// Ripped straight out of Steinberg example code (plugprovider.cpp)
class ConnectionProxy : public Steinberg::FObject, public Steinberg::Vst::IConnectionPoint
{
public:
    ConnectionProxy (Steinberg::Vst::IConnectionPoint* srcConnection);
    virtual ~ConnectionProxy ();

    //--- from IConnectionPoint
    Steinberg::tresult PLUGIN_API connect (Steinberg::Vst::IConnectionPoint* other) override;
    Steinberg::tresult PLUGIN_API disconnect (Steinberg::Vst::IConnectionPoint* other) override;
    Steinberg::tresult PLUGIN_API notify (Steinberg::Vst::IMessage* message) override;

    bool disconnect ();

    OBJ_METHODS (ConnectionProxy, FObject)
    REFCOUNT_METHODS (FObject)
    DEF_INTERFACES_1 (IConnectionPoint, FObject)

protected:
    Steinberg::IPtr<IConnectionPoint> srcConnection;
    Steinberg::IPtr<IConnectionPoint> dstConnection;
};

ConnectionProxy::ConnectionProxy (IConnectionPoint* srcConnection)
        : srcConnection (srcConnection) // share it
{}
// Use compiler generated version of the "big 5" but they are defined here, otherwise
// the definition of ConnectionProxy would leak out through the include file
ConnectionProxy::~ConnectionProxy() = default;
PluginInstance::PluginInstance(const PluginInstance& o) = default;
PluginInstance::PluginInstance(PluginInstance&& o) = default;
PluginInstance& PluginInstance::operator=(const PluginInstance& o) = default;
PluginInstance& PluginInstance::operator=(PluginInstance&& o) = default;


Steinberg::tresult PLUGIN_API ConnectionProxy::connect (IConnectionPoint* other)
{
    if (other == nullptr)
        return Steinberg::kInvalidArgument;
    if (dstConnection)
        return Steinberg::kResultFalse;

    dstConnection = other; // share it
    Steinberg::tresult res = srcConnection->connect (this);
    if (res != Steinberg::kResultTrue)
        dstConnection = nullptr;
    return res;
}

//------------------------------------------------------------------------
Steinberg::tresult PLUGIN_API ConnectionProxy::disconnect (IConnectionPoint* other)
{
    if (!other)
        return Steinberg::kInvalidArgument;

    if (other == dstConnection)
    {
        if (srcConnection)
            srcConnection->disconnect (this);
        dstConnection = nullptr;
        return Steinberg::kResultTrue;
    }

    return Steinberg::kInvalidArgument;
}

Steinberg::tresult PLUGIN_API ConnectionProxy::notify (Steinberg::Vst::IMessage* message)
{
    if (dstConnection)
    {
        // TODO we should test if we are in UI main thread else postpone the message
        return dstConnection->notify (message);
    }
    return Steinberg::kResultFalse;
}

bool ConnectionProxy::disconnect ()
{
    return disconnect (dstConnection) == Steinberg::kResultTrue;
}

PluginInstance::PluginInstance()
{
    // Constructor and destructor defined here and not in header, otherwise we
    // would need to put the definition of ConnectionProxy here too
}

PluginInstance::~PluginInstance()
{
    if(_componentProxy)
    {
        _componentProxy->disconnect();
    }
    if(_controllerProxy)
    {
        _controllerProxy->disconnect();
    }
}


PluginLoader::PluginLoader(const std::string& plugin_absolute_path, const std::string& plugin_name) :
                                                                               _path(plugin_absolute_path),
                                                                               _name(plugin_name)
{
    _host_app.release();
}


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
    if (_connect_components(instance) == false)
    {
        MIND_LOG_ERROR("Failed to connect component to editor");
        // Might still be ok? Plugin might not have an editor.
    }
    return {true, instance};
}

bool PluginLoader::_connect_components(PluginInstance& instance)
{
    if (!instance._component || !instance._controller)
    {
        MIND_LOG_ERROR("No controller");
        return false;
    }

    Steinberg::FUnknownPtr<Steinberg::Vst::IConnectionPoint> compICP(instance._component);
    Steinberg::FUnknownPtr<Steinberg::Vst::IConnectionPoint> contrICP(instance._controller);
    if (!compICP || !contrICP)
    {
        MIND_LOG_ERROR("Failed to create connection points");
        return false;
    }

    bool res = false;

    instance._componentProxy = NEW ConnectionProxy(compICP);
    instance._controllerProxy = NEW ConnectionProxy(contrICP);

    if (instance._componentProxy->connect(contrICP) != Steinberg::kResultTrue)
    {
        MIND_LOG_ERROR("Failed to connnect component");
    }
    else
    {
        if (instance._controllerProxy->connect(compICP) != Steinberg::kResultTrue)
        {
            MIND_LOG_ERROR("Failed to connnect controller");
        }
        else
            res = true;
    }
    return res;
}


Steinberg::Vst::IComponent* load_component(Steinberg::IPluginFactory* factory,
                                           const std::string& plugin_name)
{
    for (int i = 0; i < factory->countClasses(); ++i)
    {
        Steinberg::PClassInfo info;
        factory->getClassInfo(i, &info);
        MIND_LOG_INFO("Querying plugin {} of type {}", info.name, info.category);
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
    Steinberg::TUID controllerTUID;
    if (component->getControllerClassId(controllerTUID) == Steinberg::kResultTrue)
    {
        Steinberg::FUID controllerID(controllerTUID);
        if (controllerID.isValid())
        {
            res = factory->createInstance(controllerID, Steinberg::Vst::IEditController::iid,
                                          reinterpret_cast<void**>(&controller));
            if (res == Steinberg::kResultOk)
            {
                return controller;
            }
        }
        MIND_LOG_ERROR("Failed to create controller with error code: {}", res);
    }
    return nullptr;
}


} // end namespace vst3
} // end namespace sushi
