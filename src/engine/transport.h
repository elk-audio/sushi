/**
 * @Brief Handles time and tempo inside the engine
 * @copyright MIND Music Labs AB, Stockholm
 *
 *
 */


#ifndef SUSHI_TRANSPORT_H
#define SUSHI_TRANSPORT_H

namespace sushi {
namespace engine {

class Transport
{
public:
    Transport() {}
    ~Transport() {}

    void set_time(int64_t usec, int64_t samples)
    {
        _time = usec;
        _sample_count = samples;
    }

    int64_t current_time() const;
    int64_t current_samples() const;

private:
    int64_t _sample_count{0};
    int64_t _time{0};
};


} // namespace engine
} // namespace sushi

#endif //SUSHI_TRANSPORT_H
