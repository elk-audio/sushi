/**
* Parts taken and/or adapted from:
* MrsWatson - https://github.com/teragonaudio/MrsWatson
*
* Original copyright notice with BSD license:
* Copyright (c) 2013 Teragon Audio. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
*   this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright notice,
*   this list of conditions and the following disclaimer in the documentation
*   and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
#include <cstdlib>

#include <lilv-0/lilv/lilv.h>

#include "library/lv2_plugin_loader.h"
#include "library/lv2_host_callback.h"

#include "logging.h"

namespace sushi {
namespace lv2 {

MIND_GET_LOGGER_WITH_MODULE_NAME("lv2");

// Ilias TODO: Currently allocated plugin instances are not automatically freed when the loader is destroyed. Should they?

PluginLoader::PluginLoader()
{
    _world = lilv_world_new();

    // This allows loading plu-ins from their URI's, assuming they are installed in the correct paths
    // on the local machine.
    /* Find all installed plugins */
    lilv_world_load_all(_world);
    //jalv->world = world;
}

PluginLoader::~PluginLoader()
{
    lilv_world_free(_world);
}

LilvInstance* PluginLoader::getPluginInstance()
{
    return _plugin_instance;
}

const LilvPlugin* PluginLoader::get_plugin_handle_from_URI(const std::string &plugin_URI_string)
{
    if (plugin_URI_string.empty())
    {
        MIND_LOG_ERROR("Empty library path");
        return nullptr; // Calling dlopen with an empty string returns a handle to the calling
        // program, which can cause an infinite loop.
    }

    auto plugins = lilv_world_get_all_plugins(_world);
    auto plugin_uri = lilv_new_uri(_world, plugin_URI_string.c_str());

    if (!plugin_uri)
    {
        fprintf(stderr, "Missing plugin URI, try lv2ls to list plugins\n");
        // Ilias TODO: Handle error
        return nullptr;
    }

    /* Find plugin */
    printf("Plugin:       %s\n", lilv_node_as_string(plugin_uri));
    const LilvPlugin* plugin  = lilv_plugins_get_by_uri(plugins, plugin_uri);
    lilv_node_free(plugin_uri);

    if (!plugin)
    {
        fprintf(stderr, "Failed to find plugin\n");

        // Ilias TODO: Handle error

        return nullptr;
    }

    // Ilias TODO: Introduce state_threadSafeRestore later.

    // Ilias TODO: UI code - may not need it - it goes here though.

    return plugin;
}

void PluginLoader::load_plugin(const LilvPlugin* plugin_handle, double sample_rate, const LV2_Feature** feature_list)
{
    /* Instantiate the plugin */
    _plugin_instance = lilv_plugin_instantiate(
            plugin_handle,
            sample_rate,
            feature_list);

    if (_plugin_instance == nullptr)
    {
        fprintf(stderr, "Failed to instantiate plugin.\n");
        // Ilias TODO: Handle error
    }

    // Ilias TODO: Not sure this should be here.
    /* Activate plugin */
    lilv_instance_activate(_plugin_instance);
}

void PluginLoader::close_plugin_handle(const LilvPlugin* plugin_handle)
{
    assert(plugin_handle != nullptr);

    if (_plugin_instance != nullptr)
    {
        lilv_instance_deactivate(_plugin_instance);
        lilv_instance_free(_plugin_instance);
    }

    // Ilias TODO: Eventually also free plugin controls one loaded/mapped.

    _plugin_instance = nullptr;
}

} // namespace lv2
} // namespace sushi