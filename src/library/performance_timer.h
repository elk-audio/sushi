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

namespace sushi {
namespace performance {

using TimePoint = std::chrono::nanoseconds;
constexpr int MAX_LOG_ENTRIES = 20000;
// since std::hardware_destructive_interference_size is not yet supported in GCC 7
constexpr int ASSUMED_CACHE_LINE_SIZE = 64;

struct ProcessTimings
{
    float avg_case{1};
    float min_case{1};
    float max_case{0};
};


/*
 * Simplest form of Test-and-Set spinlock.
 */
class SpinLock
{
public:
    SpinLock() = default;
    MIND_DECLARE_NON_COPYABLE(SpinLock);

    void lock()
    {
        while (flag.load(std::memory_order_relaxed) == true)
        {
            /* Spin until flag is cleared. According to
             * https://geidav.wordpress.com/2016/03/23/test-and-set-spinlocks/
             * this is better as is causes fewer cache invalidations */
        }
        while (flag.exchange(true, std::memory_order_acquire)) {}
    }

    void unlock()
    {
        flag.store(false, std::memory_order_release);
    }

private:
    alignas(ASSUMED_CACHE_LINE_SIZE) std::atomic_bool flag{false};
};


class ProcessTimer
{
public:
    MIND_DECLARE_NON_COPYABLE(ProcessTimer);
    ProcessTimer() = default;
    ~ProcessTimer();

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
            // if queue is full, just drop entries.
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
            // if queue is full, just drop entries.
        }
    }

    void enable(bool enabled);

    std::optional<ProcessTimings> timings_for_node(int id);
    void save_all_to_file(const std::string& path);

protected:

    struct TimingLogPoint
    {
        int id;
        TimePoint delta_time;
    };

    struct TimingNode
    {
        int id;
        std::string name;
        ProcessTimings timings;
    };

    void _worker();
    ProcessTimings _calculate_timings(const std::vector<TimingLogPoint>& entries);
    ProcessTimings _merge_timings(ProcessTimings prev_timings, ProcessTimings new_timings);

    std::thread _process_thread;
    float _period;
    std::atomic_bool _enabled;
    SpinLock _queue_lock;

    std::map<int, TimingNode>  _timings;
    std::mutex _timing_lock;
    memory_relaxed_aquire_release::CircularFifo<TimingLogPoint, MAX_LOG_ENTRIES> _entry_queue;
};



} // namespace performance
} // namespace suushi

#endif //SUSHI_PERFORMANCE_TIMER_H
