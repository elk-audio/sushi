#include <cmath>
#include <iostream>

#include "event_timer.h"
#include "library/constants.h"

namespace sushi {
namespace engine {
namespace event_timer {

using namespace std::chrono_literals;
constexpr float MICROSECONDS = std::chrono::microseconds(1s).count();

inline Time calc_chunk_time(float samplerate)
{
     return std::chrono::microseconds(static_cast<int64_t>(std::round(MICROSECONDS / samplerate * AUDIO_CHUNK_SIZE)));
}

EventTimer::EventTimer(float default_sample_rate) : _samplerate{default_sample_rate},
                                                    _chunk_time{calc_chunk_time(default_sample_rate)}
{}

std::pair<bool, int> EventTimer::sample_offset_from_realtime(Time timestamp)
{
    auto diff = timestamp - _incoming_chunk_time.load();
    if (diff < _chunk_time)
    {
        int offset = static_cast<int>(64 * diff / _chunk_time);
        return std::make_pair(true, std::max(0, offset));
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

void EventTimer::set_samplerate(float samplerate)
{
    _samplerate = samplerate;
    _chunk_time = calc_chunk_time(samplerate);
}



} // end event_timer
} // end engine
} // end sushi
