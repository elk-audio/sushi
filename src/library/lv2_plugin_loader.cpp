/**
* Parts taken and/or adapted from Jalv LV2 host:
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

#include "lv2_plugin_loader.h"

#include "logging.h"

namespace sushi {
namespace lv2 {

MIND_GET_LOGGER_WITH_MODULE_NAME("lv2");

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
        MIND_LOG_ERROR("Empty library path");
        return nullptr; // Calling dlopen with an empty string returns a handle to the calling
        // program, which can cause an infinite loop.
    }

    auto plugins = lilv_world_get_all_plugins(_model->world);
    auto plugin_uri = lilv_new_uri(_model->world, plugin_URI_string.c_str());

    if (!plugin_uri)
    {
        MIND_LOG_ERROR("Missing plugin URI, try lv2ls to list plugins.");
        return nullptr;
    }

    /* Find plugin */
    MIND_LOG_INFO("Plugin: {}", lilv_node_as_string(plugin_uri));
    const LilvPlugin* plugin  = lilv_plugins_get_by_uri(plugins, plugin_uri);
    lilv_node_free(plugin_uri);

    if (!plugin)
    {
        MIND_LOG_ERROR("Failed to find LV2 plugin.");
        return nullptr;
    }

    // TODO: Introduce state_threadSafeRestore later.

    // TODO: Introduce necessary UI code

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
        MIND_LOG_ERROR("Failed instantiating LV2 plugin.");
        return;
    }

    /* Activate plugin */
    lilv_instance_activate(_model->instance);
}

void PluginLoader::close_plugin_instance()
{
    // TODO: Currently, as this builds on the JALV example, only a single plugin is supported.
    // Refactor to allow multiple olugins!

    if (_model->instance != nullptr)
    {
        _model->exit = true;
        lilv_instance_deactivate(_model->instance);
        lilv_instance_free(_model->instance);

        /*for (unsigned i = 0; i < _model.controls.n_controls; ++i) {
            ControlID* const control = _model.controls.controls[i];
            lilv_node_free(control->node);
            lilv_node_free(control->symbol);
            lilv_node_free(control->label);
            lilv_node_free(control->group);
            lilv_node_free(control->min);
            lilv_node_free(control->max);
            lilv_node_free(control->def);
            free(control);
        }
        free(model->controls.controls);*/

        _model->instance = nullptr;
    }
}

LV2Model* PluginLoader::getModel()
{
    return _model;
}

} // namespace lv2
} // namespace sushi