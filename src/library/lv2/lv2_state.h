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

char* make_path(LV2_State_Make_Path_Handle handle, const char* path);

const void* get_port_value(const char* port_symbol, void* user_data, uint32_t* size, uint32_t* type);

void save(LV2Model* model, const char* dir);

int load_presets(LV2Model* model, PresetSink sink, void* data);

int unload_presets(LV2Model* model);

void set_port_value(const char* port_symbol,
               void* user_data,
               const void* value,
               uint32_t size,
               uint32_t type);

void apply_state(LV2Model* model, LilvState* state);

int apply_preset(LV2Model* model, const LilvNode* preset);

int save_preset(LV2Model* model, const char* dir, const char* uri, const char* label, const char* filename);

int delete_current_preset(LV2Model* model);

}
}

#endif