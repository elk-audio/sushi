/*
 * Copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Container class for the full state of a processor.
 * @copyright 2017-2021 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_PROCESSOR_STATE_H
#define SUSHI_PROCESSOR_STATE_H

#include <optional>
#include <vector>

#include "library/constants.h"
#include "library/types.h"
#include "library/id_generator.h"
#include "engine/host_control.h"

namespace sushi {

class ProcessorState
{
public:
    virtual ~ProcessorState();


    /**
     * @brief  Check if the state contains opaque binary data from a plugin
     *         If false, this means the state is described fully by the parameter and
     *         property values. If true then the parameter and property values of this
     *         object are empty and the values stored in the binary data.
     * @return True if the instance contains binary data.
     */
    bool has_binary_data() const;

    /**
     * @brief Set the program id of the state.
     * @param program_id The program id to store
     */
    void set_program(int program_id);

    /**
     * @brief Set the bypass state.
     * @param program_id The bypass state id to store
     */
    void set_bypass(bool enabled);

    /**
     * @brief Store a parameter value in the state
     * @param parameter_id The id of the parameter
     * @param value The normalized float value
     */
    void add_parameter_change(ObjectId parameter_id, float value);

    /**
     * @brief Store a property value in the state
     * @param propery_id The id of the property
     * @param value The string value of the property
     */
    void add_property_change(ObjectId property_id, const std::string& value);

    /**
     * @brief Store binary data in the state
     * @param data A vector of binary data to be copied into the state
     */
    void set_binary_data(const std::vector<std::byte>& data);

    /**
     * @brief Store binary data in the state
     * @param data A vector of binary data to be moved into the state
     */
    void set_binary_data(std::vector<std::byte>&& data);

    /**
     * @brief Return the stored program id, if any.
     * @return An std::optional<int> populated with the program id, if stored.
     */
    std::optional<int> program() const;

    /**
     * @brief Return the stored bypass state, if any.
     * @return An std::optional<bool> populated with the bypass state, if stored.
     */
    std::optional<bool> bypassed() const;

    /**
     * @brief Return stored parameter values.
     * @return A vector of id and value pairs
     */
    const std::vector<std::pair<ObjectId, float>>& parameters() const;

    /**
     * @brief Return stored property values.
     * @return A vector of id and value pairs
     */
    const std::vector<std::pair<ObjectId, std::string>>& properties() const;

    /**
     * @brief Return stored binary data.
     * @return A vector byte data for the plugin to interpret
     */
    const std::vector<std::byte>& binary_data() const;

    std::vector<std::byte>& binary_data();

protected:
    std::optional<int> _program;
    std::optional<bool> _bypassed;
    std::vector<std::pair<ObjectId, float>> _parameter_changes;
    std::vector<std::pair<ObjectId, std::string>> _property_changes;
    std::vector<std::byte> _binary_data;
};

class RtState : public RtDeletable
{
public:
    RtState();
    RtState(const ProcessorState& state);

    ~RtState() override;

    /**
     * @brief Set the bypass state.
     * @param program_id The bypass state id to store
     */
    void set_bypass(bool enabled);

    /**
     * @brief Store a parameter value in the state
     * @param parameter_id The id of the parameter
     * @param value The normalized float value
     */
    void add_parameter_change(ObjectId parameter_id, float value);

    /**
     * @brief Return the stored bypass state, if any.
     * @return An std::optional<int> populated with the bypass state, if stored.
     */
    std::optional<int> bypassed() const;

    /**
     * @brief Return stored parameter values.
     * @return A vector of id and value pairs
     */
    const std::vector<std::pair<ObjectId, float>>& parameters() const;

protected:
    std::optional<int> _bypassed;
    std::vector<std::pair<ObjectId, float>> _parameter_changes;
};

} // end namespace sushi

#endif //SUSHI_PROCESSOR_STATE_H
