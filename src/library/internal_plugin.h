/**
 * @Brief Internal plugin manager.
 * @copyright MIND Music Labs AB, Stockholm
 *
 *
 */

#ifndef SUSHI_INTERNAL_PLUGIN_H
#define SUSHI_INTERNAL_PLUGIN_H

#include <algorithm>
#include <deque>

#include "library/processor.h"
#include "library/plugin_parameters.h"

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

    InternalPlugin()
    {
        _max_input_channels = STOMPBOX_MAX_CHANNELS;
        _max_output_channels = STOMPBOX_MAX_CHANNELS;
        _current_input_channels = STOMPBOX_MAX_CHANNELS;
        _current_output_channels = STOMPBOX_MAX_CHANNELS;
    };

    virtual ~InternalPlugin() {};

    // Parameter registration functions
    FloatParameterValue* register_float_parameter(const std::string& name,
                                                  const std::string& label,
                                                  float default_value,
                                                  FloatParameterPreProcessor* custom_pre_processor);


    IntParameterValue* register_int_parameter(const std::string& name,
                                              const std::string& label,
                                              int default_value,
                                              IntParameterPreProcessor* custom_pre_processor);


    BoolParameterValue* register_bool_parameter(const std::string& name,
                                                const std::string& label,
                                                bool default_value,
                                                BoolParameterPreProcessor* custom_pre_processor = nullptr);

    /* Currently properties dont provide a storage class and automatic updates */
    bool register_string_property(const std::string& name,
                                  const std::string& label);


    /* Currently properties dont provide a storage class and automatic updates */
    bool register_data_property(const std::string& name,
                                const std::string& label);

    /* Inherited from Processor */
    std::pair<ProcessorReturnCode, ObjectId> parameter_id_from_name(const std::string& parameter_name) override;

    void process_event(Event event) override;

private:
    /**
    * @brief Return the parameter with the given unique id
    */
    ParameterDescriptor* get_parameter(const std::string& id)
    {
        auto parameter = _parameters.find(id);
        if (parameter == _parameters.end())
        {
            return nullptr;
        }
        return parameter->second.get();
    }
    /* TODO - consider container type to use here. Deque has the very desirable property
     * that iterators are never invalidated by adding to the containers.
     * For arrays or std::vectors we need to know the maximum capacity for that to work. */
    std::deque<ParameterStorage> _parameter_values;
};

} // end namespace sushi
#endif //SUSHI_INTERNAL_PLUGIN_H
