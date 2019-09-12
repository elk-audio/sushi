/**
 * @brief Simple, non-thread safe fifo queue for use internally in the engine when
 *        concurrent access is not neccesary.
 *        Support for popping elements by value or reference, though in most cases
 *        the most efficient should be to interate over the elements in place and
 *        the call clear()
 *        Capacity should ideally be a power of 2.
 * @copyright MIND Music Labs AB, Stockholm
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

    // Actual capacity is 1 less than reserved storage, otherwis head == tail would indicate both full and empty
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
