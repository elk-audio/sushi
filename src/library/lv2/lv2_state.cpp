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

#include "lv2_state.h"

#include <lilv-0/lilv/lilv.h>

#include "logging.h"

#include "lv2_model.h"
#include "lv2_port.h"
#include "lv2_features.h"

namespace sushi {
namespace lv2 {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("lv2");

// Callback method - signature as required by Lilv
static int populate_preset_list(Model* model, const LilvNode *node, const LilvNode* title, void* /*data*/)
{
    std::string node_string = lilv_node_as_string(node);
    std::string title_string = lilv_node_as_string(title);

    model->state()->program_names().emplace_back(std::move(node_string));

    return 0;
}

State::State(Model* model):
    _model(model) {}

std::vector<std::string>& State::program_names()
{
    return _program_names;
}

int State::number_of_programs()
{
    return _program_names.size();
}

int State::current_program_index()
{
    return _current_program_index;
}

std::string State::current_program_name()
{
    return program_name(_current_program_index);
}

std::string State::program_name(int program_index)
{
    if (program_index >= 0 && program_index < number_of_programs())
    {
        return _program_names[program_index];
    }

    return "";
}

void State::populate_program_list()
{
    _load_programs(populate_preset_list, nullptr);
}

void State::save(const char* dir)
{
    _model->set_save_dir(std::string(dir) + "/");

    auto state = lilv_state_new_from_instance(
            _model->plugin_class(), _model->plugin_instance(), &_model->get_map(),
            _model->temp_dir().c_str(), dir, dir, dir,
            get_port_value, _model,
      LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE, nullptr);

   lilv_state_save(_model->lilv_world(), &_model->get_map(), &_model->get_unmap(), state, nullptr, dir, "state.ttl");

   lilv_state_free(state);

    _model->set_save_dir(std::string(""));
}

void State::_load_programs(PresetSink sink, void* data)
{
   auto presets = lilv_plugin_get_related(_model->plugin_class(), _model->nodes()->pset_Preset);

   LILV_FOREACH(nodes, i, presets)
   {
       auto preset = lilv_nodes_get(presets, i);
       lilv_world_load_resource(_model->lilv_world(), preset);

       if (sink == nullptr)
       {
           continue;
       }

       auto labels = lilv_world_find_nodes(_model->lilv_world(), preset, _model->nodes()->rdfs_label, NULL);

       if (labels)
       {
           auto label = lilv_nodes_get_first(labels);
           sink(_model, preset, label, data);
           lilv_nodes_free(labels);
       }
       else
       {
           SUSHI_LOG_ERROR("Preset {} has no rdfs:label", lilv_node_as_string(lilv_nodes_get(presets, i)));
       }
   }

   lilv_nodes_free(presets);
}

void State::unload_programs()
{
    auto presets = lilv_plugin_get_related(_model->plugin_class(), _model->nodes()->pset_Preset);

    LILV_FOREACH(nodes, i, presets)
    {
        auto preset = lilv_nodes_get(presets, i);
        lilv_world_unload_resource(_model->lilv_world(), preset);
    }

    lilv_nodes_free(presets);
}

void State::apply_state(LilvState* state)
{
    bool must_pause = !_model->safe_restore && _model->play_state() == PlayState::RUNNING;

    if (state)
    {
        if (must_pause)
        {
            _model->set_play_state(PlayState::PAUSE_REQUESTED);
            _model->set_state_to_set(state);
        }
        else
        {
            auto feature_list = _model->host_feature_list();
            lilv_state_restore(state, _model->plugin_instance(), set_port_value, _model, 0, feature_list->data());
            _model->request_update();
        }
    }
}

bool State::apply_program(const int program_index)
{
    if (program_index < number_of_programs())
    {
        auto presetNode = lilv_new_uri(_model->lilv_world(), _program_names[program_index].c_str());
        apply_program(presetNode);
        lilv_node_free(presetNode);

        _current_program_index = program_index;

        return true;
    }

    return false;
}

void State::apply_program(const LilvNode* preset)
{
    _set_preset(lilv_state_new_from_world(_model->lilv_world(), &_model->get_map(), preset));
    apply_state(_preset);
}

void State::_set_preset(LilvState* new_preset)
{
    if (_preset != nullptr)
    {
        lilv_state_free(_preset);
    }

    _preset = new_preset;
}

int State::save_program(const char* dir, const char* uri, const char* label, const char* filename)
{
   auto state = lilv_state_new_from_instance(
            _model->plugin_class(), _model->plugin_instance(), &_model->get_map(),
            _model->temp_dir().c_str(), dir, dir, dir,
            get_port_value, _model,
      LV2_STATE_IS_POD|LV2_STATE_IS_PORTABLE, nullptr);

   if (label)
   {
       lilv_state_set_label(state, label);
   }

   int ret = lilv_state_save(_model->lilv_world(), &_model->get_map(), &_model->get_unmap(), state, uri, dir, filename);

    _set_preset(state);

   return ret;
}

bool State::delete_current_program()
{
   if (_preset == nullptr)
   {
       return false;
   }

   lilv_world_unload_resource(_model->lilv_world(), lilv_state_get_uri(_preset));
   lilv_state_delete(_model->lilv_world(), _preset);
    _set_preset(nullptr);

   return true;
}

// This one has a signature as required by Lilv.
const void* get_port_value(const char* port_symbol, void* user_data, uint32_t* size, uint32_t* type)
{
    auto model = static_cast<Model*>(user_data);
    auto port = port_by_symbol(model, port_symbol);

    if (port && port->flow() == PortFlow::FLOW_INPUT && port->type() == PortType::TYPE_CONTROL)
    {
        *size = sizeof(float);
        *type = model->forge().Float;
        return port->control_pointer();
    }

    *size = *type = 0;

    return nullptr;
}

// This one has a signature as required by Lilv.
void set_port_value(const char* port_symbol,
                    void* user_data,
                    const void* value,
// size was unused also in the JALV equivalent of this method.
// Seems unneeded, but required by callback signature.
                    uint32_t /*size*/,
                    uint32_t type)
{
    auto model = static_cast<Model*>(user_data);
    auto port = port_by_symbol(model, port_symbol);

    if (port == nullptr)
    {
        SUSHI_LOG_ERROR("error: Preset port `{}' is missing", port_symbol);
        return;
    }

    float fvalue;

    auto forge = model->forge();

    if (type == forge.Float)
    {
        fvalue = *static_cast<const float*>(value);
    }
    else if (type == forge.Double)
    {
        fvalue = *static_cast<const double*>(value);
    }
    else if (type == forge.Int)
    {
        fvalue = *static_cast<const int32_t*>(value);
    }
    else if (type == forge.Long)
    {
        fvalue = *static_cast<const int64_t*>(value);
    }
    else
    {
        SUSHI_LOG_ERROR("error: Preset {} value has bad type {}",
                port_symbol,
                model->get_unmap().unmap(model->get_unmap().handle, type));

        return;
    }

    if (model->play_state() != PlayState::RUNNING)
    {
        // Set value on port directly:
        port->set_control_value(fvalue);
    }
}

}
}
