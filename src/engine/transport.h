/**
 * @Brief Handles time and tempo inside the engine
 * @copyright MIND Music Labs AB, Stockholm
 *
 *
 */


#ifndef SUSHI_TRANSPORT_H
#define SUSHI_TRANSPORT_H

#include "library/types.h"

namespace sushi {
namespace engine {

class Transport
{
public:
    Transport() {}
    ~Transport() {}

    void set_time(Time timestamp, int64_t samples)
    {
        _time = timestamp;
        _sample_count = samples;
    }

    Time current_time() const;
    int64_t current_samples() const;

private:
    int64_t _sample_count{0};
    Time    _time;
};


} // namespace engine
} // namespace sushi

#endif //SUSHI_TRANSPORT_H
