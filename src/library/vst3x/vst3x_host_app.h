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
 * @brief VST 3.x  plugin loader
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_VST3X_HOST_CONTEXT_H
#define SUSHI_VST3X_HOST_CONTEXT_H

#include "library/id_generator.h"
#include "library/constants.h"

#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstunits.h"
#include "pluginterfaces/vst/ivsthostapplication.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "public.sdk/source/vst/hosting/module.h"
#include "base/source/fobject.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wextra"
#include "public.sdk/source/vst/hosting/hostclasses.h"
#pragma GCC diagnostic pop

namespace sushi {
class HostControl;

namespace vst3 {

class Vst3xWrapper;
class ConnectionProxy;

class SushiHostApplication : public Steinberg::Vst::HostApplication
{
public:
    SushiHostApplication() : Steinberg::Vst::HostApplication() {}
    Steinberg::tresult getName (Steinberg::Vst::String128 name) override;
};

class ComponentHandler : public Steinberg::Vst::IComponentHandler, public Steinberg::FObject
{
public:
    SUSHI_DECLARE_NON_COPYABLE(ComponentHandler);

    explicit ComponentHandler(Vst3xWrapper* wrapper_instance, HostControl* host_control);
    Steinberg::tresult PLUGIN_API beginEdit (Steinberg::Vst::ParamID /*id*/) override {return Steinberg::kNotImplemented;}
    Steinberg::tresult PLUGIN_API performEdit (Steinberg::Vst::ParamID parameter_id, Steinberg::Vst::ParamValue normalized_value) override;
    Steinberg::tresult PLUGIN_API endEdit (Steinberg::Vst::ParamID /*parameter_id*/) override {return Steinberg::kNotImplemented;}
    Steinberg::tresult PLUGIN_API restartComponent (Steinberg::int32 flags) override;

    REFCOUNT_METHODS(Steinberg::FObject);
    DEF_INTERFACES_1(Steinberg::Vst::IComponentHandler, Steinberg::FObject);

private:
    Vst3xWrapper* _wrapper_instance;
    HostControl*  _host_control;
};

/**
 * @brief Container to hold plugin modules and manage their lifetimes
 */
class PluginInstance
{
public:
    SUSHI_DECLARE_NON_COPYABLE(PluginInstance);

    explicit PluginInstance(SushiHostApplication* host_app);
    ~PluginInstance();

    bool load_plugin(const std::string& plugin_path, const std::string& plugin_name);
    const std::string& name() const {return _name;}
    const std::string& vendor() const {return _vendor;}
    Steinberg::Vst::IComponent* component() const {return _component.get();}
    Steinberg::Vst::IAudioProcessor* processor() const {return _processor.get();}
    Steinberg::Vst::IEditController* controller() const {return _controller.get();}
    Steinberg::Vst::IUnitInfo* unit_info() {return _unit_info;}
    Steinberg::Vst::IMidiMapping* midi_mapper() {return _midi_mapper;}
    bool notify_controller(Steinberg::Vst::IMessage* message);
    bool notify_processor(Steinberg::Vst::IMessage* message);

private:
    void _query_extension_interfaces();
    bool _connect_components();

    std::string _name;
    std::string _vendor;

    SushiHostApplication* _host_app{nullptr};

    std::shared_ptr<VST3::Hosting::Module> _module;

    // Reference counted pointers to plugin objects
    Steinberg::OPtr<Steinberg::Vst::IComponent>      _component;
    Steinberg::OPtr<Steinberg::Vst::IAudioProcessor> _processor;
    Steinberg::OPtr<Steinberg::Vst::IEditController> _controller;

    // Abstract optional interfaces implemented by one of the objects above:
    Steinberg::OPtr<Steinberg::Vst::IMidiMapping>    _midi_mapper{nullptr};
    Steinberg::OPtr<Steinberg::Vst::IUnitInfo>       _unit_info{nullptr};

    Steinberg::OPtr<ConnectionProxy> _controller_connection;
    Steinberg::OPtr<ConnectionProxy> _component_connection;
};

Steinberg::Vst::IComponent* load_component(Steinberg::IPluginFactory* factory, const std::string& plugin_name);
Steinberg::Vst::IAudioProcessor* load_processor(Steinberg::Vst::IComponent* component);
Steinberg::Vst::IEditController* load_controller(Steinberg::IPluginFactory* factory, Steinberg::Vst::IComponent*);


} // end namespace vst
} // end namespace sushi
#endif //SUSHI_VST3X_HOST_CONTEXT_H
