#ifndef SUSHI_SPINLOCK_H
#define SUSHI_SPINLOCK_H

#include <atomic>

// since std::hardware_destructive_interference_size is not yet supported in GCC 7
constexpr int ASSUMED_CACHE_LINE_SIZE = 64;

namespace sushi {
/**
 * @brief Simple rt-safe test-and-set spinlock
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
        while (flag.exchange(true, std::memory_order_acquire))
        {}
    }

    void unlock()
    {
        flag.store(false, std::memory_order_release);
    }

private:
    alignas(ASSUMED_CACHE_LINE_SIZE) std::atomic_bool flag{false};
};

} // end namespace sushi
#endif //SUSHI_SPINLOCK_H
