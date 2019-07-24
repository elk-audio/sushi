/**
 * @Brief Wrapper for LV2 plugins - models.
 * @copyright MIND Music Labs AB, Stockholm
 *
 */

#ifdef SUSHI_BUILD_WITH_LV2

#include "lv2_model.h"

#include "lv2_features.h"

namespace sushi {
    namespace lv2 {

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
    this->features.llog.handle = this;
    this->features.llog.printf = lv2_printf;
    this->features.llog.vprintf = lv2_vprintf;
    init_feature(&this->features.log_feature, LV2_LOG__log, &this->features.llog);
}

void LV2Model::_initialize_map_feature()
{
    this->symap = symap_new();
    this->map.handle = this;
    this->map.map = map_uri;
    init_feature(&this->features.map_feature, LV2_URID__map, &this->map);
}

void LV2Model::_initialize_unmap_feature()
{
    this->unmap.handle = this;
    this->unmap.unmap = unmap_uri;
    init_feature(&this->features.unmap_feature, LV2_URID__unmap, &this->unmap);
}

}
}

#endif //SUSHI_BUILD_WITH_LV2