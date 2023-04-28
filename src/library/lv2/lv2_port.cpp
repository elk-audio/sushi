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
 * @Brief Wrapper for LV2 plugins - Wrapper for LV2 plugins - port.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifdef SUSHI_BUILD_WITH_LV2

#include "lv2_port.h"

#include <math.h>

#include "lv2_model.h"

namespace sushi {
namespace lv2 {


Port::Port(const LilvPlugin* plugin, int port_index, float default_value, Model* model):
    _index(port_index)
{
    _lilv_port = lilv_plugin_get_port_by_index(plugin, port_index);

    _optional = lilv_port_has_property(plugin, _lilv_port, model->nodes()->lv2_connectionOptional);

    /* Set the port flow (input or output) */
    if (lilv_port_is_a(plugin, _lilv_port, model->nodes()->lv2_InputPort))
    {
        _flow = PortFlow::FLOW_INPUT;
    }
    else if (lilv_port_is_a(plugin, _lilv_port, model->nodes()->lv2_OutputPort))
    {
        _flow = PortFlow::FLOW_OUTPUT;
    }

    const bool hidden = (_show_hidden == false) &&
                        lilv_port_has_property(plugin, _lilv_port, model->nodes()->pprops_notOnGUI);

    /* Set control values */
    if (lilv_port_is_a(plugin, _lilv_port, model->nodes()->lv2_ControlPort))
    {
        _type = PortType::TYPE_CONTROL;

        LilvNode* minNode;
        LilvNode* maxNode;
        LilvNode* defNode;

        lilv_port_get_range(plugin, _lilv_port, &defNode, &minNode, &maxNode);

        if (defNode != nullptr)
        {
            _def = lilv_node_as_float(defNode);
        }

        if (maxNode != nullptr)
        {
            _max = lilv_node_as_float(maxNode);
        }

        if (minNode != nullptr)
        {
            _min = lilv_node_as_float(minNode);
        }

        _control = isnan(default_value) ? _def : default_value;

        lilv_node_free(minNode);
        lilv_node_free(maxNode);
        lilv_node_free(defNode);

        if (hidden == false)
        {
            model->controls().emplace_back(ControlID::new_port_control(this, model, _index));
        }
    }
    else if (lilv_port_is_a(plugin, _lilv_port, model->nodes()->lv2_AudioPort))
    {
        _type = PortType::TYPE_AUDIO;
    }
    else if (lilv_port_is_a(plugin, _lilv_port, model->nodes()->atom_AtomPort))
    {
        _type = PortType::TYPE_EVENT;
    }

    if (model->buf_size_set() == false)
    {
        _allocate_port_buffers(model);
    }
}

void Port::reset_input_buffer()
{
    lv2_evbuf_reset(_evbuf, true);
}

void Port::reset_output_buffer()
{
    lv2_evbuf_reset(_evbuf, false);
}

void Port::_allocate_port_buffers(Model* model)
{
    switch (_type)
    {
        case PortType::TYPE_EVENT:
        {
            lv2_evbuf_free(_evbuf);

            auto handle = model->get_map().handle;

            _evbuf = lv2_host::lv2_evbuf_new(
                    model->midi_buffer_size(),
                    model->get_map().map(handle, lilv_node_as_string(model->nodes()->atom_Chunk)),
                    model->get_map().map(handle, lilv_node_as_string(model->nodes()->atom_Sequence)));

            lilv_instance_connect_port(
                    model->plugin_instance(), _index, lv2_evbuf_get_buffer(_evbuf));
        }
        default:
            break;
    }
}

float Port::min() const
{
    return _min;
}

float Port::max() const
{
    return _max;
}

PortFlow Port::flow() const
{
    return _flow;
}

PortType Port::type() const
{
    return _type;
}

const LilvPort* Port::lilv_port()
{
    return _lilv_port;
}

lv2_host::LV2_Evbuf* Port::evbuf()
{
    return _evbuf;
}

void Port::set_control_value(float c)
{
    _control = c;
}

float Port::control_value() const
{
    return _control;
}

float* Port::control_pointer()
{
    return &_control;
}

bool Port::optional() const
{
    return _optional;
}

}
}

#endif //SUSHI_BUILD_WITH_LV2
