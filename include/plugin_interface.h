/**
 * @brief The public interface of an audio plugin class
 * @copyright MIND Music Labs AB, Stockholm
 *
 */

#ifndef PLUGIN_INTERFACE_H
#define PLUGIN_INTERFACE_H

#include <string>

#include "library/sample_buffer.h"
#include "library/plugin_parameters.h"
#include "library/plugin_events.h"

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
    virtual FloatStompBoxParameter* register_float_parameter(const std::string& id,
                                                             const std::string& label,
                                                             float default_value,
                                                             FloatParameterPreProcessor* custom_pre_processor = nullptr) = 0;

    virtual IntStompBoxParameter* register_int_parameter(const std::string& id,
                                                         const std::string& label,
                                                         int default_value,
                                                         IntParameterPreProcessor* custom_pre_processor = nullptr) = 0;

    virtual BoolStompBoxParameter* register_bool_parameter(const std::string& id,
                                                           const std::string& label,
                                                           bool default_value,
                                                           BoolParameterPreProcessor* custom_pre_processor = nullptr) = 0;

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

    /**
     * @brief (Re)initialize the plugin instance. This is called from the host's non-
     *         realtime environment at startup and configuration changes.
     *         Not caled at the same time as process(). When returning OK, the plugin
     *         should e in a default state, i.e. filter registers, reverb tails, etc should
     *         be zerod
     * @param configuration Host configuration object
     *
     * @return error code, checked by the host
     */
    virtual StompBoxStatus init(const StompBoxConfig &configuration) = 0;

    // TODO: in the future this and other properties could be defined in e.g. a text file inside a plugin bundle directory,
    //       so the host can avoid to instantiate the plugins just to access these properties
    /**
     * @brief The host calls this function to retrieve a unique identifier (as string) for the given plugin.
     * @return Uuid string
     */
    virtual std::string unique_id() const = 0;

    /**
     * @brief Called by the host from the real time processing environment once for
     * every chunk. in_buffer and out_buffer are  AUDIO_CHUNK_SIZE long arrays
     * of audio data. The function should handle the case when in_buffer and
     * out_buffer points to the same memory location, for replacement processing.
     *
     * @param in_buffer Pointer to input samples buffer
     * @param out_buffer Pointer to output samples buffer, which is not necessarily pre-zeroed
     */
    virtual void process(const SampleBuffer<AUDIO_CHUNK_SIZE>* in_buffer, SampleBuffer<AUDIO_CHUNK_SIZE>* out_buffer) = 0;
};

} //namespace sushi

#endif // PLUGIN_INTERFACE_H
