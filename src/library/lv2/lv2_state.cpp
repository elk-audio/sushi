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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <lilv-0/lilv/lilv.h>

#include "lv2_state.h"
#include "lv2_model.h"
#include "lv2_features.h"

namespace sushi {
namespace lv2 {

char* make_path(LV2_State_Make_Path_Handle handle, const char* path)
{
    LV2Model* model = (LV2Model*)handle;

	// Create in save directory if saving, otherwise use temp directory
	return lv2_strjoin(model->save_dir ? model->save_dir : model->temp_dir, path);
}

static const void* get_port_value(const char* port_symbol, void* user_data, uint32_t* size, uint32_t* type)
{
    LV2Model* model = (LV2Model*)user_data;

	struct Port* port = port_by_symbol(model, port_symbol);

	if (port && port->flow == FLOW_INPUT && port->type == TYPE_CONTROL)
	{
		*size = sizeof(float);
		*type = model->forge.Float;
		return &port->control;
	}

	*size = *type = 0;

	return NULL;
}

void save(LV2Model* model, const char* dir)
{
	model->save_dir = lv2_strjoin(dir, "/");

	LilvState* const state = lilv_state_new_from_instance(
		model->plugin, model->instance, &model->map,
		model->temp_dir, dir, dir, dir,
		get_port_value, model,
		LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE, NULL);

	lilv_state_save(model->world, &model->map, &model->unmap, state, NULL,
	                dir, "state.ttl");

	lilv_state_free(state);

	free(model->save_dir);
	model->save_dir = NULL;
}

int load_presets(LV2Model* model, PresetSink sink, void* data)
{
	LilvNodes* presets = lilv_plugin_get_related(model->plugin, model->nodes.pset_Preset);
	LILV_FOREACH(nodes, i, presets)
	{
		const LilvNode* preset = lilv_nodes_get(presets, i);
		lilv_world_load_resource(model->world, preset);
		if (!sink)
		{
			continue;
		}

		LilvNodes* labels = lilv_world_find_nodes(model->world, preset, model->nodes.rdfs_label, NULL);

		if (labels)
		{
			const LilvNode* label = lilv_nodes_get_first(labels);
			sink(model, preset, label, data);
			lilv_nodes_free(labels);
		}
		else
		{
			fprintf(stderr, "Preset <%s> has no rdfs:label\n",
			        lilv_node_as_string(lilv_nodes_get(presets, i)));
		}
	}
	lilv_nodes_free(presets);

	return 0;
}

int unload_presets(LV2Model* model)
{
	LilvNodes* presets = lilv_plugin_get_related(model->plugin, model->nodes.pset_Preset);
	LILV_FOREACH(nodes, i, presets)
	{
		const LilvNode* preset = lilv_nodes_get(presets, i);
		lilv_world_unload_resource(model->world, preset);
	}
	lilv_nodes_free(presets);

	return 0;
}

static void set_port_value(const char* port_symbol,
               void* user_data,
               const void* value,
               ZIX_UNUSED uint32_t size,
               uint32_t type)
{
    LV2Model* model = (LV2Model*)user_data;
	struct Port* port = port_by_symbol(model, port_symbol);
	if (!port)
	{
		fprintf(stderr, "error: Preset port `%s' is missing\n", port_symbol);
		return;
	}

	float fvalue;
	if (type == model->forge.Float)
	{
		fvalue = *(const float*)value;
	}
	else if (type == model->forge.Double)
	{
		fvalue = *(const double*)value;
	}
	else if (type == model->forge.Int)
	{
		fvalue = *(const int32_t*)value;
	}
	else if (type == model->forge.Long)
	{
		fvalue = *(const int64_t*)value;
	}
	else
	{
		fprintf(stderr, "error: Preset `%s' value has bad type <%s>\n",
                port_symbol, model->unmap.unmap(model->unmap.handle, type));
		return;
	}

	if (model->play_state != LV2_RUNNING)
	{
		// Set value on port struct directly
		port->control = fvalue;
	}
	else
	{
		// Send value to running plugin
// TODO: Reintroduce eventually
// Currently in lv2_ui_io.
//		ui_write(model, port->index, sizeof(fvalue), 0, &fvalue);
	}

	if (model->has_ui)
	{
		// Update UI
//		char buf[sizeof(ControlChange) + sizeof(fvalue)];
//		ControlChange* ev = (ControlChange*)buf;
//		ev->index = port->index;
//		ev->protocol = 0;
//		ev->size = sizeof(fvalue);
//		*(float*)ev->body = fvalue;
//		zix_ring_write(model->plugin_events, buf, sizeof(buf));
	}
}

void apply_state(LV2Model* model, LilvState* state)
{
	bool must_pause = !model->safe_restore && model->play_state == LV2_RUNNING;
	if (state)
	{
		if (must_pause)
		{
            model->play_state = LV2_PAUSE_REQUESTED;

            model->paused.wait();
		}

		const LV2_Feature* state_features[6] = {
			&model->_features.map_feature,
			&model->_features.unmap_feature,
// TODO: Implement make path Extension
//			&model->_features.make_path_feature,
			&model->_features.state_sched_feature,
			&model->_features.safe_restore_feature,
			&model->_features.log_feature,
// TODO: Implement Options Extension
//			&model->_features.options_feature,
			NULL
		};

        lilv_state_restore(state, model->instance, set_port_value, model, 0, state_features);

		if (must_pause)
		{
            model->request_update = true;
            model->play_state = LV2_RUNNING;
		}
	}
}

int apply_preset(LV2Model* model, const LilvNode* preset)
{
	lilv_state_free(model->preset);
    model->preset = lilv_state_new_from_world(model->world, &model->map, preset);
    apply_state(model, model->preset);
	return 0;
}

int save_preset(LV2Model* model, const char* dir, const char* uri, const char* label, const char* filename)
{
	LilvState* const state = lilv_state_new_from_instance(
            model->plugin, model->instance, &model->map,
            model->temp_dir, dir, dir, dir,
            get_port_value, model,
		LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE, NULL);

	if (label)
	{
		lilv_state_set_label(state, label);
	}

	int ret = lilv_state_save(model->world, &model->map, &model->unmap, state, uri, dir, filename);

	lilv_state_free(model->preset);
    model->preset = state;

	return ret;
}

int delete_current_preset(LV2Model* model)
{
	if (!model->preset)
	{
		return 1;
	}

	lilv_world_unload_resource(model->world, lilv_state_get_uri(model->preset));
	lilv_state_delete(model->world, model->preset);
	lilv_state_free(model->preset);
    model->preset = NULL;
	return 0;
}

}
}