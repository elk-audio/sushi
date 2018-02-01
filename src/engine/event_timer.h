/**
 * @Brief Object to map between rt time and real time
 * @copyright MIND Music Labs AB, Stockholm
 */

#ifndef SUSHI_EVENT_TIMER_H
#define SUSHI_EVENT_TIMER_H

#include <atomic>
#include <cassert>
#include <tuple>

#include "library/time.h"

namespace sushi {
namespace engine {
namespace event_timer {

class EventTimer
{
public:
    explicit EventTimer(float default_sample_rate);
    /**
     * @brief Convert a timestamp to a sample offset within the next chunk
     * @param timestamp A real time timestamp
     * @return If the timestamp falls withing the next chunk, the function
     *         returns true and the offset in samples. If the timestamp
     *         lies further in the future, the function returns false and
     *         the returned offset is not valid.
     */
    std::pair<bool, int>  sample_offset_from_realtime(Time timestamp);

    /**
     * @brief Convert a sample offset to real time.
     * @param offset Offset in samples
     * @return Timestamp
     */
    Time real_time_from_sample_offset(int offset);

    /**
     * @brief Set the samplerate of the converter.
     * @param samplerate Samplerate in Hz
     */
    void set_samplerate(float samplerate);

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
    float _samplerate;
    Time _chunk_time;
    /* Start time of last chunk coming from the rt part */
    Time _outgoing_chunk_time{IMMEDIATE_PROCESS};
    /* Start time of chunk about to be processed by the rt part */
    std::atomic<Time> _incoming_chunk_time{IMMEDIATE_PROCESS};
};

static_assert(std::atomic<Time>::is_always_lock_free);

} // end event_timer
} // end engine
} // end sushi


#endif //SUSHI_EVENT_TIMER_H
