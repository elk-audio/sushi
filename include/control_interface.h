/**
 * @brief Abstract interface for external control of sushi over rpc, osc or similar
 * @copyright MIND Music Labs AB, Stockholm
 */


#ifndef SUSHI_CONTROL_INTERFACE_H
#define SUSHI_CONTROL_INTERFACE_H

#include <utility>
#include <optional>
#include <vector>

namespace sushi {
namespace ext {

enum class ControlStatus
{
    OK,
    ERROR,
    UNSUPPORTED_OPERATION,
    NOT_FOUND,
    OUT_OF_RANGE,
    INVALID_ARGUMENTS
};

enum class PlayingMode
{
    STOPPED,
    PLAYING,
    RECORDING
};

enum class SyncMode
{
    INTERNAL,
    MIDI,
    GATE,
    LINK
};

struct TimeSignature
{
    int numerator;
    int denominator;
};

struct CpuTimings
{
    float avg;
    float min;
    float max;
};

enum class ParameterType
{
    BOOL,
    INT,
    FLOAT,
    STRING_PROPERTY,
    DATA_PROPERTY,
};

struct ParameterInfo
{
    int             id;
    ParameterType   type;
    std::string     label;
    std::string     name;
    std::string     unit;
    bool            automatable;
    float           min_range;
    float           max_range;
};

struct ProcessorInfo
{
    int         id;
    std::string label;
    std::string name;
    int         parameter_count;
    int         program_count;
};

struct ProgramInfo
{
    int         id;
    std::string name;
};

struct TrackInfo
{
    int         id;
    std::string label;
    std::string name;
    int         input_channels;
    int         input_busses;
    int         output_channels;
    int         output_busses;
    int         processor_count;
};

class SushiControl
{
public:
    virtual ~SushiControl() = default;

    // Main engine controls
    virtual float                               get_samplerate() const = 0;
    virtual PlayingMode                         get_playing_mode() const = 0;
    virtual void                                set_playing_mode(PlayingMode playing_mode) = 0;
    virtual SyncMode                            get_sync_mode() const = 0;
    virtual void                                set_sync_mode(SyncMode sync_mode) = 0;
    virtual float                               get_tempo() const = 0;
    virtual ControlStatus                       set_tempo(float tempo) = 0;
    virtual TimeSignature                       get_time_signature() const = 0;
    virtual ControlStatus                       set_time_signature(TimeSignature signature) = 0;
    virtual bool                                get_timing_statistics_enabled() = 0;
    virtual void                                set_timing_statistics_enabled(bool enabled) const = 0;
    virtual std::vector<TrackInfo>              get_tracks() const = 0;

    // Keyboard control
    virtual ControlStatus                       send_note_on(int track_id, int channel, int note, float velocity) = 0;
    virtual ControlStatus                       send_note_off(int track_id, int channel, int note, float velocity) = 0;
    virtual ControlStatus                       send_note_aftertouch(int track_id, int channel, int note, float value) = 0;
    virtual ControlStatus                       send_aftertouch(int track_id, int channel, float value) = 0;
    virtual ControlStatus                       send_pitch_bend(int track_id, int channel, float value) = 0;
    virtual ControlStatus                       send_modulation(int track_id, int channel, float value) = 0;

    // Cpu Timings
    virtual std::pair<ControlStatus, CpuTimings>    get_engine_timings() const = 0;
    virtual std::pair<ControlStatus, CpuTimings>    get_track_timings(int track_id) const = 0;
    virtual std::pair<ControlStatus, CpuTimings>    get_processor_timings(int processor_id) const = 0;
    virtual ControlStatus                           reset_all_timings() = 0;
    virtual ControlStatus                           reset_track_timings(int track_id) = 0;
    virtual ControlStatus                           reset_processor_timings(int processor_id) = 0;

    // Track control
    virtual std::pair<ControlStatus, int>           get_track_id(const std::string& track_name) const = 0;
    virtual std::pair<ControlStatus, TrackInfo>     get_track_info(int track_id) const = 0;
    virtual std::pair<ControlStatus, std::vector<ProcessorInfo>> get_track_processors(int track_id) const = 0;
    virtual std::pair<ControlStatus, std::vector<ParameterInfo>> get_track_parameters(int processor_id) const = 0;

    // Processor control
    virtual std::pair<ControlStatus, int>              get_processor_id(const std::string& processor_name) const = 0;
    virtual std::pair<ControlStatus, ProcessorInfo>    get_processor_info(int processor_id) const = 0;
    virtual std::pair<ControlStatus, bool>             get_processor_bypass_state(int processor_id) const = 0;
    virtual ControlStatus                              set_processor_bypass_state(int processor_id, bool bypass_enabled) = 0;
    virtual std::pair<ControlStatus, int>              get_processor_current_program(int processor_id) const = 0;
    virtual std::pair<ControlStatus, std::string>      get_processor_current_program_name(int processor_id) const = 0;
    virtual std::pair<ControlStatus, std::string>      get_processor_program_name(int processor_id, int program_id) const = 0;
    virtual std::pair<ControlStatus, std::vector<std::string>>   get_processor_programs(int processor_id) const = 0;
    virtual ControlStatus                              set_processor_program(int processor_id, int program_id)= 0;
    virtual std::pair<ControlStatus, std::vector<ParameterInfo>> get_processor_parameters(int processor_id) const = 0;

    // Parameter control
    virtual std::pair<ControlStatus, int>              get_parameter_id(int processor_id, const std::string& parameter) const = 0;
    virtual std::pair<ControlStatus, ParameterInfo>    get_parameter_info(int processor_id, int parameter_id) const = 0;
    virtual std::pair<ControlStatus, float>            get_parameter_value(int processor_id, int parameter_id) const = 0;
    virtual std::pair<ControlStatus, float>            get_parameter_value_normalised(int processor_id, int parameter_id) const = 0;
    virtual std::pair<ControlStatus, std::string>      get_parameter_value_as_string(int processor_id, int parameter_id) const = 0;
    virtual std::pair<ControlStatus, std::string>      get_string_property_value(int processor_id, int parameter_id) const = 0;
    virtual ControlStatus                              set_parameter_value(int processor_id, int parameter_id, float value) = 0;
    virtual ControlStatus                              set_parameter_value_normalised(int processor_id, int parameter_id, float value) = 0;
    virtual ControlStatus                              set_string_property_value(int processor_id, int parameter_id, const std::string& value) = 0;


protected:
    SushiControl() = default;

};




} // ext
} // sushi


#endif //SUSHI_CONTROL_INTERFACE_H
