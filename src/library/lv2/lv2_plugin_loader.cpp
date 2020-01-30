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
 * @Brief Wrapper for LV2 plugins - Wrapper for LV2 plugins - plugin loader.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "lv2_plugin_loader.h"

#include "lv2_state.h"
#include "lv2_control.h"

#include "logging.h"

namespace sushi {
namespace lv2 {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("lv2");

PluginLoader::PluginLoader()
{
    auto world = lilv_world_new();
    _model = new LV2Model(world);
}

PluginLoader::~PluginLoader()
{
    lilv_world_free(_model->get_world());
}

const LilvPlugin* PluginLoader::get_plugin_handle_from_URI(const std::string &plugin_URI_string)
{
    if (plugin_URI_string.empty())
    {
        SUSHI_LOG_ERROR("Empty library path");
        return nullptr; // Calling dlopen with an empty string returns a handle to the calling
        // program, which can cause an infinite loop.
    }

    auto plugins = lilv_world_get_all_plugins(_model->get_world());
    auto plugin_uri = lilv_new_uri(_model->get_world(), plugin_URI_string.c_str());

    if (!plugin_uri)
    {
        SUSHI_LOG_ERROR("Missing plugin URI, try lv2ls to list plugins.");
        return nullptr;
    }

    /* Find plugin */
    SUSHI_LOG_INFO("Plugin: {}", lilv_node_as_string(plugin_uri));
    const auto plugin  = lilv_plugins_get_by_uri(plugins, plugin_uri);
    lilv_node_free(plugin_uri);

    if (!plugin)
    {
        SUSHI_LOG_ERROR("Failed to find LV2 plugin.");
        return nullptr;
    }

    return plugin;
}

void PluginLoader::load_plugin(const LilvPlugin* plugin_handle, double sample_rate, const LV2_Feature** feature_list)
{
    /* Instantiate the plugin */
    _model->set_plugin_instance(lilv_plugin_instantiate(plugin_handle, sample_rate, feature_list));

    if (_model->get_plugin_instance() == nullptr)
    {
        SUSHI_LOG_ERROR("Failed instantiating LV2 plugin.");
        return;
    }

    _model->get_features().ext_data.data_access = lilv_instance_get_descriptor(_model->get_plugin_instance())->extension_data;

    auto state_threadSafeRestore = lilv_new_uri(_model->get_world(), LV2_STATE__threadSafeRestore);

    if (lilv_plugin_has_feature(plugin_handle, state_threadSafeRestore))
    {
        _model->set_restore_thread_safe(true);
    }

    lilv_node_free(state_threadSafeRestore);
}

void PluginLoader::close_plugin_instance()
{
    if (auto instance = _model->get_plugin_instance())
    {
        _model->trigger_exit();

        lilv_instance_deactivate(instance);
        lilv_instance_free(instance);

        for (unsigned i = 0; i < _model->get_controls().size(); ++i)
        {
            auto control = _model->get_controls()[i].get();
            lilv_node_free(control->node);
            lilv_node_free(control->symbol);
            lilv_node_free(control->label);
            lilv_node_free(control->group);
            lilv_node_free(control->min);
            lilv_node_free(control->max);
            lilv_node_free(control->def);
            free(control);
        }

        _model->set_plugin_instance(nullptr);
    }
}

LV2Model* PluginLoader::getModel()
{
    return _model;
}

} // namespace lv2
} // namespace sushi