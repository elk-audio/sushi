/*
 * Copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk
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
 * @brief Container for audio processors
 * @copyright 2017-2020 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_PROCESSOR_CONTAINER_H
#define SUSHI_PROCESSOR_CONTAINER_H

#include <memory>
#include <unordered_map>
#include <vector>
#include <utility>
#include <mutex>

#include "base_processor_container.h"
#include "track.h"
#include "library/processor.h"

namespace sushi {
namespace engine {

class ProcessorContainer : public BaseProcessorContainer
{
public:
    ProcessorContainer() = default;
    SUSHI_DECLARE_NON_COPYABLE(ProcessorContainer);

    /**
     * @brief Add a processor to the container
     * @param processor The Processor instance to add
     * @return true if the track was successfully added, false otherwise.
     */
    bool add_processor(std::shared_ptr<Processor> processor) override;

    /**
     * @brief Add a track to the container
     * @param track The Track instance to add
     * @return true if the track was successfully added, false otherwise.
     */
    bool add_track(std::shared_ptr<Track> track) override;

    /**
     * @brief Remove a processor from the container
     * @param id The id of the processor
     * @return true if the processor was found and successfully removed,
     *         false otherwise
     */
    bool remove_processor(ObjectId id) override;

    /**
     * @brief Remove a track
     * @param track_id The id of the track
     * @return true of the track was found and successfully removed,
     *         false otherwise
     */
    bool remove_track(ObjectId track_id) override;

    /**
     * @brief Add a processor from a track entry
     * @param processor The processor instance to add.
     * @param track_id The id of the track to add to
     * @param before_id If populated, the processor will be added before the
     *        processor with the give id, otherwise the processor will be
     *        added to the back of the list of processors
     * @return
     */
    bool add_to_track(std::shared_ptr<Processor> processor, ObjectId track_id, std::optional<ObjectId> before_id) override;

    /**
     * @brief Remove a processor from a track entry
     * @param processor_id The id of the processor to remove.
     * @param track_id  The id of the track to remove from
     * @return true if the processor was successfullt removed from the track,
     *         false otherwise
     */
    bool remove_from_track(ObjectId processor_id, ObjectId track_id) override;

    /**
    * @brief Query whether a particular processor exists in the container
    * @param name The unique id of the processor
    * @return true if the processor exists, false otherwise
    */
    bool processor_exists(ObjectId id) const override;

    /**
     * @brief Query whether a particular processor exists in the container
     * @param name The unique name of the processor
     * @return true if the processor exists, false otherwise
     */
    bool processor_exists(const std::string& name) const override;

    /**
     * @brief Return all processors.
     * @return An std::vector containing all registered processors.
     */
    std::vector<std::shared_ptr<const Processor>> all_processors() const override;

    /**
     * @brief Access a particular processor by its unique id for editing,
     *        use with care and not from several threads at once
     * @param processor_id The id of the processor
     * @return A mutable pointer to the processor instance if found, nullptr otherwise
     */
    std::shared_ptr<Processor> mutable_processor(ObjectId id) const override;

    /**
     * @brief Access a particular processor by its unique name for editing
     * @param processor_name The name of the processor
     * @return A std::shared_ptr<Processor> to the processor instance if found,
     *         nullptr otherwise
     */
    std::shared_ptr<Processor> mutable_processor(const std::string& name) const override;

    /**
     * @brief Access a particular processor by its unique id for querying
     * @param processor_id The id of the processor
     * @return A std::shared_ptr<const Processor> to the processor instance if found,
     *         nullptr otherwise
     */
    std::shared_ptr<const Processor> processor(ObjectId) const override;

    /**
     * @brief Access a particular processor by its unique name for querying
     * @param processor_name The name of the processor
     * @return A std::shared_ptr<const Processor> to the processor instance if found,
     *         nullptr otherwise
     */
    std::shared_ptr<const Processor> processor(const std::string& name) const override;

    /**
     * @brief Access a particular track by its unique id for editing
     * @param track_id The id of the track
     * @return A std::shared_ptr<Track> to the track instance if found,
     *         nullptr otherwise
     */
    std::shared_ptr<Track> mutable_track(ObjectId track_id) const override;

    /**
     * @brief Access a particular track by its unique name for editing
     * @param track_name The name of the track
     * @return A std::shared_ptr<Track> to the track instance if found,
     *         nullptr otherwise
     */
    std::shared_ptr<Track> mutable_track(const std::string& track_name) const override;

    /**
     * @brief Access a particular track by its unique id for querying
     * @param track_id The id of the track
     * @return A std::shared_ptr<const Track> to the track instance if found,
     *         nullptr otherwise
     */
    std::shared_ptr<const Track> track(ObjectId track_id) const override;

    /**
     * @brief Access a particular track by its unique name for querying
     * @param track_name The name of the track
     * @return A std::shared_ptr<const Track> to the track instance if found,
     *         nullptr otherwise
     */
    std::shared_ptr<const Track> track(const std::string& name) const override;

    /**
     * @brief Return all processors on a given Track.
     * @param track_id The id of the track
     * @return An std::vector containing all processors on a track in order of processing.
     */
    std::vector<std::shared_ptr<const Processor>> processors_on_track(ObjectId track_id) const override;

    /**
     * @brief Return all tracks.
     * @return An std::vector of containing all Tracks
     */
    std::vector<std::shared_ptr<const Track>> all_tracks() const override;

private:

    std::unordered_map<std::string, std::shared_ptr<Processor>>           _processors_by_name;
    std::unordered_map<ObjectId, std::shared_ptr<Processor>>              _processors_by_id;
    std::unordered_map<ObjectId, std::vector<std::shared_ptr<Processor>>> _processors_by_track;

    mutable std::mutex _processors_by_name_lock;
    mutable std::mutex _processors_by_id_lock;
    mutable std::mutex _processors_by_track_lock;
};

} // namespace engine
} // namespace sushi

#endif //SUSHI_PROCESSOR_CONTAINER_H
