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

#ifndef SUSHI_LV2_STATE_H
#define SUSHI_LV2_STATE_H

#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/state/state.h"
#include "lv2_model.h"
#include "lv2_features.h"

namespace sushi {
namespace lv2 {

class LV2_State
{
public:
    LV2_State(LV2Model* model);
    ~LV2_State();

    void save(const char *dir);

    int unload_programs();

    void apply_state(LilvState* state);

    int apply_program(const int program_index);

    int apply_program(const LilvNode* preset);

    int save_program(const char* dir, const char* uri, const char* label, const char* filename);

    int delete_current_program();

    std::vector<std::string>& get_program_names();

    void populate_program_list();

    int get_number_of_programs();

    int get_current_program_index();

    std::string get_current_program_name();

    std::string program_name(int program_index);
    
private:
    void set_preset(LilvState* new_preset);

    int _load_programs(PresetSink sink, void* data);

    std::vector<std::string> _program_names;
    int _current_program_index {0};

    // Naked pointer because Lilv mmanages lifetime.
    LilvState* _preset {nullptr};

    LV2Model* _model;
};

const void* get_port_value(const char* port_symbol, void* user_data, uint32_t *size, uint32_t* type);

void set_port_value(const char* port_symbol, void* user_data, const void* value, uint32_t size, uint32_t type);

}
}

#endif