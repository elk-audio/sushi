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

    std::vector<std::string>&  get_program_names();

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