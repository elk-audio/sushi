#ifndef SUSHI_PERFORMANCE_TIMER_H
#define SUSHI_PERFORMANCE_TIMER_H

#include <chrono>
#include <optional>
#include <atomic>
#include <thread>
#include <map>
#include <mutex>

#include "fifo/circularfifo_memory_relaxed_aquire_release.h"
#include "twine.h"

#include "constants.h"
#include "spinlock.h"

namespace sushi {
namespace performance {

using TimePoint = std::chrono::nanoseconds;
constexpr int MAX_LOG_ENTRIES = 20000;

struct ProcessTimings
{
    float avg_case{1};
    float min_case{1};
    float max_case{0};
};

class PerformanceTimer
{
public:
    MIND_DECLARE_NON_COPYABLE(PerformanceTimer);

    PerformanceTimer() = default;
    ~PerformanceTimer();

    void set_timing_period(TimePoint timing_period);

    void set_timing_period(float samplerate, int buffer_size);

    TimePoint start_timer()
    {
        if (_enabled.load())
        {
            return twine::current_rt_time();
        }
        return std::chrono::nanoseconds(0);
    }

    void stop_timer(TimePoint start_time, int node_id)
    {
        if (_enabled)
        {
            TimingLogPoint tp{node_id, twine::current_rt_time() - start_time};
            _entry_queue.push(tp);
            // if queue is full, drop entries silently.
        }
    }

    void stop_timer_rt_safe(TimePoint start_time, int node_id)
    {
        if(_enabled)
        {
            TimingLogPoint tp{node_id, twine::current_rt_time() - start_time};
            _queue_lock.lock();
            _entry_queue.push(tp);
            _queue_lock.unlock();
            // if queue is full, drop entries silently.
        }
    }

    void enable(bool enabled);

    std::optional<ProcessTimings> timings_for_node(int id);

protected:

    struct TimingLogPoint
    {
        int id;
        TimePoint delta_time;
    };

    struct TimingNode
    {
        int id;
        ProcessTimings timings;
    };

    void _worker();
    ProcessTimings _calculate_timings(const std::vector<TimingLogPoint>& entries);
    ProcessTimings _merge_timings(ProcessTimings prev_timings, ProcessTimings new_timings);

    std::thread _process_thread;
    float _period;
    std::atomic_bool _enabled;

    std::map<int, TimingNode>  _timings;
    std::mutex _timing_lock;
    SpinLock _queue_lock;
    alignas(ASSUMED_CACHE_LINE_SIZE) memory_relaxed_aquire_release::CircularFifo<TimingLogPoint, MAX_LOG_ENTRIES> _entry_queue;
};

} // namespace performance
} // namespace sushi

#endif //SUSHI_PERFORMANCE_TIMER_H
