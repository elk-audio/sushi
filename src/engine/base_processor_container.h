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
 * @brief Container interface for audio processors
 * @copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_BASE_PROCESSOR_CONTAINER_H
#define SUSHI_BASE_PROCESSOR_CONTAINER_H

#include <memory>
#include <vector>

#include "library/constants.h"
#include "library/processor.h"
#include "engine/track.h"

namespace sushi {
namespace engine {

class BaseProcessorContainer
{
public:
    virtual ~BaseProcessorContainer() = default;

    virtual bool add_processor(std::shared_ptr<Processor> track) = 0;

    virtual bool add_track(std::shared_ptr<Track> track) = 0;

    virtual bool remove_processor(ObjectId id) = 0;

    virtual bool remove_track(ObjectId track_id) = 0;

    virtual bool add_to_track(std::shared_ptr<Processor> processor, ObjectId track_id, std::optional<ObjectId> before_id) = 0;

    virtual bool remove_from_track(ObjectId processor_id, ObjectId track_id) = 0;

    virtual bool processor_exists(ObjectId id) const = 0;

    virtual bool processor_exists(const std::string& name) const = 0;

    virtual std::vector<std::shared_ptr<const Processor>> all_processors() const = 0;

    virtual std::shared_ptr<Processor> mutable_processor(ObjectId id) const = 0;

    virtual std::shared_ptr<Processor> mutable_processor(const std::string& name) const = 0;

    virtual std::shared_ptr<const Processor> processor(ObjectId) const = 0;

    virtual std::shared_ptr<const Processor> processor(const std::string& name) const = 0;

    virtual std::shared_ptr<Track> mutable_track(ObjectId track_id) const = 0;

    virtual std::shared_ptr<Track> mutable_track(const std::string& track_name) const = 0;

    virtual std::shared_ptr<const Track> track(ObjectId track_id) const = 0;

    virtual std::shared_ptr<const Track> track(const std::string& name) const = 0;

    virtual std::vector<std::shared_ptr<const Processor>> processors_on_track(ObjectId track_id) const = 0;

    virtual std::vector<std::shared_ptr<const Track>> all_tracks() const = 0;

protected:
    BaseProcessorContainer() = default;
};

} // namespace engine
} // namespace sushi
#endif //SUSHI_BASE_PROCESSOR_CONTAINER_H
