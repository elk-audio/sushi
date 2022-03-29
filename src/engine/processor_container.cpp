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

#include "processor_container.h"
#include "logging.h"

namespace sushi {
namespace engine {

SUSHI_GET_LOGGER_WITH_MODULE_NAME("engine");

bool ProcessorContainer::add_processor(std::shared_ptr<Processor> processor)
{
    std::scoped_lock<std::mutex, std::mutex> lock(_processors_by_name_lock, _processors_by_id_lock);
    if (_processors_by_name.count(processor->name()) > 0)
    {
        return false;
    }
    _processors_by_name[processor->name()] = processor;
    _processors_by_id[processor->id()] = processor;
    return true;
}

bool ProcessorContainer::add_track(std::shared_ptr<Track> track)
{
    std::scoped_lock<std::mutex> lock(_processors_by_track_lock);
    if (_processors_by_track.count(track->id()) > 0)
    {
        return false;
    }
    _processors_by_track[track->id()].clear();
    return true;
}

bool ProcessorContainer::remove_processor(ObjectId id)
{
    auto processor = this->processor(id);
    if (processor == nullptr)
    {
        return false;
    }
    {
        std::scoped_lock<std::mutex> lock(_processors_by_id_lock);
        [[maybe_unused]] auto count = _processors_by_id.erase(processor->id());
        SUSHI_LOG_WARNING_IF(count != 1, "Erased {} instances of processor {}", count, processor->name())
    }
    {
        std::scoped_lock<std::mutex> lock(_processors_by_name_lock);
        [[maybe_unused]] auto count = _processors_by_name.erase(processor->name());
        SUSHI_LOG_WARNING_IF(count != 1, "Erased {} instances of processor {}", count, processor->name())
    }
    return true;
}

bool ProcessorContainer::remove_track(ObjectId track_id)
{
    std::scoped_lock<std::mutex> lock(_processors_by_track_lock);
    assert(_processors_by_track[track_id].empty());
    _processors_by_track.erase(track_id);
    return true;
}

bool ProcessorContainer::add_to_track(std::shared_ptr<Processor> processor, ObjectId track_id,
                                      std::optional<ObjectId> before_id)
{
    std::scoped_lock<std::mutex> lock(_processors_by_track_lock);
    auto& track_processors = _processors_by_track[track_id];

    if (before_id.has_value())
    {
        for (auto i = track_processors.begin(); i != track_processors.end(); ++i)
        {
            if ((*i)->id() == before_id.value())
            {
                track_processors.insert(i, processor);
                return true;
            }
        }
        // If we end up here, the track's processing chain and _processors_by_track has diverged.
        assert(false);
    }
    else
    {
        track_processors.push_back(processor);
    }
    return true;
}

bool ProcessorContainer::processor_exists(ObjectId id) const
{
    std::scoped_lock<std::mutex> lock(_processors_by_id_lock);
    return _processors_by_id.count(id) > 0;
}

bool ProcessorContainer::processor_exists(const std::string& name) const
{
    std::scoped_lock<std::mutex> lock(_processors_by_name_lock);
    return _processors_by_name.count(name) > 0;
}

bool ProcessorContainer::remove_from_track(ObjectId processor_id, ObjectId track_id)
{
    std::scoped_lock<std::mutex> lock(_processors_by_track_lock);
    auto& track_processors = _processors_by_track[track_id];
    for (auto i = track_processors.cbegin(); i != track_processors.cend(); ++i)
    {
        if ((*i)->id() == processor_id)
        {
            track_processors.erase(i);
            return true;
        }
    }
    return false;
}

std::vector<std::shared_ptr<const Processor>> ProcessorContainer::all_processors() const
{
    std::vector<std::shared_ptr<const Processor>> processors;
    std::scoped_lock<std::mutex> lock(_processors_by_id_lock);
    processors.reserve(_processors_by_id.size());
    for (const auto& p : _processors_by_id)
    {
        processors.emplace_back(p.second);
    }
    return processors;
}

std::shared_ptr<Processor> ProcessorContainer::mutable_processor(ObjectId id) const
{
    return std::const_pointer_cast<Processor>(this->processor(id));
}

std::shared_ptr<Processor> ProcessorContainer::mutable_processor(const std::string& name) const
{
    return std::const_pointer_cast<Processor>(this->processor(name));
}

std::shared_ptr<const Processor> ProcessorContainer::processor(ObjectId id) const
{
    std::scoped_lock<std::mutex> lock(_processors_by_id_lock);
    auto processor_node = _processors_by_id.find(id);
    if (processor_node == _processors_by_id.end())
    {
        return nullptr;
    }
    return processor_node->second;
}

std::shared_ptr<const Processor> ProcessorContainer::processor(const std::string& name) const
{
    std::scoped_lock<std::mutex> lock(_processors_by_name_lock);
    auto processor_node = _processors_by_name.find(name);
    if (processor_node == _processors_by_name.end())
    {
        return nullptr;
    }
    return processor_node->second;
}

std::shared_ptr<Track> ProcessorContainer::mutable_track(ObjectId track_id) const
{
    return std::const_pointer_cast<Track>(this->track(track_id));
}

std::shared_ptr<Track> ProcessorContainer::mutable_track(const std::string& track_name) const
{
    return std::const_pointer_cast<Track>(this->track(track_name));
}

std::shared_ptr<const Track> ProcessorContainer::track(ObjectId track_id) const
{
    /* Check if there is an entry for the ObjectId in the list of track processor
     * In that case we can safely look up the processor by its id and cast it */
    std::scoped_lock<std::mutex> lock(_processors_by_track_lock);
    if (_processors_by_track.count(track_id) > 0)
    {
        std::scoped_lock<std::mutex> id_lock(_processors_by_id_lock);
        auto track_node = _processors_by_id.find(track_id);
        if (track_node != _processors_by_id.end())
        {
            return std::static_pointer_cast<const Track>(track_node->second);
        }
    }
    return nullptr;
}

std::shared_ptr<const Track> ProcessorContainer::track(const std::string& track_name) const
{
    std::scoped_lock<std::mutex> _lock(_processors_by_name_lock);
    auto track_node = _processors_by_name.find(track_name);
    if (track_node != _processors_by_name.end())
    {
        /* Check if there is an entry for the ObjectId in the list of track processor
         * In that case we can safely look up the processor by its id and cast it */
        std::scoped_lock<std::mutex> lock(_processors_by_track_lock);
        if (_processors_by_track.count(track_node->second->id()) > 0)
        {
            return std::static_pointer_cast<const Track>(track_node->second);
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<const Processor>> ProcessorContainer::processors_on_track(ObjectId track_id) const
{
    std::vector<std::shared_ptr<const Processor>> processors;
    std::scoped_lock<std::mutex> lock(_processors_by_track_lock);
    auto track_node = _processors_by_track.find(track_id);
    if (track_node != _processors_by_track.end())
    {
        processors.reserve(track_node->second.size());
        for (const auto& p : track_node->second)
        {
            processors.push_back(p);
        }
    }
    return processors;
}

std::vector<std::shared_ptr<const Track>> ProcessorContainer::all_tracks() const
{
    std::vector<std::shared_ptr<const Track>> tracks;
    {
        std::scoped_lock<std::mutex, std::mutex> lock(_processors_by_track_lock, _processors_by_id_lock);
        for (const auto& p : _processors_by_track)
        {
            auto processor_node = _processors_by_id.find(p.first);
            if (processor_node != _processors_by_id.end())
            {
                tracks.push_back(std::static_pointer_cast<const Track, Processor>(processor_node->second));
            }
        }
    }
    /* Sort the list so tracks are listed in the order they were created */
    std::sort(tracks.begin(), tracks.end(), [](auto a, auto b) {return a->id() < b->id();});
    return tracks;
}

} // namespace engine
} // namespace sushi