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

#ifndef SUSHI_LV2_MODEL_H
#define SUSHI_LV2_MODEL_H

#ifdef SUSHI_BUILD_WITH_LV2

#include <exception>
#include <map>
#include <mutex>
#include <condition_variable>

#include <lv2/log/log.h>
#include <lv2/options/options.h>
#include <lv2/data-access/data-access.h>
#include <lv2/atom/forge.h>

#include "lv2_host/lv2_symap.h"

#include "library/processor.h"
#include "engine/base_event_dispatcher.h"

#include "lv2_port.h"
#include "lv2_host_nodes.h"
#include "lv2_control.h"

namespace sushi {
namespace lv2 {

class Model;
class State;
struct ControlID;

/**
Control change event, sent through ring buffers for UI updates.
*/
struct ControlChange
{
    uint32_t index;
    uint32_t protocol;
    uint32_t size;
    uint8_t body[];
};

struct LV2_URIDs
{
    LV2_URID atom_Float;
    LV2_URID atom_Int;
    LV2_URID atom_Object;
    LV2_URID atom_Path;
    LV2_URID atom_String;
    LV2_URID atom_eventTransfer;
    LV2_URID bufsz_maxBlockLength;
    LV2_URID bufsz_minBlockLength;
    LV2_URID bufsz_sequenceSize;
    LV2_URID log_Error;
    LV2_URID log_Trace;
    LV2_URID log_Warning;
    LV2_URID log_Entry;
    LV2_URID log_Note;
    LV2_URID log_log;
    LV2_URID midi_MidiEvent;
    LV2_URID param_sampleRate;
    LV2_URID patch_Get;
    LV2_URID patch_Put;
    LV2_URID patch_Set;
    LV2_URID patch_body;
    LV2_URID patch_property;
    LV2_URID patch_value;
    LV2_URID time_Position;
    LV2_URID time_bar;
    LV2_URID time_barBeat;
    LV2_URID time_beatUnit;
    LV2_URID time_beatsPerBar;
    LV2_URID time_beatsPerMinute;
    LV2_URID time_frame;
    LV2_URID time_speed;
    LV2_URID ui_updateRate;
};

enum class PlayState
{
    RUNNING,
    PAUSE_REQUESTED,
    PAUSED
};

struct HostFeatures
{
    LV2_Feature map_feature;
    LV2_Feature unmap_feature;
    LV2_State_Make_Path make_path;
    LV2_Feature make_path_feature;
    LV2_Log_Log llog;
    LV2_Feature log_feature;
    LV2_Options_Option options[6];
    LV2_Feature options_feature;

    LV2_Extension_Data_Feature ext_data;
};

/**
 * @brief LV2 depends on a "GOD" struct/class per plugin instance, which it passes around with pointers in the various
 * callbacks. LV2Model is this GOD class.
 */
class Model
{
public:
    SUSHI_DECLARE_NON_COPYABLE(Model);

    Model();
    ~Model();

    void initialize_host_feature_list();

    void load_plugin(const LilvPlugin* plugin_handle, double sample_rate, const LV2_Feature** feature_list);

    std::vector<const LV2_Feature*>* host_feature_list();

    LilvWorld* lilv_world();

    LilvInstance* plugin_instance();

    const LilvPlugin* plugin_class();
    void set_plugin_class(const LilvPlugin* new_plugin);

    int midi_buffer_size();
    float sample_rate();

    Port* get_port(int index);
    void add_port(Port port);
    int port_count();

    const HostNodes* nodes();

    const LV2_URIDs& urids();

    LV2_URID_Map& get_map();
    LV2_URID_Unmap& get_unmap();

    LV2_URID map(const char* uri);
    const char* unmap(LV2_URID urid);

    LV2_Atom_Forge& forge();

    int plugin_latency();
    void set_plugin_latency(int latency);

    void set_control_input_index(int index);

    bool update_requested();
    void request_update();
    void clear_update_request();

    State* state();

    void set_play_state(PlayState play_state);
    PlayState play_state();

    std::string temp_dir();

    std::string save_dir();
    void set_save_dir(const std::string& save_dir);

    bool buf_size_set();

    std::vector<ControlID>& controls();

    uint32_t position();
    void set_position(uint32_t position);

    float bpm();
    void set_bpm(float bpm);

    bool rolling();
    void set_rolling(bool rolling);

    LilvState* state_to_set();
    void set_state_to_set(LilvState* state_to_set);

    /** Return true iff Sushi supports the given feature. */
    bool feature_is_supported(const std::string& uri);

private:
    void _initialize_map_feature();
    void _initialize_unmap_feature();
    void _initialize_log_feature();
    void _initialize_urid_symap();

    void _initialize_make_path_feature();

    std::vector<ControlID> _controls;

    bool _buf_size_set{false};

    std::string _temp_dir;
    std::string _save_dir;

    PlayState _play_state;

    std::unique_ptr<State> _lv2_state;

    bool _request_update{false};

    int _control_input_index;

    int _plugin_latency{0};

    LV2_Atom_Forge _forge;

    LV2_URID_Map _map;
    LV2_URID_Unmap _unmap;

    lv2_host::Symap* _symap;
    std::mutex _symap_lock;

    LV2_URIDs _urids;

    LilvWorld* _world{nullptr};
    std::unique_ptr<HostNodes> _nodes{nullptr};

    std::vector<Port> _ports;

    float _sample_rate;
    int _midi_buffer_size{4096};

    const LilvPlugin* _plugin_class{nullptr};

    LilvInstance* _plugin_instance{nullptr};

    HostFeatures _features;
    std::vector<const LV2_Feature*> _feature_list;

    uint32_t _position;
    float _bpm;
    bool _rolling;

    LilvState* _state_to_set{nullptr};
};

} // end namespace lv2
} // end namespace sushi

#endif //SUSHI_BUILD_WITH_LV2

#endif //SUSHI_LV2_MODEL_H