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

// Temporary - just to check that it finds them.
#include <lilv-0/lilv/lilv.h>

#include "lv2/resize-port/resize-port.h"
#include "lv2/midi/midi.h"
#include "lv2/log/log.h"
#include "lv2/atom/atom.h"
#include "lv2/atom/forge.h"
#include "lv2/buf-size/buf-size.h"
#include "lv2/data-access/data-access.h"
#include "lv2/options/options.h"
#include "lv2/parameters/parameters.h"
#include "lv2/patch/patch.h"
#include "lv2/port-groups/port-groups.h"
#include "lv2/port-props/port-props.h"
#include "lv2/presets/presets.h"
#include "lv2/state/state.h"
#include "lv2/time/time.h"
#include "lv2/ui/ui.h"
#include "lv2/urid/urid.h"
#include "lv2/worker/worker.h"

#include "../processor.h"
#include "../../engine/base_event_dispatcher.h"

#include "lv2_symap.h"
#include "lv2_port.h"
#include "lv2_evbuf.h"
#include "lv2_host_nodes.h"
#include "lv2_control.h"

#include "lv2_semaphore.h"

namespace sushi {
namespace lv2 {

class LV2Model;
class LV2_State;
class Lv2_Worker;
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
    LV2_Worker_Schedule sched;
    LV2_Feature sched_feature;
    LV2_Worker_Schedule ssched;
    LV2_Feature state_sched_feature;
    LV2_Log_Log llog;
    LV2_Feature log_feature;
    LV2_Options_Option options[6];
    LV2_Feature options_feature;
    LV2_Feature safe_restore_feature;
    LV2_Extension_Data_Feature ext_data;
};

/**
 * @brief LV2 depends on a "GOD" struct/class per plugin instance, which it passes around with pointers in the various
 * callbacks. LV2Model is this GOD class.
 */
class LV2Model
{
public:
    LV2Model(LilvWorld* worldIn);
    ~LV2Model();

    void initialize_host_feature_list();

    HostFeatures& get_features();
    std::vector<const LV2_Feature*>* get_feature_list();

    LilvWorld* get_world();

    LilvInstance* get_plugin_instance();
    void set_plugin_instance(LilvInstance* new_instance); // TODO: Pair this with the freeing of the old instance.

    const LilvPlugin* get_plugin_class();
    void set_plugin_class(const LilvPlugin* new_plugin); // TODO: Pair this with the freeing of the old class.

    LilvState* get_preset();
    void set_preset(LilvState* new_preset);

    int get_midi_buffer_size();
    float get_sample_rate();

    Port* get_port(int index);
    void add_port(std::unique_ptr<Port>&& port);
    int get_port_count();

    const Lv2_Host_Nodes& get_nodes();

    const LV2_URIDs& get_urids();

    LV2_URID_Map& get_map();
    LV2_URID_Unmap& get_unmap();

    LV2_URID map(const char* uri);
    const char* unmap(LV2_URID urid);

    const LV2_Atom_Forge& get_forge();

    int get_plugin_latency();
    void set_plugin_latency(int latency);

    std::mutex& get_work_lock();
    Lv2_Worker* get_worker();
    Lv2_Worker* get_state_worker();

    bool get_exit();
    void trigger_exit();

    int get_control_input_index();
    void set_control_input_index(int index);

    bool update_requested();
    void request_update();
    void clear_update_request();

    void set_restore_thread_safe(bool safe);
    bool is_restore_thread_safe();

    Semaphore done; ///< Exit semaphore
    Semaphore paused; ///< Paused signal from process thread

    bool buf_size_set{false}; ///< True iff buffer size callback fired

    PlayState play_state; ///< Current play state

    std::string temp_dir; ///< Temporary plugin state directory
    std::string save_dir; ///< Plugin save directory


    // TODO: Move to ui_io?
    bool has_ui; ///< True iff a control UI is present
    std::vector<std::unique_ptr<ControlID>> controls; ///< Available plugin controls

    void set_worker_interface(const LV2_Worker_Interface* iface);

    void process_worker_replies();

    LV2_State* get_state();

private:
    void _initialize_worker_feature();
    void _initialize_map_feature();
    void _initialize_unmap_feature();
    void _initialize_log_feature();
    void _initialize_urid_symap();

    void _initialize_safe_restore_feature();
    void _initialize_make_path_feature();

    std::unique_ptr<LV2_State> _lv2_state;

    bool _safe_restore; ///< Plugin restore() is thread-safe

    bool _request_update{false}; ///< True iff a plugin update is needed

    int _control_input_index; ///< Index of control input port

    bool _exit; ///< True iff execution is finished

    std::mutex _work_lock; ///< Lock for plugin work() method
    std::unique_ptr<Lv2_Worker> _state_worker; ///< Synchronous worker for state restore
    std::unique_ptr<Lv2_Worker> _worker; ///< Worker thread implementation

    int _plugin_latency{0}; ///< Latency reported by plugin (if any)

    LV2_Atom_Forge _forge; ///< Atom forge

    LV2_URID_Map _map; ///< URI => Int map
    LV2_URID_Unmap _unmap; ///< Int => URI map

    Symap* _symap; ///< URI map
    std::mutex _symap_lock; ///< Lock for URI map

    LV2_URIDs _urids;  ///< URIDs

    Lv2_Host_Nodes _nodes; ///< Nodes

    std::vector<std::unique_ptr<Port>> _ports; ///< Port array of size num_ports

    float _sample_rate; ///< Sample rate
    int _midi_buffer_size{4096}; ///< Size of MIDI port buffers

    const LilvPlugin* _plugin_class{nullptr};

    LilvInstance* _plugin_instance {nullptr}; ///< Plugin instance (shared library)

    LilvWorld* _world{nullptr}; ///< Lilv World

    HostFeatures _features;
    std::vector<const LV2_Feature*> _feature_list;

    //  TODO: This is a separate library. Only used once for logging, I could include that later.
    //  Sratom* sratom;         ///< Atom serialiser
    //  Sratom* ui_sratom;      ///< Atom serialiser for UI thread
    //  SerdEnv* env; ///< Environment for RDF printing

    uint32_t position; ///< Transport position in frames
    float bpm; ///< Transport tempo in beats per minute
    bool rolling; ///< Transport speed (0=stop, 1=play)
};

} // end namespace lv2
} // end namespace sushi


#endif //SUSHI_BUILD_WITH_LV2
#ifndef SUSHI_BUILD_WITH_LV2

// (...)

#endif

#endif //SUSHI_LV2_MODEL_H