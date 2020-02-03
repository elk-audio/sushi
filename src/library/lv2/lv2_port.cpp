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

#include "logging.h"
#include "lv2_model.h"

namespace sushi {
namespace lv2 {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("lv2");

Port::Port(const LilvPlugin *plugin, int port_index, float default_value, LV2Model* model):
_control(0.0f),
_flow(FLOW_UNKNOWN),
_evbuf(nullptr), // For MIDI ports, otherwise NULL
_buf_size(0), // Custom buffer size, or 0
_index(port_index)
{
    lilv_port = lilv_plugin_get_port_by_index(plugin, port_index);

    const bool optional = lilv_port_has_property(plugin, lilv_port, model->get_nodes().lv2_connectionOptional);

    /* Set the port flow (input or output) */
    if (lilv_port_is_a(plugin, lilv_port, model->get_nodes().lv2_InputPort))
    {
        _flow = FLOW_INPUT;
    }
    else if (lilv_port_is_a(plugin, lilv_port, model->get_nodes().lv2_OutputPort))
    {
        _flow = FLOW_OUTPUT;
    }
    else if (!optional)
    {
        assert(false);
        SUSHI_LOG_ERROR("Mandatory LV2 port has unknown type (neither input nor output)");
        throw Port::FailedCreation();
    }

    const bool hidden = !_show_hidden &&
                        lilv_port_has_property(plugin, lilv_port, model->get_nodes().pprops_notOnGUI);

    /* Set control values */
    if (lilv_port_is_a(plugin, lilv_port, model->get_nodes().lv2_ControlPort))
    {
        _type = TYPE_CONTROL;

        LilvNode* minNode;
        LilvNode* maxNode;
        LilvNode* defNode;

        lilv_port_get_range(plugin, lilv_port, &defNode, &minNode, &maxNode);

        if(defNode != nullptr)
            _def = lilv_node_as_float(defNode);

        if(maxNode != nullptr)
            _max = lilv_node_as_float(maxNode);

        if(minNode != nullptr)
            _min = lilv_node_as_float(minNode);

        _control = isnan(default_value) ? _def : default_value;

        lilv_node_free(minNode);
        lilv_node_free(maxNode);
        lilv_node_free(defNode);

        if (!hidden)
        {
            model->get_controls().emplace_back(new_port_control(this, model, _index));
        }
    }
    else if (lilv_port_is_a(plugin, lilv_port, model->get_nodes().lv2_AudioPort))
    {
        _type = TYPE_AUDIO;
    }
    else if (lilv_port_is_a(plugin, lilv_port, model->get_nodes().atom_AtomPort))
    {
        _type = TYPE_EVENT;
    }
    else if (!optional)
    {
        assert(false);
        SUSHI_LOG_ERROR("Mandatory LV2 port has unknown data type");
        throw Port::FailedCreation();
    }

    if (!model->get_buf_size_set()) {
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

void Port::_allocate_port_buffers(LV2Model* model)
{
    switch (_type)
    {
        case TYPE_EVENT:
        {
            lv2_evbuf_free(_evbuf);

            auto handle = model->get_map().handle;

            _evbuf = lv2_evbuf_new(
                    model->get_midi_buffer_size(),
                    model->get_map().map(handle, lilv_node_as_string(model->get_nodes().atom_Chunk)),
                    model->get_map().map(handle, lilv_node_as_string(model->get_nodes().atom_Sequence)));

            lilv_instance_connect_port(
                    model->get_plugin_instance(), _index, lv2_evbuf_get_buffer(_evbuf));
        }
        default:
            break;
    }
}

float Port::get_min()
{
    return _min;
}

float Port::get_max()
{
    return _max;
}

PortFlow Port::get_flow()
{
    return _flow;
}

PortType Port::get_type()
{
    return _type;
}

const LilvPort* Port::get_lilv_port()
{
    return lilv_port;
}

LV2_Evbuf* Port::get_evbuf()
{
    return _evbuf;
}

void Port::set_control_value(float c)
{
    _control = c;
}

float Port::get_control_value()
{
    return _control;
}

float* Port::get_control_pointer()
{
    return &_control;
}

}
}

#endif //SUSHI_BUILD_WITH_LV2