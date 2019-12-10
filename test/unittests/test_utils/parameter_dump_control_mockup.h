#ifndef SUSHI_PARAMETER_DUMP_CONTROL_MOCKUP_H
#define SUSHI_PARAMETER_DUMP_CONTROL_MOCKUP_H

#include "control_interface.h"

namespace sushi {
namespace ext {

const ParameterInfo parameter1{0,ParameterType::INT,"param 1", "param 1", "unit", 0, 0};
const ParameterInfo parameter2{1,ParameterType::FLOAT,"param 2", "param 2", "unit", 1, 1};
const ParameterInfo parameter3{2,ParameterType::BOOL,"param 3", "param 3", "unit", -1, -1};
const std::vector<ParameterInfo> parameters{parameter1, parameter2, parameter3};

const ProcessorInfo processor1{0, "proc 1", "proc 1", 0 ,0};
const ProcessorInfo processor2{1, "proc 2", "proc 2", 1 ,1};
const std::vector<ProcessorInfo> processors{processor1, processor2};

const TrackInfo track1{0,"track1","track1",0,0,0,0,0};
const TrackInfo track2{1,"track2","track2",1,1,1,1,1};
const std::vector<TrackInfo> tracks{track1, track2};

class ControlMockup : public sushi::ext::SushiControl
{
public:

     // Main engine controls
    virtual float                               get_samplerate() const override {};
    virtual PlayingMode                         get_playing_mode() const override {};
    virtual void                                set_playing_mode(PlayingMode playing_mode) override {};
    virtual SyncMode                            get_sync_mode() const override {};
    virtual void                                set_sync_mode(SyncMode sync_mode) override {};
    virtual float                               get_tempo() const override {};
    virtual ControlStatus                       set_tempo(float tempo) override {};
    virtual TimeSignature                       get_time_signature() const override {};
    virtual ControlStatus                       set_time_signature(TimeSignature signature) override {};
    virtual bool                                get_timing_statistics_enabled() override {};
    virtual void                                set_timing_statistics_enabled(bool enabled) const override {};
    virtual std::vector<TrackInfo>              get_tracks() const override 
    {
        return tracks;
    };

    // Keyboard control
    virtual ControlStatus                       send_note_on(int track_id, int channel, int note, float velocity) override {};
    virtual ControlStatus                       send_note_off(int track_id, int channel, int note, float velocity) override {};
    virtual ControlStatus                       send_note_aftertouch(int track_id, int channel, int note, float value) override {};
    virtual ControlStatus                       send_aftertouch(int track_id, int channel, float value) override {};
    virtual ControlStatus                       send_pitch_bend(int track_id, int channel, float value) override {};
    virtual ControlStatus                       send_modulation(int track_id, int channel, float value) override {};

    // Cpu Timings
    virtual std::pair<ControlStatus, CpuTimings>    get_engine_timings() const override {};
    virtual std::pair<ControlStatus, CpuTimings>    get_track_timings(int track_id) const override {};
    virtual std::pair<ControlStatus, CpuTimings>    get_processor_timings(int processor_id) const override {};
    virtual ControlStatus                           reset_all_timings() override {};
    virtual ControlStatus                           reset_track_timings(int track_id) override {};
    virtual ControlStatus                           reset_processor_timings(int processor_id) override {};

    // Track control
    virtual std::pair<ControlStatus, int>           get_track_id(const std::string& track_name) const override {};
    virtual std::pair<ControlStatus, TrackInfo>     get_track_info(int track_id) const override {};
    virtual std::pair<ControlStatus, std::vector<ProcessorInfo>> get_track_processors(int track_id) const override 
    {
        return std::pair<ControlStatus, std::vector<ProcessorInfo>>(ControlStatus::OK, processors);
    };
    virtual std::pair<ControlStatus, std::vector<ParameterInfo>> get_track_parameters(int processor_id) const override {};

    // Processor control
    virtual std::pair<ControlStatus, int>              get_processor_id(const std::string& processor_name) const override {};
    virtual std::pair<ControlStatus, ProcessorInfo>    get_processor_info(int processor_id) const override {};
    virtual std::pair<ControlStatus, bool>             get_processor_bypass_state(int processor_id) const override {};
    virtual ControlStatus                              set_processor_bypass_state(int processor_id, bool bypass_enabled) override {};
    virtual std::pair<ControlStatus, int>              get_processor_current_program(int processor_id) const override {};
    virtual std::pair<ControlStatus, std::string>      get_processor_current_program_name(int processor_id) const override {};
    virtual std::pair<ControlStatus, std::string>      get_processor_program_name(int processor_id, int program_id) const override {};
    virtual std::pair<ControlStatus, std::vector<std::string>>   get_processor_programs(int processor_id) const override {};
    virtual ControlStatus                              set_processor_program(int processor_id, int program_id)override {};
    virtual std::pair<ControlStatus, std::vector<ParameterInfo>> get_processor_parameters(int processor_id) const override 
    {
        return std::pair<ControlStatus, std::vector<ParameterInfo>>(ControlStatus::OK, parameters);
    };

    // Parameter control
    virtual std::pair<ControlStatus, int>              get_parameter_id(int processor_id, const std::string& parameter) const override {};
    virtual std::pair<ControlStatus, ParameterInfo>    get_parameter_info(int processor_id, int parameter_id) const override {};
    virtual std::pair<ControlStatus, float>            get_parameter_value(int processor_id, int parameter_id) const override {};
    virtual std::pair<ControlStatus, float>            get_parameter_value_normalised(int processor_id, int parameter_id) const override {};
    virtual std::pair<ControlStatus, std::string>      get_parameter_value_as_string(int processor_id, int parameter_id) const override {};
    virtual std::pair<ControlStatus, std::string>      get_string_property_value(int processor_id, int parameter_id) const override {};
    virtual ControlStatus                              set_parameter_value(int processor_id, int parameter_id, float value) override {};
    virtual ControlStatus                              set_parameter_value_normalised(int processor_id, int parameter_id, float value) override {};
    virtual ControlStatus                              set_string_property_value(int processor_id, int parameter_id, const std::string& value) override {};
};

} // ext
} // sushi

#endif //SUSHI_PARAMETER_DUMP_CONTROL_MOCKUP_H
