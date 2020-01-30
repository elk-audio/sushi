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

#ifndef SUSHI_LV2_CONTROL_H
#define SUSHI_LV2_CONTROL_H

#ifdef SUSHI_BUILD_WITH_LV2

#include "lv2_model.h"

namespace sushi {
namespace lv2 {

struct ScalePoint
{
    float value;
    std::string label;
};

/** Type of plugin control. */
enum class ControlType
{
    PORT, // Control port
    PROPERTY // Property (set via atom message)
};

/** Plugin control. */
struct ControlID
{
    LV2Model* model;
    ControlType type;
    LilvNode* node;
    LilvNode* symbol; // Symbol
    LilvNode* label; // Human readable label
    LV2_URID property; // Iff type == PROPERTY
    int index; // Iff type == PORT
    LilvNode* group; // Port/control group, or NULL

    std::vector<std::unique_ptr<ScalePoint>> points; // Scale points
    LV2_URID value_type; // Type of control value
    LilvNode* min; // Minimum value
    LilvNode* max; // Maximum value
    LilvNode* def; // Default value

    bool is_toggle; // Boolean (0 and 1 only)
    bool is_integer; // Integer values only
    bool is_enumeration; // Point values only
    bool is_logarithmic; // Logarithmic scale
    bool is_writable; // Writable (input)
    bool is_readable; // Readable (output)
};

std::unique_ptr<ControlID> new_port_control(Port* port, LV2Model *model, uint32_t index);

bool has_range(LV2Model* model, const LilvNode* subject, const char* range_uri);

std::unique_ptr<ControlID> new_property_control(LV2Model *model, const LilvNode *property);

} // end namespace lv2
} // end namespace sushi

#endif //SUSHI_BUILD_WITH_LV2
#ifndef SUSHI_BUILD_WITH_LV2

// (...)

#endif

#endif //SUSHI_LV2_CONTROL_H