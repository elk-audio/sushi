/*
 * Copyright 2017-2023 Elk Audio AB
 *
 * SUSHI is free software: you can redistribute it and/or modify it under the terms of
 * the GNU Affero General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * SUSHI is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. See the GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License along with
 * SUSHI. If not, see http://www.gnu.org/licenses/
 */

/**
 * @brief Wrapper around the list of tracks used for rt processing and its associated
 *        multicore management
 * @Copyright 2017-2023 Elk Audio AB, Stockholm
 */

#include "elklog/static_logger.h"

#include "audio_graph.h"

namespace sushi::internal::engine {

ELKLOG_GET_LOGGER_WITH_MODULE_NAME("audio graph");

constexpr bool DISABLE_DENORMALS = true;

/**
 * Real-time worker thread callback method.
 */
void external_render_callback(void* data)
{
    auto tracks = reinterpret_cast<std::vector<sushi::internal::engine::Track*>*>(data);

    for (auto track : *tracks)
    {
        track->render();
    }
}

AudioGraph::AudioGraph(int cpu_cores,
                       int max_no_tracks,
                       [[maybe_unused]] float sample_rate,
                       [[maybe_unused]] std::optional<std::string> device_name,
                       bool debug_mode_switches) : _audio_graph(cpu_cores),
                                                   _event_outputs(cpu_cores),
                                                   _cores(cpu_cores),
                                                   _current_core(0)
{
    assert(cpu_cores > 0);

    if (_cores > 1)
    {
        twine::apple::AppleMultiThreadData apple_data {};
#ifdef SUSHI_APPLE_THREADING
        apple_data.chunk_size = AUDIO_CHUNK_SIZE;
        apple_data.current_sample_rate = sample_rate;
        if (device_name.has_value())
        {
            apple_data.device_name = device_name.value();
        }
#endif

        _worker_pool = twine::WorkerPool::create_worker_pool(_cores,
                                                             apple_data,
                                                             DISABLE_DENORMALS,
                                                             debug_mode_switches);

        for (auto& tracks : _audio_graph)
        {
            auto status = _worker_pool->add_worker(external_render_callback,
                                                   &tracks);

            if (status.first != twine::WorkerPoolStatus::OK)
            {
#ifdef SUSHI_APPLE_THREADING
                ELKLOG_LOG_ERROR("Failed to start twine worker: {}",  twine::apple::status_to_string(status.second));
#endif
            }

            tracks.reserve(max_no_tracks);
        }
    }
    else
    {
        _audio_graph[0].reserve(max_no_tracks);
    }
}

bool AudioGraph::add(Track* track)
{
    auto& slot = _audio_graph[_current_core];
    if (slot.size() < slot.capacity())
    {
        track->set_event_output(&_event_outputs[_current_core]);
        slot.push_back(track);
        _current_core = (_current_core + 1) % _cores;
        return true;
    }
    return false;
}

bool AudioGraph::add_to_core(Track* track, int core)
{
    assert(core < _cores);
    auto& slot = _audio_graph[core];
    if (slot.size() < slot.capacity())
    {
        track->set_event_output(&_event_outputs[core]);
        slot.push_back(track);
        return true;
    }
    return false;
}

bool AudioGraph::remove(Track* track)
{
    for (auto& slot : _audio_graph)
    {
        for (auto i = slot.begin(); i != slot.end(); ++i)
        {
            if (*i == track)
            {
                slot.erase(i);
                return true;
            }
        }
    }
    return false;
}

void AudioGraph::render()
{
    if (_cores == 1)
    {
        for (auto& track : _audio_graph[0])
        {
            track->render();
        }
    }
    else
    {
        _worker_pool->wakeup_and_wait();
    }
}

} // end namespace sushi::internal::engine
