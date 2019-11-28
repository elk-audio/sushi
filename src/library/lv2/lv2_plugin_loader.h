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


#ifndef SUSHI_LV2_PLUGIN_LOADER_H
#define SUSHI_LV2_PLUGIN_LOADER_H

#ifdef SUSHI_BUILD_WITH_LV2

#include <string>

#pragma GCC diagnostic ignored "-Wunused-parameter"
//#define VST_FORCE_DEPRECATED 0

#include "lv2_model.h"

#include "../constants.h"

#pragma GCC diagnostic pop

#include <dlfcn.h>

namespace sushi {
namespace lv2 {

class PluginLoader
{
public:
    SUSHI_DECLARE_NON_COPYABLE(PluginLoader)

    PluginLoader();
    ~PluginLoader();

    const LilvPlugin* get_plugin_handle_from_URI(const std::string &plugin_URI_string);

    // Not yet implemented - may never be.
    // The LV2 standard does not encourage absolute paths to plugins,
    // but instead expects them to be in the global paths it defines.
    // static LibraryHandle get_library_handle_for_plugin_from_filepath(const std::string& plugin_absolute_path);

    void load_plugin(const LilvPlugin* plugin_handle, double sample_rate, const LV2_Feature** feature_list);

    void close_plugin_instance();

    LV2Model* getModel();

private:
    LV2Model* _model;
};

} // namespace lv2
} // namespace sushi

#endif //SUSHI_LV2_PLUGIN_LOADER_H

#endif //SUSHI_BUILD_WITH_LV2