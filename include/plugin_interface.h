/**
 * @brief The public interface of an audio plugin class
 * @copyright MIND Music Labs AB, Stockholm
 *
 */

#ifndef PLUGIN_INTERFACE_H
#define PLUGIN_INTERFACE_H

#include "library/sample_buffer.h"
#include "library/plugin_parameters.h"

namespace sushi {

/* Return Status Enum */
enum class StompBoxStatus
{
    OK,
    ERROR,
    PARAMETER_ERROR,
    MEMORY_ERROR,
};

/**
 * @brief Controller object that gives the plugin an entry point to
 * call host functions like registering parameters.
 * Should not be accessed during calls to process()!?
 */
class StompBoxController
{
public:
    /**
     * @brief registers and returns a StompBoxParameter that will be
     * managed by the host.
     * If no preprocessor is supplied a standard max-min clip preprocessor
     * will be contructed and attached to the parameter
     */
    virtual FloatStompBoxParameter* register_float_parameter(std::string label,
                                                             std::string id,
                                                            float default_value = 0,
                                                            float max_value,
                                                            float min,
                                                            FloatParameterPreProcessor* customPreProcessor = nullptr) = 0;

    virtual IntStompBoxParameter* register_int_parameter(std::string label,
                                                         std::string id,
                                                         int default_value,
                                                         int max_value ,
                                                         int min,
                                                         IntParameterPreProcessor* customPreProcessor = nullptr) = 0;

    virtual BoolStompBoxParameter* register_bool_parameter(std::string label,
                                                           std::string id,
                                                           bool default_value,
                                                           BoolParameterPreProcessor* customPreProcessor = nullptr) = 0;

};

struct StompBoxConfig
{
    StompBoxController* controller;
    int sample_rate;
};

class StompBox
{
public:

    virtual ~StompBox()
    {};

/* (Re)initialize the plugin instance. This is called from the host's non-
realtime environment at startup and configuration changes. 
Not called at the same time as process(). When returning OK, the plugin 
should be in a default state, i.e. filter registers, reverb tails, etc should 
be zeroed */
    virtual StompBoxStatus init(const StompBoxConfig &configuration) = 0;

/* Optional, makes for the second most minimal interface if included.
Called before process() if called from the realtime environment.
More can be added for setting other types of parameters (bool, int..) */
    virtual void set_parameter(int parameter_id, float value) = 0;

/* Called by the host from the real time processing environment once for
every chunk. in_buffer and out_buffer are  AUDIO_CHUNK_SIZE long arrays 
of audio data. The function should handle the case when in_buffer and 
out_buffer points to the same memory location, for replacement processing.*/
    virtual void process(const SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer) = 0;
};

} //namespace sushi

#endif // PLUGIN_INTERFACE_H