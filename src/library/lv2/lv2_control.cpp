/*
  Copyright 2007-2016 David Robillard <http://drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

#include "lv2_control.h"

namespace sushi {
namespace lv2 {

std::unique_ptr<ControlID> new_port_control(Port* port, LV2Model *model, uint32_t index)
{
    const auto lilvPort = port->get_lilv_port();
    const auto plugin = model->get_plugin_class();
    const auto nodes = &model->get_nodes();

    auto id = std::make_unique<ControlID>();
    id->model = model;
    id->type = ControlType::PORT;
    id->node = lilv_node_duplicate(lilv_port_get_node(plugin, lilvPort));
    id->symbol = lilv_node_duplicate(lilv_port_get_symbol(plugin, lilvPort));
    id->label = lilv_port_get_name(plugin, lilvPort);
    id->index = index;
    id->group = lilv_port_get(plugin, lilvPort, nodes->pg_group);
    id->value_type = model->get_forge().Float;
    id->is_writable = lilv_port_is_a(plugin, lilvPort, nodes->lv2_InputPort);
    id->is_readable = lilv_port_is_a(plugin, lilvPort, nodes->lv2_OutputPort);
    id->is_toggle = lilv_port_has_property(plugin, lilvPort, nodes->lv2_toggled);
    id->is_integer = lilv_port_has_property(plugin, lilvPort, nodes->lv2_integer);
    id->is_enumeration = lilv_port_has_property(plugin, lilvPort, nodes->lv2_enumeration);
    id->is_logarithmic = lilv_port_has_property(plugin, lilvPort, nodes->pprops_logarithmic);

    lilv_port_get_range(plugin, lilvPort, &id->def, &id->min, &id->max);
    if (lilv_port_has_property(plugin, lilvPort, nodes->lv2_sampleRate))
    {
        /* Adjust range for lv2:sampleRate controls */
        if (lilv_node_is_float(id->min) || lilv_node_is_int(id->min))
        {
            const float min = lilv_node_as_float(id->min) * model->get_sample_rate();
            lilv_node_free(id->min);
            id->min = lilv_new_float(model->get_world(), min);
        }
        if (lilv_node_is_float(id->max) || lilv_node_is_int(id->max))
        {
            const float max = lilv_node_as_float(id->max) * model->get_sample_rate();
            lilv_node_free(id->max);
            id->max = lilv_new_float(model->get_world(), max);
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
                auto sp = std::make_unique<ScalePoint>();

                sp->value = lilv_node_as_float(lilv_scale_point_get_value(scale_point));
                sp->label = lilv_node_as_string(lilv_scale_point_get_label(scale_point));
                id->points.emplace_back(std::move(sp));
            }
            /* TODO inherited: Non-float scale points? */
        }

        std::sort(id->points.begin(), id->points.end(),
                [](const std::unique_ptr<ScalePoint>& a, const std::unique_ptr<ScalePoint>& b)
                        {
                            return a->value < b->value;
                        }
         );

        lilv_scale_points_free(scale_points);
    }

    return id;
}

bool has_range(LV2Model* model, const LilvNode* subject, const char* range_uri)
{
    auto world = model->get_world();
    auto range = lilv_new_uri(world, range_uri);
    const bool result = lilv_world_ask(world, subject, model->get_nodes().rdfs_range, range);
    lilv_node_free(range);
    return result;
}

std::unique_ptr<ControlID> new_property_control(LV2Model *model, const LilvNode *property)
{
    auto world = model->get_world();

    auto id = std::make_unique<ControlID>();
    id->model = model;
    id->type = ControlType::PROPERTY;
    id->node = lilv_node_duplicate(property);
    id->symbol = lilv_world_get_symbol(world, property);
    id->label = lilv_world_get(world, property, model->get_nodes().rdfs_label, nullptr);
    id->property = model->get_map().map(model, lilv_node_as_uri(property));

    id->min = lilv_world_get(world, property, model->get_nodes().lv2_minimum, nullptr);
    id->max = lilv_world_get(world, property, model->get_nodes().lv2_maximum, nullptr);
    id->def = lilv_world_get(world, property, model->get_nodes().lv2_default, nullptr);

    const char *const types[] = {
            LV2_ATOM__Int, LV2_ATOM__Long, LV2_ATOM__Float, LV2_ATOM__Double,
            LV2_ATOM__Bool, LV2_ATOM__String, LV2_ATOM__Path, nullptr
    };

    for (const char *const *t = types; *t; ++t)
    {
        if (has_range(model, property, *t))
        {
            id->value_type = model->get_map().map(model, *t);
            break;
        }
    }

    auto forge = model->get_forge();

    id->is_toggle = (id->value_type == forge.Bool);
    id->is_integer = (id->value_type == forge.Int ||
                      id->value_type == forge.Long);

    if (!id->value_type)
    {
// TODO: Use ELK error log.
        fprintf(stderr, "Unknown value type for property <%s>\n", lilv_node_as_string(property));
    }

    return id;
}

/* TODO: Re-introduce if needed.
 * ControlID* get_property_control(std::vector<std::unique_ptr<ControlID>>* controls, LV2_URID property)
{
    for (size_t i = 0; i < controls->size(); ++i)
    {
        if (controls[i]->property == property)
        {
            return controls[i]->get();
        }
    }

    return nullptr;
}
*/
}
}