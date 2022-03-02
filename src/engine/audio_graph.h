#ifndef SUSHI_AUDIO_GRAPH_H
#define SUSHI_AUDIO_GRAPH_H

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

#include <vector>

#include "twine/twine.h"

#include "engine/track.h"

namespace sushi {
namespace engine {

class AudioGraph
{
public:
    /**
     * @brief create an AudioGraph instance
     * @param cpu_cores The number of cores to use for audio processing. Must not
     *                  exceed the number of cores on the architecture
     * @param max_no_tracks The maximum number of tracks to reserve space for. As
     *                      add() and remove() could be called from an rt thread
     *                      they must not (de)allocate memory-
     * @param debug_mode_switches Enable xenomai-specific thread debugging
     */
    AudioGraph(int cpu_cores, int max_no_tracks, bool debug_mode_switches = false);

    /**
     * @brief Add a track to the graph. The track will be assigned to a cpu
     *        core on a round robin basis. Must not be called concurrently
     *        with render()
     * @param track the track instance to add
     * @return true if the track was successfully added, false otherwise
     */
    bool add(Track* track);

    /**
     * @brief Add a track to the graph and assign it to a particular cpu core.
     *        Must not be called concurrently with render()
     * @param track the track instance to add
     * @param core The cpu that should be used to process the track.
     * @return true if the track was successfully added, false otherwise
     */
    bool add_to_core(Track* track, int core);

    /**
     * @brief Remove a track from the audio graph. Must not be called concurrently
     *        with render()
     * @param track The instance to remove.
     * @return true if the track was successfully removed, false otherwise.
     */
    bool remove(Track* track);

    /**
     * @brief Return the event output buffers for all tracks. Called after render()
     *        to retrieve events passed from tracks.
     * @return A std::vector of event buffers.
     */
    std::vector<RtEventFifo<>>& event_outputs()
    {
        return _event_outputs;
    }

    /**
     * @brief Render all tracks. If cpu_cores = 1 all processing is done in the
     *        calling thread. With higher number of cores, the calling thread
     *        sleeps while processing is running.
     */
    void render();

private:
    std::vector<std::vector<Track*>>   _audio_graph;
    std::unique_ptr<twine::WorkerPool> _worker_pool;
    std::vector<RtEventFifo<>>         _event_outputs;
    int _cores;
    int _current_core;
};

} // namespace engine
} // namespace sushi

#endif //SUSHI_AUDIO_GRAPH_H
