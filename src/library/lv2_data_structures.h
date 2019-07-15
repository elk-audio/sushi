/**
 * @Brief Wrapper for VST 2.x plugins.
 * @copyright MIND Music Labs AB, Stockholm
 *
 */

#ifndef SUSHI_LV2_DATA_STRUCTURES_H
#define SUSHI_LV2_DATA_STRUCTURES_H

#ifdef SUSHI_BUILD_WITH_LV2

#include <map>
#include <mutex>

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

#include "processor.h"
//#include "lv2_midi_event_fifo.h"
#include "lv2_evbuf.h"
#include "symap.h"

#include "../engine/base_event_dispatcher.h"

namespace sushi {
namespace lv2 {

    // From JALV
/* Size factor for UI ring buffers.  The ring size is a few times the size of
   an event output to give the UI a chance to keep up.  Experiments with Ingen,
   which can highly saturate its event output, led me to this value.  It
   really ought to be enough for anybody(TM).
*/
#define N_BUFFER_CYCLES 16

#ifndef MAX
#    define MAX(a, b) (((a) > (b)) ? (a) : (b))
#endif

enum PortFlow {
    FLOW_UNKNOWN,
    FLOW_INPUT,
    FLOW_OUTPUT
};

enum PortType {
    TYPE_UNKNOWN,
    TYPE_CONTROL,
    TYPE_AUDIO,
    TYPE_EVENT,
    TYPE_CV
};

struct Port {
    const LilvPort* lilv_port;  ///< LV2 port
    enum PortType   type;       ///< Data type
    enum PortFlow   flow;       ///< Data flow direction

    // Seems in JALV to be to hold the Jack midi port. Won't need it.
    //void*         sys_port;   ///< For audio/MIDI ports, otherwise NULL

    LV2_Evbuf*      evbuf;      ///< For MIDI ports, otherwise NULL
    void*           widget;     ///< Control widget, if applicable
    size_t          buf_size;   ///< Custom buffer size, or 0
    uint32_t        index;      ///< Port index
    float           control;    ///< For control ports, otherwise 0.0f

    // Ilias. For ranges. Only for control ports.
    float           def{1.0f};
    float           max{1.0f};
    float           min{0.0f};
};

typedef struct {
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
} JalvURIDs;


typedef struct {
    // Do I even use these?
    LilvNode* atom_AtomPort;
    LilvNode* atom_Chunk;
    LilvNode* atom_Float;
    LilvNode* atom_Path;
    LilvNode* atom_Sequence;

    LilvNode* lv2_AudioPort;
    LilvNode* lv2_CVPort;
    LilvNode* lv2_ControlPort;
    LilvNode* lv2_InputPort;
    LilvNode* lv2_OutputPort;
    LilvNode* lv2_connectionOptional;
    LilvNode* lv2_control;
    LilvNode* lv2_default;
    LilvNode* lv2_enumeration;
    LilvNode* lv2_integer;
    LilvNode* lv2_maximum;
    LilvNode* lv2_minimum;
    LilvNode* lv2_name;
    LilvNode* lv2_reportsLatency;
    LilvNode* lv2_sampleRate;
    LilvNode* lv2_symbol;
    LilvNode* lv2_toggled;
    LilvNode* midi_MidiEvent;
    LilvNode* pg_group;
    LilvNode* pprops_logarithmic;
    LilvNode* pprops_notOnGUI;
    LilvNode* pprops_rangeSteps;
    LilvNode* pset_Preset;
    LilvNode* pset_bank;
    LilvNode* rdfs_comment;
    LilvNode* rdfs_label;
    LilvNode* rdfs_range;
    LilvNode* rsz_minimumSize;
    LilvNode* work_interface;
    LilvNode* work_schedule;
    LilvNode* end;  ///< NULL terminator for easy freeing of entire structure
} JalvNodes;


typedef struct {
    LV2_Feature                map_feature;
    LV2_Feature                unmap_feature;
    LV2_State_Make_Path        make_path;
    LV2_Feature                make_path_feature;
    LV2_Worker_Schedule        sched;
    LV2_Feature                sched_feature;
    LV2_Worker_Schedule        ssched;
    LV2_Feature                state_sched_feature;
    LV2_Log_Log                llog;
    LV2_Feature                log_feature;
    LV2_Options_Option         options[6];
    LV2_Feature                options_feature;
    LV2_Feature                safe_restore_feature;
    LV2_Extension_Data_Feature ext_data;
} JalvFeatures;

class Jalv
{
public:
    JalvURIDs          urids;          ///< URIDs
    JalvNodes          nodes;          ///< Nodes

    LV2_Atom_Forge     forge;          ///< Atom forge
    const char*        prog_name;      ///< Program name (argv[0])

    LilvWorld*         world;          ///< Lilv World

    LV2_URID_Map       map;            ///< URI => Int map
    LV2_URID_Unmap     unmap;          ///< Int => URI map

    /*SerdEnv*           env;            ///< Environment for RDF printing
    Sratom*            sratom;         ///< Atom serialiser
    Sratom*            ui_sratom;      ///< Atom serialiser for UI thread*/

    Symap*             symap;          ///< URI map
    std::mutex         symap_lock;     ///< Lock for URI map

    //JalvBackend*       backend;        ///< Audio system backend

    //ZixRing*           ui_events;      ///< Port events from UI
    //ZixRing*           plugin_events;  ///< Port events from plugin

//  void*              ui_event_buf;   ///< Buffer for reading UI port events

/*  JalvWorker         worker;         ///< Worker thread implementation
    JalvWorker         state_worker;   ///< Synchronous worker for state restore
    ZixSem             work_lock;      ///< Lock for plugin work() method
    ZixSem             done;           ///< Exit semaphore
    ZixSem             paused;         ///< Paused signal from process thread
    JalvPlayState      play_state;     ///< Current play state*/

/*
    char*              temp_dir;       ///< Temporary plugin state directory
    char*              save_dir;       ///< Plugin save directory
*/
    const LilvPlugin*  plugin;         ///< Plugin class (RDF data)
    LilvState*         preset;         ///< Current preset

/*
    LilvUIs*           uis;            ///< All plugin UIs (RDF data)LilvInstance
    const LilvUI*      ui;             ///< Plugin UI (RDF data)
    const LilvNode*    ui_type;        ///< Plugin UI type (unwrapped)
*/
    LilvInstance*      instance{nullptr};       ///< Plugin instance (shared library)

    void*              window;         ///< Window (if applicable)
    struct Port*       ports;          ///< Port array of size num_ports

// TODO: Ilias This needs re-introducing for control no?
/*  Controls           controls;       ///< Available plugin controls*/

/*  uint32_t           block_length;   ///< Audio buffer size (block length)*/
/*  size_t             midi_buf_size;  ///< Size of MIDI port buffers*/

    uint32_t           control_in;     ///< Index of control input port

    uint32_t           num_ports;      ///< Size of the two following arrays:

/*  uint32_t           plugin_latency; ///< Latency reported by plugin (if any)*/
/*  float              ui_update_hz;   ///< Frequency of UI updates*/

    float              sample_rate;    ///< Sample rate

/*  uint32_t           event_delta_t;  ///< Frames since last update sent to UI*/

//  uint32_t           position;       ///< Transport position in frames

/*  float              bpm;            ///< Transport tempo in beats per minute
    bool               rolling;        ///< Transport speed (0=stop, 1=play)
    bool               buf_size_set;   ///< True iff buffer size callback fired
    */

    bool               exit;           ///< True iff execution is finished

/*  bool               has_ui;         ///< True iff a control UI is present
    bool               request_update; ///< True iff a plugin update is needed
    bool               safe_restore;   ///< Plugin restore() is thread-safe*/

    JalvFeatures       features;
    const LV2_Feature** feature_list;
};

} // end namespace lv2
} // end namespace sushi


#endif //SUSHI_BUILD_WITH_LV2
#ifndef SUSHI_BUILD_WITH_LV2

// (...)

#endif

#endif //SUSHI_LV2_DATA_STRUCTURES_H