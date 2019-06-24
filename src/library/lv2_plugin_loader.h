/**
 * @brief Utilities for loading plugins stored in dynamic libraries
 * @copyright 2017 MIND Music Labs AB, Stockholm
 *
 *
 // ILIAS TODO: Update.
 // DOES THE BELOW STILL APPLY?

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

#ifndef SUSHI_LV2_PLUGIN_LOADER_H
#define SUSHI_LV2_PLUGIN_LOADER_H

#include <string>

#pragma GCC diagnostic ignored "-Wunused-parameter"
//#define VST_FORCE_DEPRECATED 0

// Temporary - just to check that it finds them.
#include <lilv-0/lilv/lilv.h>

#include "lv2/atom/atom.h"
#include "lv2/atom/forge.h"
#include "lv2/data-access/data-access.h"
#include "lv2/log/log.h"
#include "lv2/midi/midi.h"
#include "lv2/options/options.h"
#include "lv2/resize-port/resize-port.h"
#include "lv2/state/state.h"
#include "lv2/urid/urid.h"
#include "lv2/worker/worker.h"

#pragma GCC diagnostic pop

#include <dlfcn.h>

namespace sushi {
namespace lv2 {

typedef void* LibraryHandle;

// ILIAS TODO: Declare non-copyable

class PluginLoader
{
public:
    PluginLoader();
    ~PluginLoader();

    const LilvPlugin* get_plugin_handle_from_URI(const std::string &plugin_URI_string);

    // Not yet implemented - may never be.
    // Lilv does not support absolute paths to plugins, but instead expects them to be in
    // the global paths for plugins.
    // static LibraryHandle get_library_handle_for_plugin_from_filepath(const std::string& plugin_absolute_path);

    void load_plugin(const LilvPlugin* plugin_handle, double sample_rate, const LV2_Feature** feature_list);

    void close_plugin_handle(const LilvPlugin* plugin_handle);

    LilvInstance* getPluginInstance();

private:
    LilvWorld* _world;
    LilvInstance* _plugin_instance;

};

} // namespace lv2
} // namespace sushi

#endif //SUSHI_LV2_PLUGIN_LOADER_H
