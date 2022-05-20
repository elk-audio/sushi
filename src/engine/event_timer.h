/*
 * Copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk
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
 * @Brief Object to map between rt time and real time
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_EVENT_TIMER_H
#define SUSHI_EVENT_TIMER_H

#include <atomic>
#include <tuple>

#include "library/time.h"

namespace sushi {
namespace event_timer {

class EventTimer
{
public:
    explicit EventTimer(float default_sample_rate);

    ~EventTimer() = default;

    /**
     * @brief Convert a timestamp to a sample offset within the next chunk
     * @param timestamp A real time timestamp
     * @return If the timestamp falls withing the next chunk, the function
     *         returns true and the offset in samples. If the timestamp
     *         lies further in the future, the function returns false and
     *         the returned offset is not valid.
     */
    std::pair<bool, int> sample_offset_from_realtime(Time timestamp) const;

    /**
     * @brief Convert a sample offset to real time.
     * @param offset Offset in samples
     * @return Timestamp
     */
    Time real_time_from_sample_offset(int offset) const;

    /**
     * @brief Set the samplerate of the converter.
     * @param sample_rate Samplerate in Hz
     */
    void set_sample_rate(float sample_rate);

    /**
     * @brief Called from the rt part when all rt events have been processed, essentially
     *        closing the window for events for this chunk
     * @param timestamp The time when the currently processed chunk is outputted
     */
    void set_incoming_time(Time timestamp) {_incoming_chunk_time.store(timestamp + _chunk_time);}

    /**
     * @brief Called from the event thread when all outgoing events from a chunk have
     *        been processed
     * @param timestamp of the previously processed audio chunk
     */
    void set_outgoing_time(Time timestamp) {_outgoing_chunk_time = timestamp + _chunk_time;}

private:
    float               _sample_rate;
    Time                _chunk_time;
    Time                _outgoing_chunk_time{IMMEDIATE_PROCESS};
    std::atomic<Time>   _incoming_chunk_time{IMMEDIATE_PROCESS};
};

} // end event_timer
} // end sushi


#endif //SUSHI_EVENT_TIMER_H
