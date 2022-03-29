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
 * @Brief Wrapper for LV2 plugins - Wrapper for LV2 plugins - extra features.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifdef SUSHI_BUILD_WITH_LV2

#include "lv2_features.h"

#include "logging.h"

namespace sushi {
namespace lv2 {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("lv2");

Port* port_by_symbol(Model* model, const char* sym)
{
    for (int i = 0; i < model->port_count(); ++i)
    {
        auto port = model->get_port(i);
        const auto port_symbol = lilv_port_get_symbol(model->plugin_class(), port->lilv_port());

        if (strcmp(lilv_node_as_string(port_symbol), sym) == 0)
        {
            return port;
        }
    }

    return nullptr;
}

int lv2_vprintf(LV2_Log_Handle handle,
                LV2_URID type,
                [[maybe_unused]] const char* fmt,
                va_list /*ap*/)
{
    auto model = static_cast<Model*>(handle);
    auto urids = model->urids();

    if (type == urids.log_Trace && TRACE_OPTION)
    {
        SUSHI_LOG_DEBUG("LV2 Trace: {}", fmt);
    }
    else if (type == urids.log_Error)
    {
        SUSHI_LOG_DEBUG("LV2 Error: {}", fmt);
    }
    else if (type == urids.log_Warning)
    {
        SUSHI_LOG_DEBUG("LV2 Warning: {}", fmt);
    }
    else if (type == urids.log_Entry)
    {
        SUSHI_LOG_DEBUG("LV2 Entry: {}", fmt);
    }
    else if (type == urids.log_Note)
    {
        SUSHI_LOG_DEBUG("LV2 Note: {}", fmt);
    }
    else if (type == urids.log_log)
    {
        SUSHI_LOG_DEBUG("LV2 Log: {}", fmt);
    }
    else
    {
        SUSHI_LOG_DEBUG("LV2 unknown error: {}", fmt);
    }

    return 0;
}

int lv2_printf(LV2_Log_Handle handle,
               LV2_URID type,
               const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    const int ret = lv2_vprintf(handle, type, fmt, args);
    va_end(args);
    return ret;
}

char* make_path(LV2_State_Make_Path_Handle handle, const char* path)
{
    auto model = static_cast<Model*>(handle);

    // Create in save directory if saving, otherwise use temp directory

    std::string made_path;

    if (model->save_dir().empty() == false)
    {
        made_path = model->save_dir() + path;
    }
    else
    {
        made_path = model->temp_dir() + path;
    }

    const std::string::size_type size = made_path.size();
    char* buffer = new char[size + 1]; // Extra char for null
    memcpy(buffer, made_path.c_str(), size + 1);
    return buffer;
}

LV2_URID map_uri(LV2_URID_Map_Handle handle, const char* uri)
{
    auto model = static_cast<Model*>(handle);
    return model->map(uri);
}

const char* unmap_uri(LV2_URID_Unmap_Handle handle, LV2_URID urid)
{
    auto model = static_cast<Model*>(handle);
    return model->unmap(urid);
}

void init_feature(LV2_Feature* const dest, const char* const URI, void* data)
{
    dest->URI = URI;
    dest->data = data;
}

}
}

#endif //SUSHI_BUILD_WITH_LV2
