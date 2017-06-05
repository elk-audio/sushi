/**
 * @Brief Internal plugin manager.
 * @copyright MIND Music Labs AB, Stockholm
 *
 *
 */

#ifndef SUSHI_INTERNAL_PLUGIN_H
#define SUSHI_INTERNAL_PLUGIN_H

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

    /**
     * @brief Register a float typed parameter and return a pointer to a value
     *        storage object that will hold the value and set automaticaly when
     *        the processor receives parameter change events
     * @param name The unique name of the parameter
     * @param label The display name of the parameter
     * @param default_value The default value the parameter should have
     * @param custom_pre_processor A preprocessor object used to clip/scale the set value
     * @return Pointer to a FloatParameterValue object
     */
    FloatParameterValue* register_float_parameter(const std::string& name,
                                                  const std::string& label,
                                                  float default_value,
                                                  FloatParameterPreProcessor* custom_pre_processor);

    /**
     * @brief Register an int typed parameter and return a pointer to a value
     *        storage object that will hold the value and set automaticaly when
     *        the processor receives parameter change events
     * @param name The unique name of the parameter
     * @param label The display name of the parameter
     * @param default_value The default value the parameter should have
     * @param custom_pre_processor A preprocessor object used to clip/scale the set value
     * @return Pointer to an IntParameterValue object
     */
    IntParameterValue* register_int_parameter(const std::string& name,
                                              const std::string& label,
                                              int default_value,
                                              IntParameterPreProcessor* custom_pre_processor);

    /**
     * @brief Register a bool typed parameter and return a pointer to a value
     *        storage object that will hold the value and set automaticaly when
     *        the processor receives parameter change events
     * @param name The unique name of the parameter
     * @param label The display name of the parameter
     * @param default_value The default value the parameter should have
     * @return Pointer to a BoolParameterValue object
     */
    BoolParameterValue* register_bool_parameter(const std::string& name,
                                                const std::string& label,
                                                bool default_value);

    /**
     * @brief Register a string property that can be updated through events
     * @param name Unique name of the property
     * @param label Display name of the property
     * @return true if the property was registered succesfully
     */
    bool register_string_property(const std::string& name,
                                  const std::string& label);


    /**
     * @brief Register a data property that can be updated through events 
     * @param name Unique name of the property
     * @param label Display name of the property
     * @return true if the property was registered succesfully
     */
    bool register_data_property(const std::string& name,
                                const std::string& label);


    void process_event(Event event) override;

private:
    /* TODO - consider container type to use here. Deque has the very desirable property
     * that iterators are never invalidated by adding to the containers.
     * For arrays or std::vectors we need to know the maximum capacity for that to work. */
    std::deque<ParameterStorage> _parameter_values;
};

} // end namespace sushi
#endif //SUSHI_INTERNAL_PLUGIN_H
