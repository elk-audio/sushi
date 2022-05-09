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
 * @Brief State - internally used class for the storage and manipulation of LV2 presets/states.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_LV2_STATE_H
#define SUSHI_LV2_STATE_H

#include <vector>

#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "library/constants.h"
#include "lv2/state/state.h"
#include "lv2_model.h"
#include "lv2_features.h"

namespace sushi {
namespace lv2 {

class State
{
public:
    SUSHI_DECLARE_NON_COPYABLE(State);

    explicit State(Model* model);
    ~State() = default;

    void save(const char *dir);

    std::vector<std::byte> save_binary_state();

    void unload_programs();

    void apply_state(LilvState* state, bool delete_after_use);

    bool apply_program(int program_index);

    void apply_program(const LilvNode* preset);

    int save_program(const char* dir, const char* uri, const char* label, const char* filename);

    bool delete_current_program();

    std::vector<std::string>& program_names();

    void populate_program_list();

    int number_of_programs();

    int current_program_index() const;

    std::string current_program_name();

    std::string program_name(int program_index);
    
private:
    void _set_preset(LilvState* new_preset);

    void _load_programs(PresetSink sink, void* data);

    std::vector<std::string> _program_names;
    int _current_program_index{0};

    LilvState* _preset {nullptr}; // Naked pointer because Lilv manages lifetime.

    Model* _model;
};

// LV2 callbacks:
const void* get_port_value(const char* port_symbol, void* user_data, uint32_t *size, uint32_t* type);
void set_port_value(const char* port_symbol, void* user_data, const void* value, uint32_t size, uint32_t type);

}
}

#endif
