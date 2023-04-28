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
 * @Brief Port - internally used class for holding and interacting with a plugin port.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_LV2_PORT_H
#define SUSHI_LV2_PORT_H

#ifdef SUSHI_BUILD_WITH_LV2

#include <map>

#include <lilv-0/lilv/lilv.h>

#include "library/constants.h"

#include "lv2_host/lv2_evbuf.h"

namespace sushi {
namespace lv2 {

class Model;

enum class PortFlow
{
    FLOW_UNKNOWN,
    FLOW_INPUT,
    FLOW_OUTPUT
};

enum class PortType
{
    TYPE_UNKNOWN,
    TYPE_CONTROL,
    TYPE_AUDIO,
    TYPE_EVENT,
    TYPE_CV
};

class Port
{
public:
    Port(const LilvPlugin* plugin, int port_index, float default_value, Model* model);

    ~Port() = default;

    void reset_input_buffer();
    void reset_output_buffer();

    PortFlow flow() const;

    PortType type() const;

    const LilvPort* lilv_port();

    float min() const;
    float max() const;

    lv2_host::LV2_Evbuf* evbuf();

    void set_control_value(float c);

    float control_value() const;

    float* control_pointer();

    bool optional() const;

private:
    /**
    * @brief Allocates LV2 port buffers (only necessary for MIDI)
    */
    void _allocate_port_buffers(Model *model);

    float _control{0.0f}; ///< For control ports, otherwise 0.0f

    const LilvPort* _lilv_port;
    PortType _type{PortType::TYPE_UNKNOWN};
    PortFlow _flow{PortFlow::FLOW_UNKNOWN};

    lv2_host::LV2_Evbuf* _evbuf{nullptr}; // For MIDI ports, otherwise NULL

    int _index;

    // For ranges. Only used in control ports.
    float _def{1.0f};
    float _max{1.0f};
    float _min{0.0f};

    bool _show_hidden{true};

    bool _optional{true};
};

} // end namespace lv2
} // end namespace sushi


#endif // SUSHI_BUILD_WITH_LV2

#endif // SUSHI_LV2_PORT_H
