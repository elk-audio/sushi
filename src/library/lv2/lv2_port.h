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

#ifndef SUSHI_LV2_PORT_H
#define SUSHI_LV2_PORT_H

#ifdef SUSHI_BUILD_WITH_LV2

#include <exception>
#include <condition_variable>
#include <map>
#include <mutex>

#include <lilv-0/lilv/lilv.h>

#include "library/constants.h"

#include "third-party/lv2/lv2_evbuf.h"

namespace sushi {
namespace lv2 {

class LV2Model;

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
    SUSHI_DECLARE_NON_COPYABLE(Port);

    Port(const LilvPlugin* plugin, int port_index, float default_value, LV2Model* model);

    ~Port() = default;

    Port(Port &&) = default;

    void reset_input_buffer();
    void reset_output_buffer();

    PortFlow flow();

    PortType type();

    const LilvPort* lilv_port();

    float min();
    float max();

    LV2_Evbuf* evbuf();

    void set_control_value(float c);

    float control_value();

    float* control_pointer();

    struct FailedCreation : public std::exception
    {
        const char* what () const throw ()
        {
            return "Failed to Create LV2 Port";
        }
    };

private:
    /**
    * @brief Allocates LV2 port buffers (only necessary for MIDI)
    */
    void _allocate_port_buffers(LV2Model *model);

    float _control; ///< For control ports, otherwise 0.0f

    const LilvPort* _lilv_port;
    PortType _type;
    PortFlow _flow;

    LV2_Evbuf* _evbuf; // For MIDI ports, otherwise NULL

    void* _widget; // Control widget, if applicable
    int _buf_size; // Custom buffer size, or 0
    int _index;

    // For ranges. Only used in control ports.
    float _def {1.0f};
    float _max {1.0f};
    float _min {0.0f};

    bool _show_hidden {true};
};

} // end namespace lv2
} // end namespace sushi


#endif //SUSHI_BUILD_WITH_LV2

#endif //SUSHI_LV2_MODEL_H