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

#include <lilv-0/lilv/lilv.h>

#include "lv2_state.h"
#include "lv2_model.h"
#include "lv2_port.h"
#include "lv2_features.h"

namespace sushi {
namespace lv2 {

// This one has a footprint as required by lilv.
const void* get_port_value(const char* port_symbol, void* user_data, uint32_t* size, uint32_t* type)
{
    auto model = static_cast<LV2Model*>(user_data);
	auto port = port_by_symbol(model, port_symbol);

	if (port && port->getFlow() == FLOW_INPUT && port->getType() == TYPE_CONTROL)
	{
		*size = sizeof(float);
		*type = model->get_forge().Float;
		return &port->control;
	}

	*size = *type = 0;

	return nullptr;
}

void save(LV2Model* model, const char* dir)
{
	model->save_dir = std::string(dir) + "/";

	auto state = lilv_state_new_from_instance(
		model->get_plugin_class(), model->get_plugin_instance(), &model->get_map(),
		model->temp_dir.c_str(), dir, dir, dir,
		get_port_value, model,
		LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE, nullptr);

	lilv_state_save(model->get_world(), &model->get_map(), &model->get_unmap(), state, nullptr, dir, "state.ttl");

	lilv_state_free(state);

	model->save_dir.clear();
}

int load_presets(LV2Model* model, PresetSink sink, void* data)
{
	auto presets = lilv_plugin_get_related(model->get_plugin_class(), model->get_nodes().pset_Preset);

	LILV_FOREACH(nodes, i, presets)
	{
		auto preset = lilv_nodes_get(presets, i);
		lilv_world_load_resource(model->get_world(), preset);

		if (!sink)
		{
			continue;
		}

		auto labels = lilv_world_find_nodes(model->get_world(), preset, model->get_nodes().rdfs_label, NULL);

		if (labels)
		{
			auto label = lilv_nodes_get_first(labels);
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
	auto presets = lilv_plugin_get_related(model->get_plugin_class(), model->get_nodes().pset_Preset);

	LILV_FOREACH(nodes, i, presets)
	{
		auto preset = lilv_nodes_get(presets, i);
		lilv_world_unload_resource(model->get_world(), preset);
	}
	lilv_nodes_free(presets);

	return 0;
}

void set_port_value(const char* port_symbol,
               void* user_data,
               const void* value,
               uint32_t size,
               uint32_t type)
{
    auto model = static_cast<LV2Model*>(user_data);
	auto port = port_by_symbol(model, port_symbol);

	if (!port)
	{
		fprintf(stderr, "error: Preset port `%s' is missing\n", port_symbol);
		return;
	}

	float fvalue;

	auto forge = model->get_forge();

	if (type == forge.Float)
	{
		fvalue = *(const float*)value;
	}
	else if (type == forge.Double)
	{
		fvalue = *(const double*)value;
	}
	else if (type == forge.Int)
	{
		fvalue = *(const int32_t*)value;
	}
	else if (type == forge.Long)
	{
		fvalue = *(const int64_t*)value;
	}
	else
	{
		fprintf(stderr, "error: Preset `%s' value has bad type <%s>\n",
                port_symbol, model->get_unmap().unmap(model->get_unmap().handle, type));
		return;
	}

	if (model->play_state != PlayState::RUNNING)
	{
		// Set value on port directly
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
//		zix_ring_write(model->_plugin_events, buf, sizeof(buf));
	}
}

void apply_state(LV2Model* model, LilvState* state)
{
	bool must_pause = !model->is_restore_thread_safe() && model->play_state == PlayState::RUNNING;

	if (state)
	{
		if (must_pause)
		{
            model->play_state = PlayState::PAUSE_REQUESTED;

            model->paused.wait();
		}

        auto feature_list = model->get_feature_list();

        lilv_state_restore(state, model->get_plugin_instance(), set_port_value, model, 0, feature_list->data());

		if (must_pause)
		{
            model->request_update();
            model->play_state = PlayState::RUNNING;
		}
	}
}

int apply_preset(LV2Model* model, const LilvNode* preset)
{
    model->set_preset(lilv_state_new_from_world(model->get_world(), &model->get_map(), preset));
    apply_state(model, model->get_preset());
	return 0;
}

int save_preset(LV2Model* model, const char* dir, const char* uri, const char* label, const char* filename)
{
	auto state = lilv_state_new_from_instance(
            model->get_plugin_class(), model->get_plugin_instance(), &model->get_map(),
            model->temp_dir.c_str(), dir, dir, dir,
            get_port_value, model,
		LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE, nullptr);

	if (label)
	{
		lilv_state_set_label(state, label);
	}

	int ret = lilv_state_save(model->get_world(), &model->get_map(), &model->get_unmap(), state, uri, dir, filename);

    model->set_preset(state);

	return ret;
}

int delete_current_preset(LV2Model* model)
{
	if (!model->get_preset())
	{
		return 1;
	}

	lilv_world_unload_resource(model->get_world(), lilv_state_get_uri(model->get_preset()));
	lilv_state_delete(model->get_world(), model->get_preset());
    model->set_preset(nullptr);
	return 0;
}

}
}