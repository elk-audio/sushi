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
#include "lv2_worker.h"
#include "lv2_state.h"
#include "logging.h"

namespace {
// TODO: verify that these LV2 features work as intended:
/** These features have no data */
static const LV2_Feature static_features[] = {
        { LV2_STATE__loadDefaultState, NULL },
        { LV2_BUF_SIZE__powerOf2BlockLength, NULL },
        { LV2_BUF_SIZE__fixedBlockLength, NULL },
        { LV2_BUF_SIZE__boundedBlockLength, NULL } };

} // anonymous namespace


namespace sushi {
namespace lv2 {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("lv2");

void LV2Model::initialize_host_feature_list()
{
    /* Build feature list for passing to plugins */
    std::vector<const LV2_Feature*> features({
            &_features.map_feature,
            &_features.unmap_feature,
            &_features.log_feature,
            &_features.sched_feature,
            &_features.make_path_feature,
// TODO: Re-introduce options extension!
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
    lv2_atom_forge_init(&this->forge, &this->map);

    this->urids.atom_Float = symap_map(this->symap, LV2_ATOM__Float);
    this->urids.atom_Int = symap_map(this->symap, LV2_ATOM__Int);
    this->urids.atom_Object = symap_map(this->symap, LV2_ATOM__Object);
    this->urids.atom_Path = symap_map(this->symap, LV2_ATOM__Path);
    this->urids.atom_String = symap_map(this->symap, LV2_ATOM__String);
    this->urids.atom_eventTransfer = symap_map(this->symap, LV2_ATOM__eventTransfer);
    this->urids.bufsz_maxBlockLength = symap_map(this->symap, LV2_BUF_SIZE__maxBlockLength);
    this->urids.bufsz_minBlockLength = symap_map(this->symap, LV2_BUF_SIZE__minBlockLength);
    this->urids.bufsz_sequenceSize = symap_map(this->symap, LV2_BUF_SIZE__sequenceSize);
    this->urids.log_Error = symap_map(this->symap, LV2_LOG__Error);
    this->urids.log_Trace = symap_map(this->symap, LV2_LOG__Trace);
    this->urids.log_Warning = symap_map(this->symap, LV2_LOG__Warning);
    this->urids.log_Entry = symap_map(this->symap, LV2_LOG__Entry);
    this->urids.log_Note = symap_map(this->symap, LV2_LOG__Note);
    this->urids.log_log = symap_map(this->symap, LV2_LOG__log);
    this->urids.midi_MidiEvent = symap_map(this->symap, LV2_MIDI__MidiEvent);
    this->urids.param_sampleRate = symap_map(this->symap, LV2_PARAMETERS__sampleRate);
    this->urids.patch_Get = symap_map(this->symap, LV2_PATCH__Get);
    this->urids.patch_Put = symap_map(this->symap, LV2_PATCH__Put);
    this->urids.patch_Set = symap_map(this->symap, LV2_PATCH__Set);
    this->urids.patch_body = symap_map(this->symap, LV2_PATCH__body);
    this->urids.patch_property = symap_map(this->symap, LV2_PATCH__property);
    this->urids.patch_value = symap_map(this->symap, LV2_PATCH__value);
    this->urids.time_Position = symap_map(this->symap, LV2_TIME__Position);
    this->urids.time_bar = symap_map(this->symap, LV2_TIME__bar);
    this->urids.time_barBeat = symap_map(this->symap, LV2_TIME__barBeat);
    this->urids.time_beatUnit = symap_map(this->symap, LV2_TIME__beatUnit);
    this->urids.time_beatsPerBar = symap_map(this->symap, LV2_TIME__beatsPerBar);
    this->urids.time_beatsPerMinute = symap_map(this->symap, LV2_TIME__beatsPerMinute);
    this->urids.time_frame = symap_map(this->symap, LV2_TIME__frame);
    this->urids.time_speed = symap_map(this->symap, LV2_TIME__speed);
    this->urids.ui_updateRate = symap_map(this->symap, LV2_UI__updateRate);
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
    this->symap = symap_new();
    this->map.handle = this;
    this->map.map = map_uri;
    init_feature(&this->_features.map_feature, LV2_URID__map, &this->map);
}

void LV2Model::_initialize_unmap_feature()
{
    this->unmap.handle = this;
    this->unmap.unmap = unmap_uri;
    init_feature(&this->_features.unmap_feature, LV2_URID__unmap, &this->unmap);
}

void LV2Model::_initialize_worker_feature()
{
    this->worker.model = this;
    this->state_worker.model = this;

    this->_features.sched.handle = &this->worker;
    this->_features.sched.schedule_work = lv2_worker_schedule;
    init_feature(&this->_features.sched_feature,
                 LV2_WORKER__schedule, &this->_features.sched);

    this->_features.ssched.handle = &this->state_worker;
    this->_features.ssched.schedule_work = lv2_worker_schedule;
    init_feature(&this->_features.state_sched_feature,
                 LV2_WORKER__schedule, &this->_features.ssched);
}

void LV2Model::_initialize_safe_restore_feature()
{
    init_feature(&this->_features.safe_restore_feature,
                 LV2_STATE__threadSafeRestore,
                 NULL);
}

void LV2Model::_initialize_make_path_feature()
{
    this->_features.make_path.handle = this;
    this->_features.make_path.path = make_path;
    init_feature(&this->_features.make_path_feature,
                 LV2_STATE__makePath, &this->_features.make_path);
}

}
}

#endif //SUSHI_BUILD_WITH_LV2