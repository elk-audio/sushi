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
 * @Brief Object to map between rt time and real time
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#include <algorithm>
#include <cmath>
#include <cassert>

#include "event_timer.h"
#include "library/constants.h"

namespace sushi {
namespace event_timer {

using namespace std::chrono_literals;
constexpr float MICROSECONDS = std::chrono::microseconds(1s).count();

inline Time calc_chunk_time(float samplerate)
{
     return std::chrono::microseconds(static_cast<int64_t>(std::round(MICROSECONDS / samplerate * AUDIO_CHUNK_SIZE)));
}

EventTimer::EventTimer(float default_sample_rate) : _sample_rate{default_sample_rate},
                                                    _chunk_time{calc_chunk_time(default_sample_rate)}
{
    assert(EventTimer::_incoming_chunk_time.is_lock_free());
}

std::pair<bool, int> EventTimer::sample_offset_from_realtime(Time timestamp)
{
    auto diff = timestamp - _incoming_chunk_time.load();
    if (diff < _chunk_time)
    {
        int64_t offset = (AUDIO_CHUNK_SIZE * diff) / _chunk_time;
        return std::make_pair(true, static_cast<int>(std::max(int64_t{0}, offset)));
    }
    else
    {
        return std::make_pair(false, 0);
    }
}

Time EventTimer::real_time_from_sample_offset(int offset)
{
    return _outgoing_chunk_time + offset * _chunk_time / AUDIO_CHUNK_SIZE;
}

void EventTimer::set_sample_rate(float sample_rate)
{
    _sample_rate = sample_rate;
    _chunk_time = calc_chunk_time(sample_rate);
}

} // end event_timer
} // end sushi
