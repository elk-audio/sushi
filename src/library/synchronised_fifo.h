/**
 * @brief Generic, thread safe queue for use in non-rt threads
 * @copyright MIND Music Labs AB, Stockholm
 */

#ifndef SUSHI_SYNCHRONISED_FIFO_H
#define SUSHI_SYNCHRONISED_FIFO_H

#include <condition_variable>
#include <chrono>
#include <deque>

template <class T> class SynchronizedQueue
{
public:
    void push(T const& message)
    {
        std::lock_guard<std::mutex> lock(_queue_mutex);
        _queue.push_front(message);
        _notifier.notify_one();
    }

    void push(T&& message)
    {
        std::lock_guard<std::mutex> lock(_queue_mutex);
        _queue.push_front(std::move(message));
        _notifier.notify_one();
    }

    T pop()
    {
        std::lock_guard<std::mutex> lock(_queue_mutex);
        T message = std::move(_queue.back());
        _queue.pop_back();
        return std::move(message);
    }

    void wait_for_data(const std::chrono::milliseconds& timeout)
    {
        if (_queue.empty())
        {
            std::unique_lock<std::mutex> lock(_wait_mutex);
            _notifier.wait_for(lock, timeout);
        }
    }

    bool empty()
    {
        return _queue.empty();
    }
private:
    std::deque<T>           _queue;
    std::mutex              _queue_mutex;
    std::mutex              _wait_mutex;
    std::condition_variable _notifier;
};

#endif //SUSHI_SYNCHRONISED_FIFO_H
