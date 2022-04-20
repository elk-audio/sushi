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

    void set_program(int program_id);

    void set_bypass(bool enabled);

    void add_parameter_change(ObjectId parameter_id, float value);

    void add_property_change(ObjectId property_id, const std::string& value);

    std::optional<int> program() const;

    std::optional<int> bypassed() const;

    const std::vector<std::pair<ObjectId, float>>& parameters() const;

    const std::vector<std::pair<ObjectId, std::string>>& properties() const;

protected:
    std::optional<int> _program;
    std::optional<int> _bypassed;
    std::vector<std::pair<ObjectId, float>> _parameter_changes;
    std::vector<std::pair<ObjectId, std::string>> _property_changes;
};

class RtState : public RtDeletable
{
public:
    RtState();
    RtState(const ProcessorState& state);

    ~RtState() override;

    void set_bypass(bool enabled);

    void add_parameter_change(ObjectId parameter_id, float value);

    std::optional<int> bypassed() const;

    const std::vector<std::pair<ObjectId, float>>& parameters() const;

protected:
    std::optional<int> _bypassed;
    std::vector<std::pair<ObjectId, float>> _parameter_changes;
};

} // end namespace sushi

#endif //SUSHI_PROCESSOR_STATE_H
