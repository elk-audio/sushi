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
 * @brief Generic, thread safe queue for use in non-rt threads
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
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
        return message;
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
