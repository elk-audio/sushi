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


#ifndef SUSHI_LV2_PLUGIN_LOADER_H
#define SUSHI_LV2_PLUGIN_LOADER_H

#ifdef SUSHI_BUILD_WITH_LV2

#include <string>

#include "library/constants.h"

#include "lv2_model.h"

namespace sushi {
namespace lv2 {

class PluginLoader
{
public:
    SUSHI_DECLARE_NON_COPYABLE(PluginLoader);

    PluginLoader();
    ~PluginLoader();

    const LilvPlugin* plugin_handle_from_URI(const std::string &plugin_URI_string);

    void load_plugin(const LilvPlugin* plugin_handle, double sample_rate, const LV2_Feature** feature_list);

    void close_plugin_instance();

    Model* model();

private:
    Model* _model;
};

} // namespace lv2
} // namespace sushi

#endif //SUSHI_LV2_PLUGIN_LOADER_H

#endif //SUSHI_BUILD_WITH_LV2