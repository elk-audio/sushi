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
    PORT, ///< Control port
    PROPERTY ///< Property (set via atom message)
};

/** Plugin control. */
struct ControlID
{
    LV2Model* model;
    ControlType type;
    LilvNode* node;
    LilvNode* symbol; ///< Symbol
    LilvNode* label; ///< Human readable label
    LV2_URID property; ///< Iff type == PROPERTY
    int index; ///< Iff type == PORT
    LilvNode* group; ///< Port/control group, or NULL

    std::vector<std::unique_ptr<ScalePoint>> points; ///< Scale points
    LV2_URID value_type; ///< Type of control value
    LilvNode* min; ///< Minimum value
    LilvNode* max; ///< Maximum value
    LilvNode* def; ///< Default value

    bool is_toggle; ///< Boolean (0 and 1 only)
    bool is_integer; ///< Integer values only
    bool is_enumeration; ///< Point values only
    bool is_logarithmic; ///< Logarithmic scale
    bool is_writable; ///< Writable (input)
    bool is_readable; ///< Readable (output)
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