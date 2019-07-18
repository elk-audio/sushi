/**
* Parts taken and/or adapted from:
* MrsWatson - https://github.com/teragonaudio/MrsWatson
*
* Original copyright notice with BSD license:
* Copyright (c) 2013 Teragon Audio. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
*   this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright notice,
*   this list of conditions and the following disclaimer in the documentation
*   and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
#include <cstdlib>
#include <iostream>

#include "lv2_plugin_loader.h"

#include "lv2_host_callback.h"

#include "logging.h"

namespace sushi {
namespace lv2 {

static const bool TRACE_OPTION = false;

static inline bool lv2_ansi_start(FILE *stream, int color)
{
    // TODO: What is this.
#ifdef HAVE_ISATTY
    if (isatty(fileno(stream))) {
    return fprintf(stream, "\033[0;%dm", color);
}
#endif
    return 0;
}

static inline void lv2_ansi_reset(FILE *stream)
{
#ifdef HAVE_ISATTY
    if (isatty(fileno(stream))) {
    fprintf(stream, "\033[0m");
    fflush(stream);
}
#endif
}

int lv2_vprintf(LV2_Log_Handle handle,
                LV2_URID type,
                const char *fmt,
                va_list ap)
{
    // TODO: Lock
    Jalv* jalv  = (Jalv*)handle;
    bool  fancy = true;
    if (type == jalv->urids.log_Trace && TRACE_OPTION)
    {
        lv2_ansi_start(stderr, 32);
        fprintf(stderr, "trace: ");
    }
    else if (type == jalv->urids.log_Error)
    {
        lv2_ansi_start(stderr, 31);
        fprintf(stderr, "error: ");
    }
    else if (type == jalv->urids.log_Warning)
    {
        lv2_ansi_start(stderr, 33);
        fprintf(stderr, "warning: ");
    }
    else
    {
        fancy = false;
    }

    const int st = vfprintf(stderr, fmt, ap);

    if (fancy)
    {
        lv2_ansi_reset(stderr);
    }

    return st;
}

int lv2_printf(LV2_Log_Handle handle,
               LV2_URID type,
               const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    const int ret = lv2_vprintf(handle, type, fmt, args);
    va_end(args);
    return ret;
}

// Ilias TODO: Unsure if these are global, or per plugin, yet.
void populate_nodes(JalvNodes& nodes, LilvWorld* world)
{
    /* Cache URIs for concepts we'll use */
    nodes.atom_AtomPort          = lilv_new_uri(world, LV2_ATOM__AtomPort);
    nodes.atom_Chunk             = lilv_new_uri(world, LV2_ATOM__Chunk);
    nodes.atom_Float             = lilv_new_uri(world, LV2_ATOM__Float);
    nodes.atom_Path              = lilv_new_uri(world, LV2_ATOM__Path);
    nodes.atom_Sequence          = lilv_new_uri(world, LV2_ATOM__Sequence);
    nodes.lv2_AudioPort          = lilv_new_uri(world, LV2_CORE__AudioPort);
    nodes.lv2_CVPort             = lilv_new_uri(world, LV2_CORE__CVPort);
    nodes.lv2_ControlPort        = lilv_new_uri(world, LV2_CORE__ControlPort);
    nodes.lv2_InputPort          = lilv_new_uri(world, LV2_CORE__InputPort);
    nodes.lv2_OutputPort         = lilv_new_uri(world, LV2_CORE__OutputPort);
    nodes.lv2_connectionOptional = lilv_new_uri(world, LV2_CORE__connectionOptional);
    nodes.lv2_control            = lilv_new_uri(world, LV2_CORE__control);
    nodes.lv2_default            = lilv_new_uri(world, LV2_CORE__default);
    nodes.lv2_enumeration        = lilv_new_uri(world, LV2_CORE__enumeration);
    nodes.lv2_integer            = lilv_new_uri(world, LV2_CORE__integer);
    nodes.lv2_maximum            = lilv_new_uri(world, LV2_CORE__maximum);
    nodes.lv2_minimum            = lilv_new_uri(world, LV2_CORE__minimum);
    nodes.lv2_name               = lilv_new_uri(world, LV2_CORE__name);
    nodes.lv2_reportsLatency     = lilv_new_uri(world, LV2_CORE__reportsLatency);
    nodes.lv2_sampleRate         = lilv_new_uri(world, LV2_CORE__sampleRate);
    nodes.lv2_symbol             = lilv_new_uri(world, LV2_CORE__symbol);
    nodes.lv2_toggled            = lilv_new_uri(world, LV2_CORE__toggled);
    nodes.midi_MidiEvent         = lilv_new_uri(world, LV2_MIDI__MidiEvent);
    nodes.pg_group               = lilv_new_uri(world, LV2_PORT_GROUPS__group);
    nodes.pprops_logarithmic     = lilv_new_uri(world, LV2_PORT_PROPS__logarithmic);
    nodes.pprops_notOnGUI        = lilv_new_uri(world, LV2_PORT_PROPS__notOnGUI);
    nodes.pprops_rangeSteps      = lilv_new_uri(world, LV2_PORT_PROPS__rangeSteps);
    nodes.pset_Preset            = lilv_new_uri(world, LV2_PRESETS__Preset);
    nodes.pset_bank              = lilv_new_uri(world, LV2_PRESETS__bank);
    nodes.rdfs_comment           = lilv_new_uri(world, LILV_NS_RDFS "comment");
    nodes.rdfs_label             = lilv_new_uri(world, LILV_NS_RDFS "label");
    nodes.rdfs_range             = lilv_new_uri(world, LILV_NS_RDFS "range");
    nodes.rsz_minimumSize        = lilv_new_uri(world, LV2_RESIZE_PORT__minimumSize);
    nodes.work_interface         = lilv_new_uri(world, LV2_WORKER__interface);
    nodes.work_schedule          = lilv_new_uri(world, LV2_WORKER__schedule);
    nodes.end                    = NULL;
}

MIND_GET_LOGGER_WITH_MODULE_NAME("lv2");

// Ilias TODO: Currently allocated plugin instances are not automatically freed when the _loader is destroyed. Should they be?

static LV2_URID map_uri(LV2_URID_Map_Handle handle, const char* uri)
{
    Jalv* jalv = (Jalv*)handle;
    std::unique_lock<std::mutex> lock(jalv->symap_lock); // Added by Ilias, replacing ZixSem
    const LV2_URID id = symap_map(jalv->symap, uri);

    return id;
}

static const char* unmap_uri(LV2_URID_Unmap_Handle handle, LV2_URID urid)
{
    Jalv* jalv = (Jalv*)handle;
    std::unique_lock<std::mutex> lock(jalv->symap_lock);  // Added by Ilias, replacing ZixSem
    const char* uri = symap_unmap(jalv->symap, urid);
    return uri;
}

static void init_feature(LV2_Feature* const dest, const char* const URI, void* data)
{
    dest->URI = URI;
    dest->data = data;
}

PluginLoader::PluginLoader()
{
    _jalv.world = lilv_world_new();

    // This allows loading plu-ins from their URI's, assuming they are installed in the correct paths
    // on the local machine.
    /* Find all installed plugins */
    lilv_world_load_all(_jalv.world);

    populate_nodes(_jalv.nodes, _jalv.world);

    _jalv.symap = symap_new();
    //zix_sem_init(&jalv.work_lock, 1);

    _jalv.map.handle  = &_jalv; // What is this for? I may be stupid here.
    _jalv.map.map     = map_uri;
    init_feature(&_jalv.features.map_feature, LV2_URID__map, &_jalv.map);

    //_jalv.worker.jalv       = &_jalv;
    //_jalv.state_worker.jalv = &_jalv;

    _jalv.unmap.handle  = &_jalv; // What is this for? I may be stupid here.
    _jalv.unmap.unmap   = unmap_uri;
    init_feature(&_jalv.features.unmap_feature, LV2_URID__unmap, &_jalv.unmap);

    lv2_atom_forge_init(&_jalv.forge, &_jalv.map);

    _jalv.urids.atom_Float           = symap_map(_jalv.symap, LV2_ATOM__Float);
    _jalv.urids.atom_Int             = symap_map(_jalv.symap, LV2_ATOM__Int);
    _jalv.urids.atom_Object          = symap_map(_jalv.symap, LV2_ATOM__Object);
    _jalv.urids.atom_Path            = symap_map(_jalv.symap, LV2_ATOM__Path);
    _jalv.urids.atom_String          = symap_map(_jalv.symap, LV2_ATOM__String);
    _jalv.urids.atom_eventTransfer   = symap_map(_jalv.symap, LV2_ATOM__eventTransfer);
    _jalv.urids.bufsz_maxBlockLength = symap_map(_jalv.symap, LV2_BUF_SIZE__maxBlockLength);
    _jalv.urids.bufsz_minBlockLength = symap_map(_jalv.symap, LV2_BUF_SIZE__minBlockLength);
    _jalv.urids.bufsz_sequenceSize   = symap_map(_jalv.symap, LV2_BUF_SIZE__sequenceSize);
    _jalv.urids.log_Error            = symap_map(_jalv.symap, LV2_LOG__Error);
    _jalv.urids.log_Trace            = symap_map(_jalv.symap, LV2_LOG__Trace);
    _jalv.urids.log_Warning          = symap_map(_jalv.symap, LV2_LOG__Warning);
    _jalv.urids.midi_MidiEvent       = symap_map(_jalv.symap, LV2_MIDI__MidiEvent);
    _jalv.urids.param_sampleRate     = symap_map(_jalv.symap, LV2_PARAMETERS__sampleRate);
    _jalv.urids.patch_Get            = symap_map(_jalv.symap, LV2_PATCH__Get);
    _jalv.urids.patch_Put            = symap_map(_jalv.symap, LV2_PATCH__Put);
    _jalv.urids.patch_Set            = symap_map(_jalv.symap, LV2_PATCH__Set);
    _jalv.urids.patch_body           = symap_map(_jalv.symap, LV2_PATCH__body);
    _jalv.urids.patch_property       = symap_map(_jalv.symap, LV2_PATCH__property);
    _jalv.urids.patch_value          = symap_map(_jalv.symap, LV2_PATCH__value);
    _jalv.urids.time_Position        = symap_map(_jalv.symap, LV2_TIME__Position);
    _jalv.urids.time_bar             = symap_map(_jalv.symap, LV2_TIME__bar);
    _jalv.urids.time_barBeat         = symap_map(_jalv.symap, LV2_TIME__barBeat);
    _jalv.urids.time_beatUnit        = symap_map(_jalv.symap, LV2_TIME__beatUnit);
    _jalv.urids.time_beatsPerBar     = symap_map(_jalv.symap, LV2_TIME__beatsPerBar);
    _jalv.urids.time_beatsPerMinute  = symap_map(_jalv.symap, LV2_TIME__beatsPerMinute);
    _jalv.urids.time_frame           = symap_map(_jalv.symap, LV2_TIME__frame);
    _jalv.urids.time_speed           = symap_map(_jalv.symap, LV2_TIME__speed);
    _jalv.urids.ui_updateRate        = symap_map(_jalv.symap, LV2_UI__updateRate);

    _jalv.features.llog.handle  = &_jalv;
    _jalv.features.llog.printf  = lv2_printf;
    _jalv.features.llog.vprintf = lv2_vprintf;
    init_feature(&_jalv.features.log_feature, LV2_LOG__log, &_jalv.features.llog);
}

PluginLoader::~PluginLoader()
{
//    free_nodes(_jalv.nodes);
    lilv_world_free(_jalv.world);
}

const LilvPlugin* PluginLoader::get_plugin_handle_from_URI(const std::string &plugin_URI_string)
{
    if (plugin_URI_string.empty())
    {
        MIND_LOG_ERROR("Empty library path");
        return nullptr; // Calling dlopen with an empty string returns a handle to the calling
        // program, which can cause an infinite loop.
    }

    auto plugins = lilv_world_get_all_plugins(_jalv.world);
    auto plugin_uri = lilv_new_uri(_jalv.world, plugin_URI_string.c_str());

    if (!plugin_uri)
    {
        fprintf(stderr, "Missing plugin URI, try lv2ls to list plugins\n");
        // Ilias TODO: Handle error
        return nullptr;
    }

    /* Find plugin */
    printf("Plugin:       %s\n", lilv_node_as_string(plugin_uri));
    const LilvPlugin* plugin  = lilv_plugins_get_by_uri(plugins, plugin_uri);
    lilv_node_free(plugin_uri);

    if (!plugin)
    {
        fprintf(stderr, "Failed to find plugin\n");

        // Ilias TODO: Handle error

        return nullptr;
    }

    // Ilias TODO: Introduce state_threadSafeRestore later.

    // Ilias TODO: UI code - may not need it - it goes here though.

    return plugin;
}

void PluginLoader::load_plugin(const LilvPlugin* plugin_handle, double sample_rate, const LV2_Feature** feature_list)
{
    /* Instantiate the plugin */
    _jalv.instance = lilv_plugin_instantiate(
            plugin_handle,
            sample_rate,
            feature_list);

    if (_jalv.instance == nullptr)
    {
        fprintf(stderr, "Failed to instantiate plugin.\n");
        // Ilias TODO: Handle error
    }

    // Ilias TODO: Not sure this should be here.
    // Maybe it should be called after ports are "dealt with", whatever that means.
    /* Activate plugin */
    lilv_instance_activate(_jalv.instance);
}

void PluginLoader::close_plugin_instance()
{
    // TODO Ilias: Currently, as this builds on the JALV example, only a single plugin is supported.
    // Refactor to allow multiple olugins!

    if (_jalv.instance != nullptr)
    {
        _jalv.exit = true;

        // TODO: Ilias These too are already freed elsewhere it seems?
/*        for (uint32_t i = 0; i < _jalv.num_ports; ++i) {
            if (_jalv.ports[i].evbuf) {
                lv2_evbuf_free(_jalv.ports[i].evbuf);
            }
        }
*/
        lilv_instance_deactivate(_jalv.instance);
        lilv_instance_free(_jalv.instance);

        // TODO: Ilias This gives a segfault... Understand why, is it freed elsewhere?
        //free(_jalv.ports);


        for (LilvNode** n = (LilvNode**)&_jalv.nodes; *n; ++n) {
            lilv_node_free(*n);
        }

        /*for (unsigned i = 0; i < _jalv.controls.n_controls; ++i) {
            ControlID* const control = _jalv.controls.controls[i];
            lilv_node_free(control->node);
            lilv_node_free(control->symbol);
            lilv_node_free(control->label);
            lilv_node_free(control->group);
            lilv_node_free(control->min);
            lilv_node_free(control->max);
            lilv_node_free(control->def);
            free(control);
        }
        free(jalv->controls.controls);*/

        // TODO: Ilias This gives a segfault... Understand why, is it freed elsewhere?
        //free(_jalv.feature_list);

        _jalv.instance = nullptr;
    }
}

} // namespace lv2
} // namespace sushi