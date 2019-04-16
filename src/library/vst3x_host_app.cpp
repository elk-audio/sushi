#include <cstring>

#include "pluginterfaces/base/ustring.h"
#include "public.sdk/source/vst/hosting/stringconvert.h"

#include "vst3x_host_app.h"
#include "vst3x_wrapper.h"
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

ComponentHandler::ComponentHandler(Vst3xWrapper* wrapper_instance) : _wrapper_instance(wrapper_instance)
{}

Steinberg::tresult ComponentHandler::performEdit(Steinberg::Vst::ParamID parameter_id,
                                                 Steinberg::Vst::ParamValue normalized_value)
{
    _wrapper_instance->set_parameter_change(ObjectId(parameter_id), static_cast<float>(normalized_value));
    return Steinberg::kResultOk;
}

/* ConnectionProxy is more or less ripped straight out of Steinberg example code.
 * But edited to follow Mind coding style. See plugprovider.cpp. */
class ConnectionProxy : public Steinberg::FObject, public Steinberg::Vst::IConnectionPoint
{
public:
    MIND_DECLARE_NON_COPYABLE(ConnectionProxy);

    explicit ConnectionProxy(Steinberg::Vst::IConnectionPoint* srcConnection) : _source_connection (srcConnection) {}
    virtual ~ConnectionProxy() = default;

    Steinberg::tresult PLUGIN_API connect(Steinberg::Vst::IConnectionPoint* other) override;
    Steinberg::tresult PLUGIN_API disconnect(Steinberg::Vst::IConnectionPoint* other) override;
    Steinberg::tresult PLUGIN_API notify(Steinberg::Vst::IMessage* message) override;

    bool disconnect();

    OBJ_METHODS(ConnectionProxy, FObject)
    REFCOUNT_METHODS(FObject)
    DEF_INTERFACES_1(IConnectionPoint, FObject)

protected:
    Steinberg::IPtr<IConnectionPoint> _source_connection;
    Steinberg::IPtr<IConnectionPoint> _dest_connection;
};

Steinberg::tresult PLUGIN_API ConnectionProxy::connect(IConnectionPoint* other)
{
    if (other == nullptr)
    {
        return Steinberg::kInvalidArgument;
    }

    if (_dest_connection)
    {
        return Steinberg::kResultFalse;
    }

    _dest_connection = other;
    Steinberg::tresult res = _source_connection->connect(this);
    if (res != Steinberg::kResultTrue)
    {
        _dest_connection = nullptr;
    }
    return res;
}

Steinberg::tresult PLUGIN_API ConnectionProxy::disconnect(IConnectionPoint* other)
{
    if (other == nullptr)
    {
        return Steinberg::kInvalidArgument;
    }

    if (other == _dest_connection)
    {
        if (_source_connection)
        {
            _source_connection->disconnect(this);
        }
        _dest_connection = nullptr;
        return Steinberg::kResultTrue;
    }
    return Steinberg::kInvalidArgument;
}

Steinberg::tresult PLUGIN_API ConnectionProxy::notify(Steinberg::Vst::IMessage* message)
{
    if (_dest_connection)
    {
        // TODO we should test if we are in UI main thread else postpone the message
        // Steinberg comment, maybe we should add a check here
        return _dest_connection->notify(message);
    }
    return Steinberg::kResultFalse;
}

bool ConnectionProxy::disconnect()
{
    auto status =  disconnect(_dest_connection);
    return  status == Steinberg::kResultTrue;
}

PluginInstance::PluginInstance()
{
    // Constructor defined here and not in header file, otherwise we
    // would need to put the definition of ConnectionProxy here too
}

PluginInstance::~PluginInstance()
{
    if(_component_connection)
    {
        _component_connection->disconnect();
    }
    if(_controller_connection)
    {
        _controller_connection->disconnect();
    }
}

bool PluginInstance::load_plugin(const std::string& plugin_path, const std::string& plugin_name)
{
    std::string error_msg;
    _module = VST3::Hosting::Module::create(plugin_path, error_msg);
    if (!_module)
    {
        MIND_LOG_ERROR("Failed to load VST3 Module: {}", error_msg);
        return false;
    }
    auto factory = _module->getFactory().get();
    if (!factory)
    {
        MIND_LOG_ERROR("Failed to get PluginFactory, plugin is probably broken");
        return false;
    }

    auto component = load_component(factory, plugin_name);
    if (!component)
    {
        return false;
    }
    auto res = component->initialize(&_host_app);
    if (res != Steinberg::kResultOk)
    {
        MIND_LOG_ERROR("Failed to initialize component with error code: {}", res);
        return false;
    }

    auto processor = load_processor(component);
    if (!processor)
    {
        MIND_LOG_ERROR("Failed to get processor from component");
        return false;
    }

    auto controller = load_controller(factory, component);
    if (!controller)
    {
        MIND_LOG_ERROR("Failed to load controller");
        return false;
    }

    res = controller->initialize(&_host_app);
    if (res != Steinberg::kResultOk)
    {
        MIND_LOG_ERROR("Failed to initialize component with error code: {}", res);
        return false;
    }

    _component = component;
    _processor = processor;
    _controller = controller;
    _name = plugin_name;

    _query_extension_interfaces();

    if (_connect_components() == false)
    {
        MIND_LOG_ERROR("Failed to connect component to editor");
        // Might still be ok? Plugin might not have an editor.
    }
    return true;
}

void PluginInstance::_query_extension_interfaces()
{
    Steinberg::Vst::IMidiMapping* midi_mapper;
    auto res = _controller->queryInterface(Steinberg::Vst::IMidiMapping::iid, reinterpret_cast<void**>(&midi_mapper));
    if (res == Steinberg::kResultOk)
    {
        _midi_mapper = midi_mapper;
        MIND_LOG_INFO("Plugin supports Midi Mapping interface");
    }
    Steinberg::Vst::IUnitInfo* unit_info;
    res = _controller->queryInterface(Steinberg::Vst::IUnitInfo::iid, reinterpret_cast<void**>(&unit_info));
    if (res == Steinberg::kResultOk)
    {
        _unit_info = unit_info;
        MIND_LOG_INFO("Plugin supports Unit Info interface for programs");
    }
}

bool PluginInstance::_connect_components()
{
    Steinberg::FUnknownPtr<Steinberg::Vst::IConnectionPoint> compICP(_component);
    Steinberg::FUnknownPtr<Steinberg::Vst::IConnectionPoint> contrICP(_controller);
    if (!compICP || !contrICP)
    {
        MIND_LOG_ERROR("Failed to create connection points");
        return false;
    }

    _component_connection = NEW ConnectionProxy(compICP);
    _controller_connection = NEW ConnectionProxy(contrICP);

    if (_component_connection->connect(contrICP) != Steinberg::kResultTrue)
    {
        MIND_LOG_ERROR("Failed to connect component");
    }
    else
    {
        if (_controller_connection->connect(compICP) != Steinberg::kResultTrue)
        {
            MIND_LOG_ERROR("Failed to connect controller");
        }
        else
        {
            return true;
        }
    }
    return false;
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
