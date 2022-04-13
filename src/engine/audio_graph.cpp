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
 * @brief Wrapper around the list of tracks used for rt processing and its associated
 *        multicore management
 * @copyright 2017-2022 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include "twine/src/twine_internal.h"

#include "audio_graph.h"

namespace sushi {
namespace engine {

constexpr bool DISABLE_DENORMALS = true;

void external_render_callback(void* data)
{
    /* Signal that this is a realtime audio processing thread */
    twine::ThreadRtFlag rt_flag;

    auto tracks = reinterpret_cast<std::vector<Track*>*>(data);
    for (auto i : *tracks)
    {
        i->render();
    }
}

AudioGraph::AudioGraph(int cpu_cores,
                       int max_no_tracks,
                       bool debug_mode_switches) : _audio_graph(cpu_cores),
                                                   _event_outputs(cpu_cores),
                                                   _cores(cpu_cores),
                                                   _current_core(0)
{
    assert(cpu_cores > 0);
    if (_cores > 1)
    {
        _worker_pool = twine::WorkerPool::create_worker_pool(_cores, DISABLE_DENORMALS, debug_mode_switches);
        for (auto& i : _audio_graph)
        {
            _worker_pool->add_worker(external_render_callback, &i);
            i.reserve(max_no_tracks);
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

} // namespace engine
} // namespace sushi
