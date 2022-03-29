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
 * SUSHI. If not, see http://www.gnu.org/licenses/
 */

/**
 * @Brief LV2 plugin host nodes. Internally used class for each plugin instance Nodes list.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_LV2_HOST_NODES_H
#define SUSHI_LV2_HOST_NODES_H

#ifdef SUSHI_BUILD_WITH_LV2

#include <lilv-0/lilv/lilv.h>

#include <lv2/atom/atom.h>
#include <lv2/resize-port/resize-port.h>
#include <lv2/port-props/port-props.h>
#include <lv2/port-groups/port-groups.h>
#include <lv2/presets/presets.h>
#include <lv2/state/state.h>
#include <lv2/urid/urid.h>
#include <lv2/worker/worker.h>
#include <lv2/midi/midi.h>

namespace sushi {
namespace lv2 {

class HostNodes
{
public:
    SUSHI_DECLARE_NON_COPYABLE(HostNodes);

    explicit HostNodes(LilvWorld* world)
    {
        /* Cache URIs for concepts we'll use */
        this->atom_AtomPort = lilv_new_uri(world, LV2_ATOM__AtomPort);
        this->atom_Chunk = lilv_new_uri(world, LV2_ATOM__Chunk);
        this->atom_Float = lilv_new_uri(world, LV2_ATOM__Float);
        this->atom_Path = lilv_new_uri(world, LV2_ATOM__Path);
        this->atom_Sequence = lilv_new_uri(world, LV2_ATOM__Sequence);
        this->lv2_AudioPort = lilv_new_uri(world, LV2_CORE__AudioPort);
        this->lv2_CVPort = lilv_new_uri(world, LV2_CORE__CVPort);
        this->lv2_ControlPort = lilv_new_uri(world, LV2_CORE__ControlPort);
        this->lv2_InputPort = lilv_new_uri(world, LV2_CORE__InputPort);
        this->lv2_OutputPort = lilv_new_uri(world, LV2_CORE__OutputPort);
        this->lv2_connectionOptional = lilv_new_uri(world, LV2_CORE__connectionOptional);
        this->lv2_control = lilv_new_uri(world, LV2_CORE__control);
        this->lv2_default = lilv_new_uri(world, LV2_CORE__default);
        this->lv2_enumeration = lilv_new_uri(world, LV2_CORE__enumeration);
        this->lv2_integer = lilv_new_uri(world, LV2_CORE__integer);
        this->lv2_maximum = lilv_new_uri(world, LV2_CORE__maximum);
        this->lv2_minimum = lilv_new_uri(world, LV2_CORE__minimum);
        this->lv2_name = lilv_new_uri(world, LV2_CORE__name);
        this->lv2_reportsLatency = lilv_new_uri(world, LV2_CORE__reportsLatency);
        this->lv2_sampleRate = lilv_new_uri(world, LV2_CORE__sampleRate);
        this->lv2_symbol = lilv_new_uri(world, LV2_CORE__symbol);
        this->lv2_toggled = lilv_new_uri(world, LV2_CORE__toggled);
        this->midi_MidiEvent = lilv_new_uri(world, LV2_MIDI__MidiEvent);
        this->pg_group = lilv_new_uri(world, LV2_PORT_GROUPS__group);
        this->pprops_logarithmic = lilv_new_uri(world, LV2_PORT_PROPS__logarithmic);
        this->pprops_notOnGUI = lilv_new_uri(world, LV2_PORT_PROPS__notOnGUI);
        this->pprops_rangeSteps = lilv_new_uri(world, LV2_PORT_PROPS__rangeSteps);
        this->pset_Preset = lilv_new_uri(world, LV2_PRESETS__Preset);
        this->pset_bank = lilv_new_uri(world, LV2_PRESETS__bank);
        this->rdfs_comment = lilv_new_uri(world, LILV_NS_RDFS "comment");
        this->rdfs_label = lilv_new_uri(world, LILV_NS_RDFS "label");
        this->rdfs_range = lilv_new_uri(world, LILV_NS_RDFS "range");
        this->rsz_minimumSize = lilv_new_uri(world, LV2_RESIZE_PORT__minimumSize);

        this->work_interface = lilv_new_uri(world, LV2_WORKER__interface);
        this->work_schedule = lilv_new_uri(world, LV2_WORKER__schedule);
    }

    ~HostNodes()
    {
        lilv_node_free(atom_AtomPort);
        lilv_node_free(atom_Chunk);
        lilv_node_free(atom_Float);
        lilv_node_free(atom_Path);
        lilv_node_free(atom_Sequence);

        lilv_node_free(lv2_AudioPort);
        lilv_node_free(lv2_CVPort);
        lilv_node_free(lv2_ControlPort);
        lilv_node_free(lv2_InputPort);
        lilv_node_free(lv2_OutputPort);
        lilv_node_free(lv2_connectionOptional);
        lilv_node_free(lv2_control);
        lilv_node_free(lv2_default);
        lilv_node_free(lv2_enumeration);
        lilv_node_free(lv2_integer);
        lilv_node_free(lv2_maximum);
        lilv_node_free(lv2_minimum);
        lilv_node_free(lv2_name);
        lilv_node_free(lv2_reportsLatency);
        lilv_node_free(lv2_sampleRate);
        lilv_node_free(lv2_symbol);
        lilv_node_free(lv2_toggled);
        lilv_node_free(midi_MidiEvent);
        lilv_node_free(pg_group);
        lilv_node_free(pprops_logarithmic);
        lilv_node_free(pprops_notOnGUI);
        lilv_node_free(pprops_rangeSteps);
        lilv_node_free(pset_Preset);
        lilv_node_free(pset_bank);
        lilv_node_free(rdfs_comment);
        lilv_node_free(rdfs_label);
        lilv_node_free(rdfs_range);
        lilv_node_free(rsz_minimumSize);

        lilv_node_free(work_interface);
        lilv_node_free(work_schedule);
    }

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
};

#endif //SUSHI_BUILD_WITH_LV2

} // end namespace lv2
} // end namespace sushi

#endif //SUSHI_LV2_HOST_NODES_H
