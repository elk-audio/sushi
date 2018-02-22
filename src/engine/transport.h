/**
 * @Brief Handles time, tempo and start/stop inside the engine
 * @copyright MIND Music Labs AB, Stockholm
 */

#ifndef SUSHI_TRANSPORT_H
#define SUSHI_TRANSPORT_H

#include "library/time.h"

namespace sushi {
namespace engine {

class Transport
{
public:
    Transport() {}
    ~Transport() {}

    void set_time(Time timestamp, int64_t samples)
    {
        _time = timestamp + _latency;
        _sample_count = samples;
    }

    void set_latency(Time output_latency)
    {
        _latency = output_latency;
    }

    Time current_time() const {return _time;}
    int64_t current_samples() const {return _sample_count;}

private:
    int64_t _sample_count{0};
    Time    _time{0};
    Time    _latency{0};
};


} // namespace engine
} // namespace sushi

#endif //SUSHI_TRANSPORT_H
