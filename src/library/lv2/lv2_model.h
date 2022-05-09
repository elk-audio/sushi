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
 * @Brief LV2 plugin host Model - initializes and holds the LV2 model for each plugin instance.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_LV2_MODEL_H
#define SUSHI_LV2_MODEL_H

#ifdef SUSHI_BUILD_WITH_LV2

#include <map>
#include <mutex>

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
class Worker;
struct ControlID;
class LV2_Wrapper;

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

    LV2_Worker_Schedule sched;
    LV2_Feature sched_feature;
    LV2_Worker_Schedule ssched;
    LV2_Feature state_sched_feature;
    LV2_Feature safe_restore_feature;

    LV2_Log_Log llog;
    LV2_Feature log_feature;
    std::array<LV2_Options_Option, 6> options;
    LV2_Feature options_feature;

    LV2_Extension_Data_Feature ext_data;
};

constexpr int FEATURE_LIST_SIZE = 11;

/**
 * @brief LV2 depends on a "GOD" struct/class per plugin instance,
 * which it passes around with pointers in the various callbacks.
 * LV2Model is this class - to the extent needed for Lilv.
 */
class Model
{
public:
    SUSHI_DECLARE_NON_COPYABLE(Model);

    Model(float sample_rate, LV2_Wrapper* wrapper, LilvWorld* world);
    ~Model();

    ProcessorReturnCode load_plugin(const LilvPlugin* plugin_handle,
                                    double sample_rate);

    // Warning: LV2 / Lilv require this list to be null-terminated.
    // So remember to check for null when iterating over it!
    std::array<const LV2_Feature*, FEATURE_LIST_SIZE>* host_feature_list();

    LilvWorld* lilv_world();

    LilvInstance* plugin_instance();

    const LilvPlugin* plugin_class();

    int midi_buffer_size() const;
    float sample_rate() const;

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

    int plugin_latency() const;
    void set_plugin_latency(int latency);

    void set_control_input_index(int index);

    bool update_requested() const;
    void request_update();
    void clear_update_request();

    State* state();

    void set_play_state(PlayState play_state);
    PlayState play_state() const;

    std::string temp_dir();

    std::string save_dir();
    void set_save_dir(const std::string& save_dir);

    bool buf_size_set() const;

    std::vector<ControlID>& controls();

    uint32_t position() const;
    void set_position(uint32_t position);

    float bpm() const;
    void set_bpm(float bpm);

    bool rolling() const;
    void set_rolling(bool rolling);

    std::pair<LilvState*, bool> state_to_set();
    void set_state_to_set(LilvState* state_to_set, bool delete_after_use);

    int input_audio_channel_count() const;
    int output_audio_channel_count() const;

    bool exit {false}; ///< True iff execution is finished

    Worker* worker();
    Worker* state_worker();

    bool safe_restore();

    LV2_Wrapper* wrapper();

private:
    bool _create_ports(const LilvPlugin* plugin);
    Port _create_port(const LilvPlugin* plugin, int port_index, float default_value);

    void _initialize_map_feature();
    void _initialize_unmap_feature();
    void _initialize_log_feature();
    void _initialize_urid_symap();

    void _initialize_make_path_feature();

    void _initialize_worker_feature();
    void _initialize_safe_restore_feature();
    void _initialize_options_feature();

    void _create_controls(bool writable);

    void _initialize_host_feature_list();

    bool _check_for_required_features(const LilvPlugin* plugin);

    /** Return true if Sushi supports the given feature. */
    bool _feature_is_supported(const std::string& uri);

    std::vector<ControlID> _controls;

    bool _buf_size_set{false};

    std::string _temp_dir;
    std::string _save_dir;

    PlayState _play_state{PlayState::PAUSED};

    std::unique_ptr<State> _lv2_state;

    bool _request_update{false};

    int _control_input_index{0};

    int _plugin_latency{0};

    LV2_Atom_Forge _forge;

    LV2_URID_Map _map;
    LV2_URID_Unmap _unmap;

    lv2_host::Symap* _symap{nullptr};
    std::mutex _symap_lock;

    LV2_URIDs _urids;

    float _sample_rate{0};

    LV2_Wrapper* _wrapper{nullptr};

    LilvWorld* _world{nullptr};
    std::unique_ptr<HostNodes> _nodes{nullptr};

    std::vector<Port> _ports;

    const int _buffer_size{AUDIO_CHUNK_SIZE};
    int _midi_buffer_size{4096};

    int _ui_update_hz{30};

    const LilvPlugin* _plugin_class{nullptr};

    LilvInstance* _plugin_instance{nullptr};

    HostFeatures _features;
    std::array<const LV2_Feature*, FEATURE_LIST_SIZE> _feature_list;

    uint32_t _position;
    float _bpm{0};
    bool _rolling{false};

    bool _delete_state_after_use{false};
    LilvState* _state_to_set{nullptr};

    int _input_audio_channel_count{0};
    int _output_audio_channel_count{0};

    bool _safe_restore{false};

    std::unique_ptr<Worker> _state_worker;
    std::unique_ptr<Worker> _worker;
};

} // end namespace lv2
} // end namespace sushi

#endif //SUSHI_BUILD_WITH_LV2

#endif //SUSHI_LV2_MODEL_H
