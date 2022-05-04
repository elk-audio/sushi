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
 * @brief Internal plugin base class.
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_INTERNAL_PLUGIN_H
#define SUSHI_INTERNAL_PLUGIN_H

#include <deque>
#include <unordered_map>
#include <mutex>

#include "library/processor.h"
#include "library/plugin_parameters.h"
#include "library/constants.h"

namespace sushi {

constexpr int DEFAULT_CHANNELS = MAX_TRACK_CHANNELS;

class StringUid
{
public:
    virtual std::string_view uid() const
    {
        return "";
    }
};

/**
 * @brief CRTP helper to avoid having to implement both a static function and
 *        a virtual one to access the plugin string uid
 *
 *        Usage: Implement static_uid() and inherit from UidHelper<ClassName>
 */
template <typename T>
class UidHelper : public virtual StringUid
{
public:
    virtual std::string_view uid() const override
    {
        return T::static_uid();
    }
};

/**
 * @brief internal base class for processors that keeps track of all host-related
 * configuration and provides basic parameter and event handling.
 */
class InternalPlugin : public Processor, public virtual StringUid
{
public:
    SUSHI_DECLARE_NON_COPYABLE(InternalPlugin)

    explicit InternalPlugin(HostControl host_control);

    ~InternalPlugin() override = default;

    void process_event(const RtEvent& event) override;

    std::pair<ProcessorReturnCode, float> parameter_value(ObjectId parameter_id) const override;

    std::pair<ProcessorReturnCode, float> parameter_value_in_domain(ObjectId parameter_id) const override;

    std::pair<ProcessorReturnCode, std::string> parameter_value_formatted(ObjectId parameter_id) const override;

    std::pair<ProcessorReturnCode, std::string> property_value(ObjectId property_id) const override;

    ProcessorReturnCode set_property_value(ObjectId property_id, const std::string& value) override;

    ProcessorReturnCode set_state(ProcessorState* state, bool realtime_running) override;

    ProcessorState save_state() const override;

    /**
     * @brief Register a float typed parameter and return a pointer to a value
     *        storage object that will hold the value and set automatically when
     *        the processor receives parameter change events
     * @param name The unique name of the parameter
     * @param label The display name of the parameter
     * @param unit The unit of the parameters display value
     * @param default_value The default value the parameter should have
     * @param min_value The minimum value the parameter can have
     * @param max_value The maximum value the parameter can have
     * @param automatable Whether the parameter can be automated, or if not, that it then is output-only
     * @param pre_proc An optional preprocessor object used to clip/scale the set value
     * @return Pointer to a FloatParameterValue object
     */
    FloatParameterValue* register_float_parameter(const std::string& name,
                                                  const std::string& label,
                                                  const std::string& unit,
                                                  float default_value,
                                                  float min_value,
                                                  float max_value,
                                                  Direction automatable,
                                                  FloatParameterPreProcessor* pre_proc = nullptr);

    /**
     * @brief Register an int typed parameter and return a pointer to a value
     *        storage object that will hold the value and set automatically when
     *        the processor receives parameter change events
     * @param name The unique name of the parameter
     * @param label The display name of the parameter
     * @param unit The unit of the parameters display value
     * @param default_value The default value the parameter should have
     * @param min_value The minimum value the parameter can have
     * @param max_value The maximum value the parameter can have
     * @param automatable Whether the parameter can be automated, or if not, that it then is output-only
     * @param pre_proc An optional preprocessor object used to clip/scale the set value
     * @return Pointer to an IntParameterValue object
     */
    IntParameterValue* register_int_parameter(const std::string& name,
                                              const std::string& label,
                                              const std::string& unit,
                                              int default_value,
                                              int min_value,
                                              int max_value,
                                              Direction automatable,
                                              IntParameterPreProcessor* pre_proc = nullptr);

    /**
     * @brief Register a bool typed parameter and return a pointer to a value
     *        storage object that will hold the value and set automatically when
     *        the processor receives parameter change events
     * @param name The unique name of the parameter
     * @param label The display name of the parameter
     * @param unit The unit of the parameters display value
     * @param default_value The default value the parameter should have
     * @param automatable Whether the parameter can be automated, or if not, that it then is output-only
     * @return Pointer to a BoolParameterValue object
     */
    BoolParameterValue* register_bool_parameter(const std::string& name,
                                                const std::string& label,
                                                const std::string& unit,
                                                bool default_value,
                                                Direction automatable);

    /**
     * @brief Register a string property that can be updated through events. String
     *        properties will be updated in the non-rt thread. For string parameters
     *        to be received in an rt thread, call send_property_to_realtime()
     *        when received.
     * @param name Unique name of the property
     * @param label Display name of the property
     * @param default_value The default value of the property
     * @return true if the property was registered successfully
     */
    bool register_property(const std::string& name,
                           const std::string& label,
                           const std::string& default_value);

    PluginInfo info() const override;

protected:
    /**
     * @brief Update the value of a parameter and send an event notifying
     *        the host of the change.
     * @param storage The ParameterValue to update
     * @param new_value The new value to use
     */
    void set_parameter_and_notify(FloatParameterValue* storage, float new_value);

    /**
     * @brief Update the value of a parameter and send an event notifying
     *        the host of the change.
     * @param storage The ParameterValue to update
     * @param new_value The new value to use
     */
    void set_parameter_and_notify(IntParameterValue* storage, int new_value);

    /**
     * @brief Update the value of a parameter and send an event notifying
     *        the host of the change.
     * @param storage The ParameterValue to update
     * @param new_value The new value to use
     */
    void set_parameter_and_notify(BoolParameterValue* storage, bool new_value);

    /**
     * @brief Pass opaque data to the realtime part of the plugin in a thread-safe manner
     *        The data will be passed as an RtEvent with type DATA_PROPERTY_CHANGE.
     * @param data The data to pass, memory management is the responsibility of the receiver.
     * @param id An identifier that will be used to populate the property_id field_of the RtEvent.
     */
    void send_data_to_realtime(BlobData data, int id);

    /**
     * @brief Pass a string property value to the realtime part of the plugin in a thread-
     *        safe manner and with managed lifetime. The string will be passed to the rt-
     *        thread as an RtEvent with type STRING_PROPERTY_CHANGE.
     * @param property_id The id of the string property whose value is changed.
     * @param value The string value to pass. Lifetime will be handled automatically.
     */
    void send_property_to_realtime(ObjectId property_id, const std::string& value);

private:
    void _set_rt_state(const RtState* state);

    void _handle_parameter_event(const ParameterChangeRtEvent* event);

    /* TODO: Consider container type to use here. Deque has the very desirable property
     *  that iterators are never invalidated by adding to the containers.
     *  For arrays or std::vectors we need to know the maximum capacity for that to work. */
    std::deque<ParameterStorage> _parameter_values;

    mutable std::mutex _property_lock;
    std::unordered_map<ObjectId, std::string> _property_values;
};

} // end namespace sushi
#endif //SUSHI_INTERNAL_PLUGIN_H
