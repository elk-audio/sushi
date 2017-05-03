/**
 * @Brief Unique id generators for processors, parameters, etc.
 * @copyright MIND Music Labs AB, Stockholm
 *
 *
 */
#ifndef SUSHI_ID_GENERATOR_H
#define SUSHI_ID_GENERATOR_H

#include <atomic>

template <typename T>
class BaseIdGenerator
{
public:
    static T new_id()
    {
        static std::atomic<T> counter{0};
        return counter.fetch_add(1);
    }
};

typedef uint32_t ProcessorId;

class ProcessorIdGenerator : public BaseIdGenerator<ProcessorId>
{ };
#endif //SUSHI_ID_GENERATOR_H
