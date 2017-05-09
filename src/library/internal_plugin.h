/**
 * @Brief Internal plugin manager.
 * @copyright MIND Music Labs AB, Stockholm
 *
 *
 */

#ifndef SUSHI_INTERNAL_PLUGIN_H
#define SUSHI_INTERNAL_PLUGIN_H

#include "library/plugin_parameters.h"
#include "library/processor.h"

#include <algorithm>
#include <map>
namespace sushi {

/* Assume that all Stompboxes handle stereo */
constexpr int STOMPBOX_MAX_CHANNELS = 2;

/**
 * @brief internal wrapper class for StompBox instances that keeps track
 * of all the host-related configuration.
 */

class InternalPlugin : public Processor
{
public:
    MIND_DECLARE_NON_COPYABLE(InternalPlugin)
    /**
     * @brief Create a new StompboxManager that takes ownership of instance
     * and will delete it when the manager is deleted.
     */
    InternalPlugin()
    {
        _max_input_channels = STOMPBOX_MAX_CHANNELS;
        _max_output_channels = STOMPBOX_MAX_CHANNELS;
        _current_input_channels = STOMPBOX_MAX_CHANNELS;
        _current_output_channels = STOMPBOX_MAX_CHANNELS;
    };

    virtual ~InternalPlugin() {};

    // Parameter registration functions

    FloatStompBoxParameter* register_float_parameter(const std::string& id,
                                                     const std::string& label,
                                                     float default_value,
                                                     FloatParameterPreProcessor* custom_pre_processor);


    IntStompBoxParameter* register_int_parameter(const std::string& id,
                                                 const std::string& label,
                                                 int default_value,
                                                 IntParameterPreProcessor* custom_pre_processor);


    BoolStompBoxParameter* register_bool_parameter(const std::string& id,
                                                   const std::string& label,
                                                   bool default_value,
                                                   BoolParameterPreProcessor* custom_pre_processor = nullptr);


    StringStompBoxParameter* register_string_parameter(const std::string& id,
                                                       const std::string& label,
                                                       const std::string& default_value);


    DataStompBoxParameter* register_data_parameter(const std::string& id,
                                                   const std::string& label,
                                                   char* default_value);

    /* Inherited from Processor */
    std::pair<ProcessorReturnCode, ObjectId> parameter_id_from_name(const std::string& parameter_name) override;

    void process_event(Event event) override;

private:
    /**
    * @brief Return the parameter with the given unique id
    */
    BaseStompBoxParameter* get_parameter(const std::string& id)
    {
        auto parameter = _parameters.find(id);
        if (parameter == _parameters.end())
        {
            return nullptr;
        }
        return parameter->second.get();
    }

    /**
     * @brief Register a newly created parameter
     * @param parameter Pointer to a parameter object
     * @return true if the parameter was successfully registered, false otherwise
     */
    bool register_parameter(BaseStompBoxParameter* parameter);

    std::map<std::string, std::unique_ptr<BaseStompBoxParameter>> _parameters;

    std::vector<BaseStompBoxParameter*> _parameters_by_index;
};

} // end namespace sushi
#endif //SUSHI_INTERNAL_PLUGIN_H
