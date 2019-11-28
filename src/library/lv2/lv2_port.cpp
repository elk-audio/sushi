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
 * @Brief Wrapper for LV2 plugins - Wrapper for LV2 plugins - port.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifdef SUSHI_BUILD_WITH_LV2

#include <math.h>

#include "lv2_model.h"
#include "lv2_features.h"
#include "lv2_worker.h"
#include "lv2_state.h"
#include "logging.h"

namespace sushi {
namespace lv2 {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("lv2");

// TODO: Pass model as const eventually
Port::Port(const LilvPlugin *plugin, int port_index, float default_value, LV2Model* model):
_index(port_index),
control(0.0f),
_flow(FLOW_UNKNOWN),
_evbuf(nullptr), // For MIDI ports, otherwise NULL
_buf_size(0) // Custom buffer size, or 0
{
    lilv_port = lilv_plugin_get_port_by_index(plugin, port_index);

    // TODO: The below is not used in lv2apply example... Reinstate?
    //port->sys_port = NULL; // For audio/MIDI ports, otherwise NULL

    const bool optional = lilv_port_has_property(plugin, lilv_port, model->nodes.lv2_connectionOptional);

    /* Set the port flow (input or output) */
    if (lilv_port_is_a(plugin, lilv_port, model->nodes.lv2_InputPort))
    {
        _flow = FLOW_INPUT;
    }
    else if (lilv_port_is_a(plugin, lilv_port, model->nodes.lv2_OutputPort))
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
                        lilv_port_has_property(plugin, lilv_port, model->nodes.pprops_notOnGUI);

    /* Set control values */
    if (lilv_port_is_a(plugin, lilv_port, model->nodes.lv2_ControlPort))
    {
        _type = TYPE_CONTROL;

// TODO: min max def are used also in the Jalv control structure. Remove these eventually?
        LilvNode* minNode;
        LilvNode* maxNode;
        LilvNode* defNode;

        lilv_port_get_range(plugin, lilv_port, &defNode, &minNode, &maxNode);

        if(defNode != nullptr)
            def = lilv_node_as_float(defNode);

        if(maxNode != nullptr)
            max = lilv_node_as_float(maxNode);

        if(minNode != nullptr)
            min = lilv_node_as_float(minNode);

        control = isnan(default_value) ? def : default_value;

        lilv_node_free(minNode);
        lilv_node_free(maxNode);
        lilv_node_free(defNode);

        if (!hidden)
        {
            add_control(&model->controls, new_port_control(this, model, _index));
        }
    }
    else if (lilv_port_is_a(plugin, lilv_port, model->nodes.lv2_AudioPort))
    {
        _type = TYPE_AUDIO;

// TODO: CV port(s).
//#ifdef HAVE_JACK_METADATA
//        } else if (lilv_port_is_a(model->plugin, port->lilv_port,
//                      model->nodes.lv2_CVPort)) {
//port->type = TYPE_CV;
//#endif

    }
    else if (lilv_port_is_a(plugin, lilv_port, model->nodes.atom_AtomPort))
    {
        _type = TYPE_EVENT;
    }
    else if (!optional)
    {
        assert(false);
        SUSHI_LOG_ERROR("Mandatory LV2 port has unknown data type");
        throw Port::FailedCreation();
    }

    if (!model->buf_size_set) {
        _allocate_port_buffers(model);
    }
}

void Port::_allocate_port_buffers(LV2Model* model)
{
    switch (_type)
    {
        case TYPE_EVENT: {
            lv2_evbuf_free(_evbuf);
            const size_t buf_size = (buf_size > 0)
                                    ? buf_size
                                    : model->midi_buf_size;
            _evbuf = lv2_evbuf_new(
                    buf_size,
                    model->map.map(model->map.handle,
                                   lilv_node_as_string(model->nodes.atom_Chunk)),
                    model->map.map(model->map.handle,
                                   lilv_node_as_string(model->nodes.atom_Sequence)));
            lilv_instance_connect_port(
                    model->instance, _index, lv2_evbuf_get_buffer(_evbuf));
        }
        default:
            break;
    }
}

}
}

#endif //SUSHI_BUILD_WITH_LV2