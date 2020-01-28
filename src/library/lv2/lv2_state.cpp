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

static int populate_preset_list(LV2Model* model, const LilvNode *node, const LilvNode* title, void* data)
{
    std::string node_string = lilv_node_as_string(node);
    std::string title_string = lilv_node_as_string(title);
    //printf("%s (%s)\n", node_string.c_str(), title_string.c_str());

    model->get_state()->get_program_names().emplace_back(std::move(node_string));

    return 0;
}

LV2_State::LV2_State(LV2Model* model):
    _model(model)
{

}

LV2_State::~LV2_State()
{

}

std::vector<std::string>& LV2_State::get_program_names()
{
    return _program_names;
}

int LV2_State::get_number_of_programs()
{
    return _program_names.size();
}

int LV2_State::get_current_program_index()
{
    return _current_program_index;
}

std::string LV2_State::get_current_program_name()
{
    return program_name(_current_program_index);
}

std::string LV2_State::program_name(int program_index)
{
    if (program_index >= 0 && program_index < get_number_of_programs())
    {
        return _program_names[program_index];
    }

    return "";
}

void LV2_State::populate_program_list()
{
    _load_programs(populate_preset_list, nullptr);
}

void LV2_State::save(const char* dir)
{
	_model->save_dir = std::string(dir) + "/";

	auto state = lilv_state_new_from_instance(
		_model->get_plugin_class(), _model->get_plugin_instance(), &_model->get_map(),
		_model->temp_dir.c_str(), dir, dir, dir,
		get_port_value, _model,
		LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE, nullptr);

	lilv_state_save(_model->get_world(), &_model->get_map(), &_model->get_unmap(), state, nullptr, dir, "state.ttl");

	lilv_state_free(state);

	_model->save_dir.clear();
}

int LV2_State::_load_programs(PresetSink sink, void* data)
{
	auto presets = lilv_plugin_get_related(_model->get_plugin_class(), _model->get_nodes().pset_Preset);

	LILV_FOREACH(nodes, i, presets)
	{
		auto preset = lilv_nodes_get(presets, i);
		lilv_world_load_resource(_model->get_world(), preset);

		if (!sink)
		{
			continue;
		}

		auto labels = lilv_world_find_nodes(_model->get_world(), preset, _model->get_nodes().rdfs_label, NULL);

		if (labels)
		{
			auto label = lilv_nodes_get_first(labels);
			sink(_model, preset, label, data);
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

int LV2_State::unload_programs()
{
	auto presets = lilv_plugin_get_related(_model->get_plugin_class(), _model->get_nodes().pset_Preset);

	LILV_FOREACH(nodes, i, presets)
	{
		auto preset = lilv_nodes_get(presets, i);
		lilv_world_unload_resource(_model->get_world(), preset);
	}

	lilv_nodes_free(presets);

	return 0;
}

void LV2_State::apply_state(LilvState* state)
{
	bool must_pause = !_model->is_restore_thread_safe() && _model->play_state == PlayState::RUNNING;

	if (state)
	{
		if (must_pause)
		{
            _model->play_state = PlayState::PAUSE_REQUESTED;

// TODO: REINTRODUCE / FIX!
// I should only be pausing when process_audio is running, otherwise this freezes here.
//            model->paused.wait();
		}

        auto feature_list = _model->get_feature_list();

        lilv_state_restore(state, _model->get_plugin_instance(), set_port_value, _model, 0, feature_list->data());

		if (must_pause)
		{
            _model->request_update();
            _model->play_state = PlayState::RUNNING;
		}
	}
}

int LV2_State::apply_program(const int program_index)
{
    if (program_index < get_number_of_programs())
    {
        auto presetNode = lilv_new_uri(_model->get_world(), _program_names[program_index].c_str());
        apply_program(presetNode);
        lilv_node_free(presetNode);

        _current_program_index = program_index;

        return 0;
    }

    return -1;
}

int LV2_State::apply_program(const LilvNode* preset)
{
    set_preset(lilv_state_new_from_world(_model->get_world(), &_model->get_map(), preset));
    apply_state(_preset);
	return 0;
}

void LV2_State::set_preset(LilvState* new_preset)
{
    if (_preset != nullptr)
    {
        lilv_state_free(_preset);
    }

    _preset = new_preset;
}

int LV2_State::save_program(const char* dir, const char* uri, const char* label, const char* filename)
{
	auto state = lilv_state_new_from_instance(
            _model->get_plugin_class(), _model->get_plugin_instance(), &_model->get_map(),
            _model->temp_dir.c_str(), dir, dir, dir,
            get_port_value, _model,
		LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE, nullptr);

	if (label)
	{
		lilv_state_set_label(state, label);
	}

	int ret = lilv_state_save(_model->get_world(), &_model->get_map(), &_model->get_unmap(), state, uri, dir, filename);

    set_preset(state);

	return ret;
}

int LV2_State::delete_current_program()
{
	if (!_preset)
	{
		return 1;
	}

	lilv_world_unload_resource(_model->get_world(), lilv_state_get_uri(_preset));
	lilv_state_delete(_model->get_world(), _preset);
    set_preset(nullptr);
	return 0;
}

// This one has a signature as required by lilv.
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

}
}