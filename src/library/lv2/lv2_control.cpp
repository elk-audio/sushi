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

#include "lv2_control.h"

#include "elklog/static_logger.h"

namespace sushi::internal::lv2 {

ELKLOG_GET_LOGGER_WITH_MODULE_NAME("lv2");

ControlID ControlID::new_port_control(Port* port, Model* model, uint32_t index)
{
    const auto lilvPort = port->lilv_port();
    const auto plugin = model->plugin_class();
    const auto nodes = model->nodes();

    ControlID id;
    id.model = model;
    id.type = ControlType::PORT;
    id.node = lilv_node_duplicate(lilv_port_get_node(plugin, lilvPort));
    id.symbol = lilv_node_duplicate(lilv_port_get_symbol(plugin, lilvPort));
    id.label = lilv_port_get_name(plugin, lilvPort);
    id.index = index;
    id.group = lilv_port_get(plugin, lilvPort, nodes->pg_group);
    id.value_type = model->forge().Float;
    id.is_writable = lilv_port_is_a(plugin, lilvPort, nodes->lv2_InputPort);
    id.is_readable = lilv_port_is_a(plugin, lilvPort, nodes->lv2_OutputPort);
    id.is_toggle = lilv_port_has_property(plugin, lilvPort, nodes->lv2_toggled);
    id.is_integer = lilv_port_has_property(plugin, lilvPort, nodes->lv2_integer);
    id.is_enumeration = lilv_port_has_property(plugin, lilvPort, nodes->lv2_enumeration);
    id.is_logarithmic = lilv_port_has_property(plugin, lilvPort, nodes->pprops_logarithmic);

    lilv_port_get_range(plugin, lilvPort, &id.def, &id.min, &id.max);
    if (lilv_port_has_property(plugin, lilvPort, nodes->lv2_sampleRate))
    {
        /* Adjust range for lv2:sampleRate controls */
        if (lilv_node_is_float(id.min) || lilv_node_is_int(id.min))
        {
            const float min = lilv_node_as_float(id.min) * model->sample_rate();
            lilv_node_free(id.min);
            id.min = lilv_new_float(model->lilv_world(), min);
        }
        if (lilv_node_is_float(id.max) || lilv_node_is_int(id.max))
        {
            const float max = lilv_node_as_float(id.max) * model->sample_rate();
            lilv_node_free(id.max);
            id.max = lilv_new_float(model->lilv_world(), max);
        }
    }

    /* Get scale points */
    auto scale_points = lilv_port_get_scale_points(plugin, lilvPort);
    if (scale_points)
    {
        LILV_FOREACH(scale_points, s, scale_points)
        {
            const auto scale_point = lilv_scale_points_get(scale_points, s);

            if (lilv_node_is_float(lilv_scale_point_get_value(scale_point)) ||
                lilv_node_is_int(lilv_scale_point_get_value(scale_point)))
            {
                auto sp = ScalePoint();

                sp.value = lilv_node_as_float(lilv_scale_point_get_value(scale_point));
                sp.label = lilv_node_as_string(lilv_scale_point_get_label(scale_point));
                id.scale_points.push_back(std::move(sp));
            }
        }

        std::sort(id.scale_points.begin(), id.scale_points.end(),
      [](const ScalePoint& a, const ScalePoint& b)
            {
                return a.value < b.value;
            }
         );

        lilv_scale_points_free(scale_points);
    }

    return id;
}

bool ControlID::has_range(Model* model, const LilvNode* subject, const char* range_uri)
{
    auto world = model->lilv_world();
    auto range = lilv_new_uri(world, range_uri);
    const bool result = lilv_world_ask(world, subject, model->nodes()->rdfs_range, range);
    lilv_node_free(range);
    return result;
}

ControlID ControlID::new_property_control(Model* model, const LilvNode* property)
{
    auto world = model->lilv_world();

    ControlID id;
    id.model = model;
    id.type = ControlType::PROPERTY;
    id.node = lilv_node_duplicate(property);
    id.symbol = lilv_world_get_symbol(world, property);
    id.label = lilv_world_get(world, property, model->nodes()->rdfs_label, nullptr);
    id.property = model->get_map().map(model, lilv_node_as_uri(property));

    id.min = lilv_world_get(world, property, model->nodes()->lv2_minimum, nullptr);
    id.max = lilv_world_get(world, property, model->nodes()->lv2_maximum, nullptr);
    id.def = lilv_world_get(world, property, model->nodes()->lv2_default, nullptr);

    const char *const types[] = {
            LV2_ATOM__Int, LV2_ATOM__Long, LV2_ATOM__Float, LV2_ATOM__Double,
            LV2_ATOM__Bool, LV2_ATOM__String, LV2_ATOM__Path, nullptr
    };

    for (const char *const *t = types; *t; ++t)
    {
        if (ControlID::has_range(model, property, *t))
        {
            id.value_type = model->get_map().map(model, *t);
            break;
        }
    }

    auto forge = model->forge();

    id.is_toggle = (id.value_type == forge.Bool);
    id.is_integer = (id.value_type == forge.Int ||
                      id.value_type == forge.Long);

    if (id.value_type == false)
    {
        ELKLOG_LOG_ERROR("Unknown value type for property {}", lilv_node_as_string(property));
    }

    return id;
}

} // end namespace sushi::internal::lv2
