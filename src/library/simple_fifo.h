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
 * @brief Simple, non-thread safe fifo queue for use internally in the engine when
 *        concurrent access is not neccesary.
 *        Support for popping elements by value or reference, though in most cases
 *        the most efficient should be to iterate over the elements in place and
 *        the call clear()
 *        Capacity should ideally be a power of 2.
 * @copyright 2017-2019 Modern Ancient Instruments Networked AB, dba Elk, Stockholm
 */

#ifndef SUSHI_SIMPLE_FIFO_H
#define SUSHI_SIMPLE_FIFO_H

#include <array>
#include <cassert>

namespace sushi {

template<typename T, size_t storage_capacity>
class SimpleFifo
{
public:
    bool push(const T& element)
    {
        int new_tail = increment(_tail);
        if (new_tail == _head)
        {
            return false; // Fifo is full
        }
        _data[_tail] = element;
        _tail = new_tail;
        return true;
    }

    bool pop(T& element)
    {
        if (empty())
        {
            return false; // Fifo is empty
        }
        element = _data[_head];
        _head = increment(_head);
        return true;
    }

    const T& pop()
    {
        // This should not be called on an empty queue
        assert(empty() == false);
        int old_head = _head;
        _head = increment(_head);
        return _data[old_head];
    }

    T operator [](int i) const
    {
        assert(static_cast<size_t>(i) < storage_capacity);
        return _data[(_head + i) % storage_capacity];
    }

    T& operator [](int i)
    {
        assert(static_cast<size_t>(i) < storage_capacity);
        return _data[(_head + i) % storage_capacity];
    }

    int size() const
    {
        if (_head <= _tail)
        {
            return _tail - _head;
        }
        return storage_capacity - _head + _tail;
    }

    // Actual capacity is 1 less than reserved storage, otherwise head == tail would indicate both full and empty
    int capacity() const {return storage_capacity - 1;}

    bool empty() const {return _head == _tail;}

    void clear()
    {
        _head = 0;
        _tail = 0;
    }

private:
    inline int increment(int index) {return (index + 1) % storage_capacity;}

    std::array<T, storage_capacity> _data;
    // Elements are pushed at the tail and read from the head (head is first in the queue)
    int _head{0};
    int _tail{0};
};

}// namespace sushi

#endif //SUSHI_SIMPLE_FIFO_H
