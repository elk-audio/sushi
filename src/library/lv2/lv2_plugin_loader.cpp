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
 * SUSHI. If not, see http://www.gnu.org/licenses/
 */

/**
* Parts adapted from Jalv LV2 host:
* Copyright 2007-2016 David Robillard <http://drobilla.net>

* Permission to use, copy, modify, and/or distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.

* THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
* ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
* ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
* OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/**
 * @Brief Wrapper for LV2 plugins - Wrapper for LV2 plugins - plugin loader.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "lv2_plugin_loader.h"

#include "lv2_worker.h"
#include "lv2_state.h"

#include "logging.h"

namespace sushi {
namespace lv2 {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("lv2");

PluginLoader::PluginLoader()
{
    LilvWorld* world = lilv_world_new();

    _model = new LV2Model(world);
}

PluginLoader::~PluginLoader()
{
    lilv_world_free(_model->world);
}

const LilvPlugin* PluginLoader::get_plugin_handle_from_URI(const std::string &plugin_URI_string)
{
    if (plugin_URI_string.empty())
    {
        SUSHI_LOG_ERROR("Empty library path");
        return nullptr; // Calling dlopen with an empty string returns a handle to the calling
        // program, which can cause an infinite loop.
    }

    auto plugins = lilv_world_get_all_plugins(_model->world);
    auto plugin_uri = lilv_new_uri(_model->world, plugin_URI_string.c_str());

    if (!plugin_uri)
    {
        SUSHI_LOG_ERROR("Missing plugin URI, try lv2ls to list plugins.");
        return nullptr;
    }

    /* Find plugin */
    SUSHI_LOG_INFO("Plugin: {}", lilv_node_as_string(plugin_uri));
    const LilvPlugin* plugin  = lilv_plugins_get_by_uri(plugins, plugin_uri);
    lilv_node_free(plugin_uri);

    if (!plugin)
    {
        SUSHI_LOG_ERROR("Failed to find LV2 plugin.");
        return nullptr;
    }

// TODO: Introduce necessary UI code from Jalv.

    return plugin;
}

void PluginLoader::load_plugin(const LilvPlugin* plugin_handle, double sample_rate, const LV2_Feature** feature_list)
{
    /* Instantiate the plugin */
    _model->instance = lilv_plugin_instantiate(
            plugin_handle,
            sample_rate,
            feature_list);

    if (_model->instance == nullptr)
    {
        SUSHI_LOG_ERROR("Failed instantiating LV2 plugin.");
        return;
    }

    _model->_features.ext_data.data_access = lilv_instance_get_descriptor(_model->instance)->extension_data;

    /* Create workers if necessary */
    if (lilv_plugin_has_extension_data(plugin_handle, _model->nodes.work_interface))
    {
        const LV2_Worker_Interface* iface = (const LV2_Worker_Interface*)
                lilv_instance_get_extension_data(_model->instance, LV2_WORKER__interface);

        lv2_worker_init(_model, &_model->worker, iface, true);
        if (_model->safe_restore)
        {
            lv2_worker_init(_model, &_model->state_worker, iface, false);
        }
    }

    LilvNode* state_threadSafeRestore = lilv_new_uri(_model->world, LV2_STATE__threadSafeRestore);
    if (lilv_plugin_has_feature(plugin_handle, state_threadSafeRestore))
    {
        _model->safe_restore = true;
    }
    lilv_node_free(state_threadSafeRestore);
}

void PluginLoader::close_plugin_instance()
{
// TODO: Currently, as this builds on the JALV example, only a single plugin is supported.
// Refactor to allow multiple plugins!

    if (_model->instance != nullptr)
    {
        _model->exit = true;

        /* Terminate the worker */
        lv2_worker_finish(&_model->worker);

        lilv_instance_deactivate(_model->instance);
        lilv_instance_free(_model->instance);

        lv2_worker_destroy(&_model->worker);

        for (unsigned i = 0; i < _model->controls.size(); ++i)
        {
            ControlID* const control = _model->controls[i].get();
            lilv_node_free(control->node);
            lilv_node_free(control->symbol);
            lilv_node_free(control->label);
            lilv_node_free(control->group);
            lilv_node_free(control->min);
            lilv_node_free(control->max);
            lilv_node_free(control->def);
            free(control);
        }

        _model->instance = nullptr;
    }
}

LV2Model* PluginLoader::getModel()
{
    return _model;
}

} // namespace lv2
} // namespace sushi