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
 * @Brief Wrapper for LV2 plugins - Wrapper for LV2 plugins - model.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifdef SUSHI_BUILD_WITH_LV2

#include "lv2_model.h"

#include "lv2_features.h"
#include "lv2_state.h"
#include "logging.h"

namespace {
/** These features have no data */
static const LV2_Feature static_features[] = {
        { LV2_STATE__loadDefaultState, nullptr },
        { LV2_BUF_SIZE__powerOf2BlockLength, nullptr },
        { LV2_BUF_SIZE__fixedBlockLength, nullptr },
        { LV2_BUF_SIZE__boundedBlockLength, nullptr } };

} // anonymous namespace


namespace sushi {
namespace lv2 {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("lv2");

LV2Model::LV2Model(LilvWorld* worldIn):
    _nodes(worldIn),
    _world(worldIn)
{
    // This allows loading plu-ins from their URI's, assuming they are installed in the correct paths
    // on the local machine.
    /* Find all installed plugins */
    lilv_world_load_all(_world);

    _initialize_map_feature();
    _initialize_unmap_feature();
    _initialize_urid_symap();
    _initialize_log_feature();
    _initialize_safe_restore_feature();
    _initialize_make_path_feature();

    _lv2_state = std::make_unique<LV2_State>(this);
}

LV2Model::~LV2Model()
{
    symap_free(_symap);
}

void LV2Model::initialize_host_feature_list()
{
    /* Build feature list for passing to plugins */
    std::vector<const LV2_Feature*> features({
            &_features.map_feature,
            &_features.unmap_feature,
            &_features.log_feature,
            &_features.make_path_feature,
// TODO: Re-introduce options extension.
            //&_features.options_feature,
            &static_features[0],
            &static_features[1],
            &static_features[2],
            &static_features[3],
            nullptr
    });

    _feature_list = std::move(features);
}

void LV2Model::_initialize_urid_symap()
{
    lv2_atom_forge_init(&this->_forge, &_map);

    this->_urids.atom_Float = symap_map(this->_symap, LV2_ATOM__Float);
    this->_urids.atom_Int = symap_map(this->_symap, LV2_ATOM__Int);
    this->_urids.atom_Object = symap_map(this->_symap, LV2_ATOM__Object);
    this->_urids.atom_Path = symap_map(this->_symap, LV2_ATOM__Path);
    this->_urids.atom_String = symap_map(this->_symap, LV2_ATOM__String);
    this->_urids.atom_eventTransfer = symap_map(this->_symap, LV2_ATOM__eventTransfer);
    this->_urids.bufsz_maxBlockLength = symap_map(this->_symap, LV2_BUF_SIZE__maxBlockLength);
    this->_urids.bufsz_minBlockLength = symap_map(this->_symap, LV2_BUF_SIZE__minBlockLength);
    this->_urids.bufsz_sequenceSize = symap_map(this->_symap, LV2_BUF_SIZE__sequenceSize);
    this->_urids.log_Error = symap_map(this->_symap, LV2_LOG__Error);
    this->_urids.log_Trace = symap_map(this->_symap, LV2_LOG__Trace);
    this->_urids.log_Warning = symap_map(this->_symap, LV2_LOG__Warning);
    this->_urids.log_Entry = symap_map(this->_symap, LV2_LOG__Entry);
    this->_urids.log_Note = symap_map(this->_symap, LV2_LOG__Note);
    this->_urids.log_log = symap_map(this->_symap, LV2_LOG__log);
    this->_urids.midi_MidiEvent = symap_map(this->_symap, LV2_MIDI__MidiEvent);
    this->_urids.param_sampleRate = symap_map(this->_symap, LV2_PARAMETERS__sampleRate);
    this->_urids.patch_Get = symap_map(this->_symap, LV2_PATCH__Get);
    this->_urids.patch_Put = symap_map(this->_symap, LV2_PATCH__Put);
    this->_urids.patch_Set = symap_map(this->_symap, LV2_PATCH__Set);
    this->_urids.patch_body = symap_map(this->_symap, LV2_PATCH__body);
    this->_urids.patch_property = symap_map(this->_symap, LV2_PATCH__property);
    this->_urids.patch_value = symap_map(this->_symap, LV2_PATCH__value);
    this->_urids.time_Position = symap_map(this->_symap, LV2_TIME__Position);
    this->_urids.time_bar = symap_map(this->_symap, LV2_TIME__bar);
    this->_urids.time_barBeat = symap_map(this->_symap, LV2_TIME__barBeat);
    this->_urids.time_beatUnit = symap_map(this->_symap, LV2_TIME__beatUnit);
    this->_urids.time_beatsPerBar = symap_map(this->_symap, LV2_TIME__beatsPerBar);
    this->_urids.time_beatsPerMinute = symap_map(this->_symap, LV2_TIME__beatsPerMinute);
    this->_urids.time_frame = symap_map(this->_symap, LV2_TIME__frame);
    this->_urids.time_speed = symap_map(this->_symap, LV2_TIME__speed);
    this->_urids.ui_updateRate = symap_map(this->_symap, LV2_UI__updateRate);
}

void LV2Model::_initialize_log_feature()
{
    this->_features.llog.handle = this;
    this->_features.llog.printf = lv2_printf;
    this->_features.llog.vprintf = lv2_vprintf;
    init_feature(&this->_features.log_feature, LV2_LOG__log, &this->_features.llog);
}

void LV2Model::_initialize_map_feature()
{
    this->_symap = lv2_host::symap_new();
    this->_map.handle = this;
    this->_map.map = map_uri;
    init_feature(&this->_features.map_feature, LV2_URID__map, &this->_map);
}

void LV2Model::_initialize_unmap_feature()
{
    this->_unmap.handle = this;
    this->_unmap.unmap = unmap_uri;
    init_feature(&this->_features.unmap_feature, LV2_URID__unmap, &this->_unmap);
}

LV2_State* LV2Model::state()
{
    return _lv2_state.get();
}

void LV2Model::_initialize_safe_restore_feature()
{
    init_feature(&this->_features.safe_restore_feature,
                 LV2_STATE__threadSafeRestore,
                 nullptr);
}

void LV2Model::_initialize_make_path_feature()
{
    this->_features.make_path.handle = this;
    this->_features.make_path.path = make_path;
    init_feature(&this->_features.make_path_feature,
                 LV2_STATE__makePath, &this->_features.make_path);
}

HostFeatures& LV2Model::host_features()
{
    return _features;
}

std::vector<const LV2_Feature*>* LV2Model::host_feature_list()
{
    return &_feature_list;
}

LilvWorld* LV2Model::lilv_world()
{
    return _world;
}

LilvInstance* LV2Model::plugin_instance()
{
    return _plugin_instance;
}

void LV2Model::set_plugin_instance(LilvInstance* new_instance)
{
    _plugin_instance = new_instance;
}

const LilvPlugin* LV2Model::plugin_class()
{
    return _plugin_class;
}

void LV2Model::set_plugin_class(const LilvPlugin* new_plugin)
{
    _plugin_class = new_plugin;
}

int LV2Model::midi_buffer_size()
{
    return _midi_buffer_size;
}

float LV2Model::sample_rate()
{
    return _sample_rate;
}

Port* LV2Model::get_port(int index)
{
    return &_ports[index];
}

void LV2Model::add_port(Port port)
{
    _ports.push_back(port);
}

int LV2Model::port_count()
{
    return _ports.size();
}

const Lv2_Host_Nodes& LV2Model::nodes()
{
    return _nodes;
}

const LV2_URIDs& LV2Model::urids()
{
    return _urids;
}

LV2_URID_Map& LV2Model::get_map()
{
    return _map;
}

LV2_URID_Unmap& LV2Model::get_unmap()
{
    return _unmap;
}

LV2_URID LV2Model::map(const char* uri)
{
    std::unique_lock<std::mutex> lock(_symap_lock);
    return symap_map(_symap, uri);
}

const char* LV2Model::unmap(LV2_URID urid)
{
    std::unique_lock<std::mutex> lock(_symap_lock);
    const char* uri = symap_unmap(_symap, urid);

    return uri;
}

LV2_Atom_Forge& LV2Model::forge()
{
    return _forge;
}

int LV2Model::plugin_latency()
{
    return _plugin_latency;
}

void LV2Model::set_plugin_latency(int latency)
{
    _plugin_latency = latency;
}

void LV2Model::trigger_exit()
{
    _exit = true;
}

void LV2Model::set_control_input_index(int index)
{
    _control_input_index = index;
}

bool LV2Model::update_requested()
{
    return _request_update;
}

void LV2Model::request_update()
{
    _request_update = true;
}

void LV2Model::clear_update_request()
{
    _request_update = false;
}

void LV2Model::set_restore_thread_safe(bool safe)
{
    _safe_restore = safe;
}

bool LV2Model::restore_thread_safe()
{
    return _safe_restore;
}

void LV2Model::set_play_state(PlayState play_state)
{
    _play_state = play_state;
}

PlayState LV2Model::play_state()
{
    return _play_state;
}

std::string LV2Model::temp_dir()
{
    return _temp_dir;
}

std::string LV2Model::save_dir()
{
    return _save_dir;
}

void LV2Model::set_save_dir(const std::string& save_dir)
{
    _save_dir = save_dir;
}

bool LV2Model::buf_size_set()
{
    return _buf_size_set;
}

std::vector<std::unique_ptr<ControlID>>& LV2Model::controls()
{
    return _controls;
}

uint32_t LV2Model::position()
{
    return _position;
}

void LV2Model::set_position(uint32_t position)
{
    _position = position;
}

float LV2Model::bpm()
{
    return _bpm;
}

void LV2Model::set_bpm(float bpm)
{
    _bpm = bpm;
}

bool LV2Model::rolling()
{
    return _rolling;
}

void LV2Model::set_rolling(bool rolling)
{
    _rolling = rolling;
}

}
}

#endif //SUSHI_BUILD_WITH_LV2